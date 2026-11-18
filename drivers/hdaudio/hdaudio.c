/* hdaudio.sys: the audio function driver (port class) for High Definition
   Audio on Windows 98, 98 SE and ME, in C. It drives the audio function
   group (HDAUDIO\FUNC_01) that hdaudbus.sys finds on a codec, only
   through the bus driver's HDAUDIO_BUS_INTERFACE (<hdaudio.h>), so it
   also sits on XP's HDAUDBUS.SYS. It plays 16-bit stereo PCM at 44.1 or
   48 kHz through the function group's first analog output path, with a
   volume and a mute control (codec.c chooses the path).

   StartDevice gets the bus interface from the function group's device and
   makes two subdevices:
     Wave      a WaveCyclic miniport: one render pin, a DAC node and a
               bridge pin. Its stream's DMA buffer is the bus driver's
               (AllocateDmaBuffer), behind an IDmaChannel of our own, and
               a timer notifies the port each interval, as the bus driver
               gives a function driver no interrupt.
     Topology  a topology miniport: the bridge pin from Wave, a volume and
               a mute node, and the line out pin. The nodes drive the
               first amplifier on the codec's path that has them.
   Objects in C: each one is a struct whose first member is its vtable. */
#include "hda.h"
#include <initguid.h>
#include <portcls.h>
#include <ksmedia.h>
#include <hdaudio.h>

#define TAG 'aDHc'
#define FRAGMENTS 8 /* notification intervals in the DMA buffer */

typedef struct WAVE WAVE;
typedef struct STREAM STREAM;

typedef struct {
  LONG Refs;
  HDAUDIO_BUS_INTERFACE Bus;
  HDA_CODEC Codec;
  WAVE *Wave;
  ULONG Gain[2];
  LONG Level[2]; /* 1/65536 dB */
  BOOLEAN Mute;
} ADAPTER;

/* The stream's DMA buffer: an engine and buffer of the bus driver's,
   behind IDmaChannel for the port. */
typedef struct {
  const IDmaChannelVtbl *lpVtbl;
  LONG Refs;
  ADAPTER *Adapter;
  HANDLE Engine;
  PMDL Mdl;
  PVOID Va;
  ULONG Size;       /* allocated: the cyclic buffer the engine runs over */
  ULONG BufferSize; /* what the port is told */
  UCHAR Tag;
  USHORT Fmt;
  volatile ULONG *Position;
} DMA;

struct WAVE {
  const IMiniportWaveCyclicVtbl *lpVtbl;
  LONG Refs;
  ADAPTER *Adapter;
  PPORTWAVECYCLIC Port;
  PSERVICEGROUP ServiceGroup;
  STREAM *Stream; /* the one stream there can be */
};

struct STREAM {
  const IMiniportWaveCyclicStreamVtbl *lpVtbl;
  LONG Refs;
  WAVE *Wave;
  DMA *Dma;
  KSSTATE State;
  BOOLEAN Fresh; /* reset and not run since: the position is 0 */
  ULONG Rate, Interval, FrameSize;
  KTIMER Timer;
  KDPC Dpc;
};

typedef struct {
  const IMiniportTopologyVtbl *lpVtbl;
  LONG Refs;
  ADAPTER *Adapter;
} TOPO;

static BOOLEAN guid_eq(const GUID *a, const GUID *b) {
  const ULONG *x = (const ULONG *)a, *y = (const ULONG *)b;
  return x[0] == y[0] && x[1] == y[1] && x[2] == y[2] && x[3] == y[3];
}

static void *alloc(ULONG size) {
  void *p = ExAllocatePoolWithTag(NonPagedPool, size, TAG);
  if (p)
    RtlZeroMemory(p, size);
  return p;
}

/* ---- The adapter ---- */

static void adapter_release(ADAPTER *a) {
  if (__sync_sub_and_fetch(&a->Refs, 1))
    return;
  if (a->Bus.InterfaceDereference)
    a->Bus.InterfaceDereference(a->Bus.Context);
  ExFreePool(a);
}

/* A codec verb through the bus driver, for codec.c. */
static ULONG bus_verb(PVOID context, ULONG verb) {
  ADAPTER *a = context;
  HDAUDIO_CODEC_TRANSFER t;
  RtlZeroMemory(&t, sizeof t);
  t.Output.Command = verb;
  if (!NT_SUCCESS(a->Bus.TransferCodecVerbs(a->Bus.Context, 1, &t, NULL,
                                            NULL)) ||
      !t.Input.IsValid)
    return 0xFFFFFFFF;
  return t.Input.Response;
}

/* The amplifier's range in 1/65536 dB: 0 dB is its offset and each step
   (size + 1) / 4 dB. A path without one gets a fixed 0 dB. */
static void level_range(HDA_CODEC *h, LONG *min, LONG *max, LONG *step) {
  ULONG c = h->VolNid ? h->VolCaps : 0;
  *step = (LONG)(HDA_AMPCAP_SIZE(c) + 1) * 16384;
  *min = -(LONG)HDA_AMPCAP_OFFSET(c) * *step;
  *max = ((LONG)HDA_AMPCAP_STEPS(c) - (LONG)HDA_AMPCAP_OFFSET(c)) * *step;
}

static void set_level(ADAPTER *a, ULONG ch, LONG level) {
  LONG min, max, step;
  level_range(&a->Codec, &min, &max, &step);
  if (level < min)
    level = min;
  if (level > max)
    level = max;
  a->Gain[ch] = (ULONG)((level - min + step / 2) / step);
  a->Level[ch] = min + (LONG)a->Gain[ch] * step;
}

/* ---- The DMA channel (IDmaChannel) over the bus driver's buffer ---- */

static const IDmaChannelVtbl dma_vtbl;

/* Free the buffer and the engine, and point the DAC at no stream. */
static void dma_free(DMA *d) {
  ADAPTER *a = d->Adapter;
  if (d->Mdl) {
    codec_stream(&a->Codec, 0, 0);
    a->Bus.FreeDmaBuffer(a->Bus.Context, d->Engine);
    d->Mdl = NULL;
    d->Va = NULL;
    d->Size = d->BufferSize = 0;
  }
  if (d->Engine) {
    a->Bus.FreeDmaEngine(a->Bus.Context, d->Engine);
    d->Engine = NULL;
  }
}

/* An engine for 16-bit stereo at rate and a buffer of size bytes, and the
   DAC on its stream. */
static NTSTATUS dma_setup(DMA *d, ULONG rate, ULONG size) {
  ADAPTER *a = d->Adapter;
  HDAUDIO_STREAM_FORMAT f;
  HDAUDIO_CONVERTER_FORMAT conv;
  SIZE_T got;
  ULONG fifo;
  NTSTATUS st;
  dma_free(d);
  f.SampleRate = rate;
  f.ValidBitsPerSample = 16;
  f.ContainerSize = 16;
  f.NumberOfChannels = 2;
  st = a->Bus.AllocateRenderDmaEngine(a->Bus.Context, &f, FALSE, &d->Engine,
                                      &conv);
  if (!NT_SUCCESS(st)) {
    d->Engine = NULL;
    return st;
  }
  st = a->Bus.AllocateDmaBuffer(a->Bus.Context, d->Engine, size, &d->Mdl,
                                &got, &d->Tag, &fifo);
  if (!NT_SUCCESS(st)) {
    d->Mdl = NULL;
    dma_free(d);
    return st;
  }
  d->Va = MmGetSystemAddressForMdl(d->Mdl);
  d->Size = d->BufferSize = (ULONG)got;
  d->Fmt = conv.ConverterFormat;
  st = a->Bus.GetLinkPositionRegister(a->Bus.Context, d->Engine,
                                      (PULONG *)&d->Position);
  if (!NT_SUCCESS(st)) {
    dma_free(d);
    return st;
  }
  codec_stream(&a->Codec, d->Tag, d->Fmt);
  return STATUS_SUCCESS;
}

static NTSTATUS NTAPI dma_qi(PDMACHANNEL This, REFIID iid, PVOID *out) {
  if (guid_eq(iid, &IID_IUnknown) || guid_eq(iid, &IID_IDmaChannel)) {
    *out = This;
    This->lpVtbl->AddRef(This);
    return STATUS_SUCCESS;
  }
  *out = NULL;
  return STATUS_INVALID_PARAMETER;
}

static ULONG NTAPI dma_addref(PDMACHANNEL This) {
  return __sync_add_and_fetch(&((DMA *)This)->Refs, 1);
}

static ULONG NTAPI dma_release(PDMACHANNEL This) {
  DMA *d = (DMA *)This;
  LONG r = __sync_sub_and_fetch(&d->Refs, 1);
  if (!r) {
    dma_free(d);
    adapter_release(d->Adapter);
    ExFreePool(d);
  }
  return r;
}

/* The buffer comes from the bus driver with the stream; the port's own
   requests for one are answered with it. */
static NTSTATUS NTAPI dma_allocate_buffer(PDMACHANNEL This, ULONG size,
                                          PPHYSICAL_ADDRESS constraint) {
  DMA *d = (DMA *)This;
  (void)constraint;
  return d->Mdl && size <= d->Size ? STATUS_SUCCESS
                                   : STATUS_INSUFFICIENT_RESOURCES;
}

static void NTAPI dma_free_buffer(PDMACHANNEL This) { (void)This; }

static ULONG NTAPI dma_size(PDMACHANNEL This) {
  return ((DMA *)This)->Size;
}

static ULONG NTAPI dma_buffer_size(PDMACHANNEL This) {
  return ((DMA *)This)->BufferSize;
}

static void NTAPI dma_set_buffer_size(PDMACHANNEL This, ULONG size) {
  DMA *d = (DMA *)This;
  d->BufferSize = size <= d->Size ? size : d->Size;
}

static PVOID NTAPI dma_system_address(PDMACHANNEL This) {
  return ((DMA *)This)->Va;
}

/* Called as Microsoft's compiler calls a member function that returns a
   struct: the result through a pointer after This (see dma_phys in the
   first version's notes, README.md). */
static PHYSICAL_ADDRESS *NTAPI dma_physical_address(PDMACHANNEL This,
                                                    PHYSICAL_ADDRESS *out) {
  DMA *d = (DMA *)This;
  out->QuadPart = d->Mdl ? (LONGLONG)MmGetMdlPfnArray(d->Mdl)[0] << PAGE_SHIFT
                         : 0;
  if (d->Mdl)
    out->QuadPart += MmGetMdlByteOffset(d->Mdl);
  return out;
}

static PADAPTER_OBJECT NTAPI dma_adapter(PDMACHANNEL This) {
  (void)This;
  return NULL;
}

static void NTAPI dma_copy(PDMACHANNEL This, PVOID dst, PVOID src,
                           ULONG n) {
  (void)This;
  RtlCopyMemory(dst, src, n);
}

static const IDmaChannelVtbl dma_vtbl = {
    .QueryInterface = dma_qi,
    .AddRef = dma_addref,
    .Release = dma_release,
    .AllocateBuffer = dma_allocate_buffer,
    .FreeBuffer = dma_free_buffer,
    .TransferCount = dma_buffer_size,
    .MaximumBufferSize = dma_size,
    .AllocatedBufferSize = dma_size,
    .BufferSize = dma_buffer_size,
    .SetBufferSize = dma_set_buffer_size,
    .SystemAddress = dma_system_address,
    .PhysicalAddress = (PHYSICAL_ADDRESS(NTAPI *)(PDMACHANNEL))(
        void (*)(void))dma_physical_address,
    .GetAdapterObject = dma_adapter,
    .CopyTo = dma_copy,
    .CopyFrom = dma_copy,
};

/* ---- The wave stream (IMiniportWaveCyclicStream) ---- */

static const IMiniportWaveCyclicStreamVtbl stream_vtbl;

/* The rate of a KSDATAFORMAT, or 0 when it is not 16-bit stereo PCM at a
   rate the DAC has. */
static ULONG stream_rate(HDA_CODEC *h, PKSDATAFORMAT df) {
  PWAVEFORMATEX wfx;
  if (df->FormatSize < sizeof(KSDATAFORMAT_WAVEFORMATEX) ||
      !guid_eq(&df->MajorFormat, &KSDATAFORMAT_TYPE_AUDIO) ||
      !guid_eq(&df->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM) ||
      !guid_eq(&df->Specifier, &KSDATAFORMAT_SPECIFIER_WAVEFORMATEX))
    return 0;
  wfx = &((PKSDATAFORMAT_WAVEFORMATEX)df)->WaveFormatEx;
  if ((wfx->wFormatTag != WAVE_FORMAT_PCM &&
       wfx->wFormatTag != WAVE_FORMAT_EXTENSIBLE) ||
      wfx->nChannels != 2 || wfx->wBitsPerSample != 16 ||
      !(h->DacPcm & (1u << 17)))
    return 0;
  if (wfx->nSamplesPerSec == 48000 && (h->DacPcm & (1u << 6)))
    return 48000;
  if (wfx->nSamplesPerSec == 44100 && (h->DacPcm & (1u << 5)))
    return 44100;
  return 0;
}

/* The fragment for the notification interval in whole 128-byte units (a
   BDL entry's alignment), and a buffer of FRAGMENTS of them from the bus
   driver, for the rate. */
static NTSTATUS stream_setup(STREAM *s) {
  ULONG frame = s->Rate * 4 / 1000 * (s->Interval ? s->Interval : 10);
  NTSTATUS st;
  frame = (frame + 127) & ~127u;
  st = dma_setup(s->Dma, s->Rate, frame * FRAGMENTS);
  if (NT_SUCCESS(st))
    s->FrameSize = frame;
  s->Fresh = TRUE;
  return st;
}

/* Each interval: the port reads the position and moves data. */
static VOID NTAPI stream_tick(PKDPC dpc, PVOID context, PVOID a, PVOID b) {
  STREAM *s = context;
  WAVE *w = s->Wave;
  (void)dpc, (void)a, (void)b;
  w->Port->lpVtbl->Notify(w->Port, w->ServiceGroup);
}

static NTSTATUS NTAPI stream_qi(PMINIPORTWAVECYCLICSTREAM This, REFIID iid,
                                PVOID *out) {
  if (guid_eq(iid, &IID_IUnknown) ||
      guid_eq(iid, &IID_IMiniportWaveCyclicStream)) {
    *out = This;
    This->lpVtbl->AddRef(This);
    return STATUS_SUCCESS;
  }
  *out = NULL;
  return STATUS_INVALID_PARAMETER;
}

static ULONG NTAPI stream_addref(PMINIPORTWAVECYCLICSTREAM This) {
  return __sync_add_and_fetch(&((STREAM *)This)->Refs, 1);
}

static ULONG NTAPI stream_release(PMINIPORTWAVECYCLICSTREAM This) {
  STREAM *s = (STREAM *)This;
  LONG r = __sync_sub_and_fetch(&s->Refs, 1);
  if (!r) {
    KeCancelTimer(&s->Timer);
    if (s->Dma->Engine) {
      HANDLE h = s->Dma->Engine;
      s->Wave->Adapter->Bus.SetDmaEngineState(s->Wave->Adapter->Bus.Context,
                                              ResetState, 1, &h);
    }
    s->Dma->lpVtbl->Release((PDMACHANNEL)s->Dma);
    s->Wave->Stream = NULL;
    s->Wave->lpVtbl->Release((PMINIPORTWAVECYCLIC)s->Wave);
    ExFreePool(s);
  }
  return r;
}

static NTSTATUS NTAPI stream_set_format(PMINIPORTWAVECYCLICSTREAM This,
                                        PKSDATAFORMAT df) {
  STREAM *s = (STREAM *)This;
  ULONG rate;
  if (s->State == KSSTATE_RUN)
    return STATUS_INVALID_DEVICE_REQUEST;
  rate = stream_rate(&s->Wave->Adapter->Codec, df);
  if (!rate)
    return STATUS_INVALID_PARAMETER;
  if (rate == s->Rate)
    return STATUS_SUCCESS;
  s->Rate = rate;
  return stream_setup(s);
}

static ULONG NTAPI stream_set_notification_freq(
    PMINIPORTWAVECYCLICSTREAM This, ULONG interval, PULONG frame) {
  STREAM *s = (STREAM *)This;
  if (interval != s->Interval && s->State != KSSTATE_RUN) {
    s->Interval = interval;
    stream_setup(s);
  }
  *frame = s->FrameSize;
  return s->Interval;
}

static NTSTATUS NTAPI stream_set_state(PMINIPORTWAVECYCLICSTREAM This,
                                       KSSTATE state) {
  STREAM *s = (STREAM *)This;
  ADAPTER *a = s->Wave->Adapter;
  HANDLE h = s->Dma->Engine;
  NTSTATUS st = STATUS_SUCCESS;
  LARGE_INTEGER due;
  if (!h)
    return STATUS_INVALID_DEVICE_REQUEST;
  switch (state) {
  case KSSTATE_RUN:
    st = a->Bus.SetDmaEngineState(a->Bus.Context, RunState, 1, &h);
    if (!NT_SUCCESS(st))
      return st;
    s->Fresh = FALSE;
    due.QuadPart = -10000LL * s->Interval;
    KeSetTimerEx(&s->Timer, due, s->Interval, &s->Dpc);
    break;
  case KSSTATE_ACQUIRE:
  case KSSTATE_PAUSE:
    if (s->State == KSSTATE_RUN) {
      KeCancelTimer(&s->Timer);
      st = a->Bus.SetDmaEngineState(a->Bus.Context, StopState, 1, &h);
    }
    break;
  case KSSTATE_STOP:
    KeCancelTimer(&s->Timer);
    /* Back to the start of the buffer. */
    st = a->Bus.SetDmaEngineState(a->Bus.Context, ResetState, 1, &h);
    s->Fresh = TRUE;
    break;
  }
  if (NT_SUCCESS(st))
    s->State = state;
  return st;
}

static NTSTATUS NTAPI stream_get_position(PMINIPORTWAVECYCLICSTREAM This,
                                          PULONG position) {
  STREAM *s = (STREAM *)This;
  DMA *d = s->Dma;
  ULONG pos = s->Fresh || !d->Position ? 0 : *d->Position;
  *position = d->BufferSize ? pos % d->BufferSize : 0;
  return STATUS_SUCCESS;
}

static NTSTATUS NTAPI stream_normalize(PMINIPORTWAVECYCLICSTREAM This,
                                       PLONGLONG position) {
  STREAM *s = (STREAM *)This;
  LONGLONG bps = (LONGLONG)s->Rate * 4; /* bytes a second */
  *position = *position / bps * 10000000 + *position % bps * 10000000 / bps;
  return STATUS_SUCCESS;
}

static void NTAPI stream_silence(PMINIPORTWAVECYCLICSTREAM This, PVOID buf,
                                 ULONG count) {
  (void)This;
  RtlZeroMemory(buf, count); /* 16-bit PCM is signed */
}

static const IMiniportWaveCyclicStreamVtbl stream_vtbl = {
    .QueryInterface = stream_qi,
    .AddRef = stream_addref,
    .Release = stream_release,
    .SetFormat = stream_set_format,
    .SetNotificationFreq = stream_set_notification_freq,
    .SetState = stream_set_state,
    .GetPosition = stream_get_position,
    .NormalizePhysicalPosition = stream_normalize,
    .Silence = stream_silence,
};

/* ---- The wave miniport (IMiniportWaveCyclic) ---- */

/* The STATIC_ GUIDs are flat lists, without braces for Data4. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"

#define AUDIO_RANGE(rate)                                                    \
  {                                                                          \
      .DataRange = {{.FormatSize = sizeof(KSDATARANGE_AUDIO),                \
                     .MajorFormat = {STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO)}, \
                     .SubFormat = {STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)},  \
                     .Specifier = {STATICGUIDOF(                             \
                         KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)}}},            \
      .MaximumChannels = 2, .MinimumBitsPerSample = 16,                      \
      .MaximumBitsPerSample = 16, .MinimumSampleFrequency = (rate),          \
      .MaximumSampleFrequency = (rate),                                      \
  }

static const KSDATARANGE_AUDIO range_48k = AUDIO_RANGE(48000);
static const KSDATARANGE_AUDIO range_44k1 = AUDIO_RANGE(44100);
static const PKSDATARANGE wave_ranges[] = {(PKSDATARANGE)&range_48k,
                                           (PKSDATARANGE)&range_44k1};

static const KSDATARANGE bridge_range = {{
    .FormatSize = sizeof(KSDATARANGE),
    .MajorFormat = {STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO)},
    .SubFormat = {STATICGUIDOF(KSDATAFORMAT_SUBTYPE_ANALOG)},
    .Specifier = {STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)},
}};
static const PKSDATARANGE bridge_ranges[] = {(PKSDATARANGE)&bridge_range};
#pragma clang diagnostic pop

static const PCPIN_DESCRIPTOR wave_pins[] = {
    /* 0: render, from KMixer */
    {1, 1, 0, NULL,
     {.DataRangesCount = 2, .DataRanges = wave_ranges,
      .DataFlow = KSPIN_DATAFLOW_IN, .Communication = KSPIN_COMMUNICATION_SINK,
      .Category = &KSCATEGORY_AUDIO}},
    /* 1: bridge, to the topology's pin 0 */
    {0, 0, 0, NULL,
     {.DataRangesCount = 1, .DataRanges = bridge_ranges,
      .DataFlow = KSPIN_DATAFLOW_OUT,
      .Communication = KSPIN_COMMUNICATION_NONE,
      .Category = &KSCATEGORY_AUDIO}},
};

static const PCNODE_DESCRIPTOR wave_nodes[] = {
    {0, NULL, &KSNODETYPE_DAC, NULL},
};

/* A node's pin 1 is its input, pin 0 its output. */
static const PCCONNECTION_DESCRIPTOR wave_connections[] = {
    {PCFILTER_NODE, 0, 0, 1},
    {0, 0, PCFILTER_NODE, 1},
};

static const PCFILTER_DESCRIPTOR wave_filter = {
    0, NULL,
    sizeof(PCPIN_DESCRIPTOR), 2, wave_pins,
    sizeof(PCNODE_DESCRIPTOR), 1, wave_nodes,
    2, wave_connections,
    0, NULL,
};

static NTSTATUS NTAPI wave_qi(PMINIPORTWAVECYCLIC This, REFIID iid,
                              PVOID *out) {
  if (guid_eq(iid, &IID_IUnknown) || guid_eq(iid, &IID_IMiniport) ||
      guid_eq(iid, &IID_IMiniportWaveCyclic)) {
    *out = This;
    This->lpVtbl->AddRef(This);
    return STATUS_SUCCESS;
  }
  *out = NULL;
  return STATUS_INVALID_PARAMETER;
}

static ULONG NTAPI wave_addref(PMINIPORTWAVECYCLIC This) {
  return __sync_add_and_fetch(&((WAVE *)This)->Refs, 1);
}

static ULONG NTAPI wave_release(PMINIPORTWAVECYCLIC This) {
  WAVE *w = (WAVE *)This;
  LONG r = __sync_sub_and_fetch(&w->Refs, 1);
  if (!r) {
    w->Adapter->Wave = NULL;
    if (w->ServiceGroup)
      w->ServiceGroup->lpVtbl->Release(w->ServiceGroup);
    if (w->Port)
      w->Port->lpVtbl->Release(w->Port);
    adapter_release(w->Adapter);
    ExFreePool(w);
  }
  return r;
}

static NTSTATUS NTAPI wave_get_description(PMINIPORTWAVECYCLIC This,
                                           PPCFILTER_DESCRIPTOR *out) {
  (void)This;
  *out = (PPCFILTER_DESCRIPTOR)&wave_filter;
  return STATUS_SUCCESS;
}

/* The format for the render pin: the client's range (KMixer's, say) must
   allow 16-bit stereo at the rate of our range. */
static NTSTATUS NTAPI wave_intersect(PMINIPORTWAVECYCLIC This, ULONG pin,
                                     PKSDATARANGE client, PKSDATARANGE ours,
                                     ULONG outsize, PVOID result,
                                     PULONG resultsize) {
  WAVE *w = (WAVE *)This;
  PKSDATARANGE_AUDIO c = (PKSDATARANGE_AUDIO)client;
  ULONG rate = ((PKSDATARANGE_AUDIO)ours)->MaximumSampleFrequency;
  PKSDATAFORMAT_WAVEFORMATEX f = result;
  ULONG ratebit = rate == 48000 ? 1u << 6 : 1u << 5;
  if (pin != 0 || !guid_eq(&client->Specifier,
                           &KSDATAFORMAT_SPECIFIER_WAVEFORMATEX))
    return STATUS_NOT_IMPLEMENTED; /* port class's own intersection */
  if (!(w->Adapter->Codec.DacPcm & ratebit))
    return STATUS_NO_MATCH;
  if (client->FormatSize >= sizeof(KSDATARANGE_AUDIO) &&
      (c->MaximumChannels < 2 || c->MinimumBitsPerSample > 16 ||
       c->MaximumBitsPerSample < 16 || c->MinimumSampleFrequency > rate ||
       c->MaximumSampleFrequency < rate))
    return STATUS_NO_MATCH;
  *resultsize = sizeof(KSDATAFORMAT_WAVEFORMATEX);
  if (!outsize)
    return STATUS_BUFFER_OVERFLOW;
  if (outsize < sizeof(KSDATAFORMAT_WAVEFORMATEX))
    return STATUS_BUFFER_TOO_SMALL;
  f->DataFormat = *ours;
  f->DataFormat.FormatSize = sizeof(KSDATAFORMAT_WAVEFORMATEX);
  f->DataFormat.Flags = 0;
  f->DataFormat.SampleSize = 4;
  f->DataFormat.Reserved = 0;
  f->WaveFormatEx.wFormatTag = WAVE_FORMAT_PCM;
  f->WaveFormatEx.nChannels = 2;
  f->WaveFormatEx.nSamplesPerSec = rate;
  f->WaveFormatEx.nAvgBytesPerSec = rate * 4;
  f->WaveFormatEx.nBlockAlign = 4;
  f->WaveFormatEx.wBitsPerSample = 16;
  f->WaveFormatEx.cbSize = 0;
  return STATUS_SUCCESS;
}

static NTSTATUS NTAPI wave_init(PMINIPORTWAVECYCLIC This, PUNKNOWN adapter,
                                PRESOURCELIST resources,
                                PPORTWAVECYCLIC port) {
  WAVE *w = (WAVE *)This;
  (void)adapter, (void)resources;
  w->Port = port;
  port->lpVtbl->AddRef(port);
  w->Adapter->Wave = w;
  return PcNewServiceGroup(&w->ServiceGroup, NULL);
}

static NTSTATUS NTAPI wave_new_stream(PMINIPORTWAVECYCLIC This,
                                      PMINIPORTWAVECYCLICSTREAM *out,
                                      PUNKNOWN outer, POOL_TYPE pool,
                                      ULONG pin, BOOLEAN capture,
                                      PKSDATAFORMAT df, PDMACHANNEL *dma,
                                      PSERVICEGROUP *group) {
  WAVE *w = (WAVE *)This;
  STREAM *s;
  DMA *d;
  ULONG rate;
  NTSTATUS st;
  (void)outer, (void)pool;
  if (capture || pin != 0)
    return STATUS_INVALID_DEVICE_REQUEST;
  if (w->Stream)
    return STATUS_INSUFFICIENT_RESOURCES;
  rate = stream_rate(&w->Adapter->Codec, df);
  if (!rate)
    return STATUS_INVALID_PARAMETER;
  s = alloc(sizeof *s);
  d = alloc(sizeof *d);
  if (!s || !d) {
    if (s)
      ExFreePool(s);
    if (d)
      ExFreePool(d);
    return STATUS_INSUFFICIENT_RESOURCES;
  }
  d->lpVtbl = &dma_vtbl;
  d->Refs = 1;
  d->Adapter = w->Adapter;
  __sync_add_and_fetch(&w->Adapter->Refs, 1);
  s->lpVtbl = &stream_vtbl;
  s->Refs = 1;
  s->Wave = w;
  s->Dma = d;
  s->State = KSSTATE_STOP;
  s->Rate = rate;
  s->Interval = 10;
  KeInitializeTimerEx(&s->Timer, NotificationTimer);
  KeInitializeDpc(&s->Dpc, stream_tick, s);
  st = stream_setup(s);
  if (!NT_SUCCESS(st)) {
    d->lpVtbl->Release((PDMACHANNEL)d);
    ExFreePool(s);
    return st;
  }
  This->lpVtbl->AddRef(This);
  w->Stream = s;
  *out = (PMINIPORTWAVECYCLICSTREAM)s;
  *dma = (PDMACHANNEL)d;
  d->lpVtbl->AddRef((PDMACHANNEL)d);
  *group = w->ServiceGroup;
  w->ServiceGroup->lpVtbl->AddRef(w->ServiceGroup);
  return STATUS_SUCCESS;
}

static const IMiniportWaveCyclicVtbl wave_vtbl = {
    .QueryInterface = wave_qi,
    .AddRef = wave_addref,
    .Release = wave_release,
    .GetDescription = wave_get_description,
    .DataRangeIntersection = wave_intersect,
    .Init = wave_init,
    .NewStream = wave_new_stream,
};

/* ---- The topology miniport (IMiniportTopology) ---- */

static NTSTATUS basic_support(PPCPROPERTY_REQUEST r, ULONG type,
                              const KSPROPERTY_STEPPING_LONG *range) {
  ULONG flags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET |
                KSPROPERTY_TYPE_SET;
  ULONG full = sizeof(KSPROPERTY_DESCRIPTION) +
               (range ? sizeof(KSPROPERTY_MEMBERSHEADER) +
                            sizeof(KSPROPERTY_STEPPING_LONG)
                      : 0);
  if (r->ValueSize >= full) {
    PKSPROPERTY_DESCRIPTION d = r->Value;
    d->AccessFlags = flags;
    d->DescriptionSize = full;
    d->PropTypeSet.Set = KSPROPTYPESETID_General;
    d->PropTypeSet.Id = type;
    d->PropTypeSet.Flags = 0;
    d->MembersListCount = range ? 1 : 0;
    d->Reserved = 0;
    if (range) {
      PKSPROPERTY_MEMBERSHEADER m = (PKSPROPERTY_MEMBERSHEADER)(d + 1);
      m->MembersFlags = KSPROPERTY_MEMBER_STEPPEDRANGES;
      m->MembersSize = sizeof(KSPROPERTY_STEPPING_LONG);
      m->MembersCount = 1;
      m->Flags = 0;
      *(PKSPROPERTY_STEPPING_LONG)(m + 1) = *range;
    }
    r->ValueSize = full;
    return STATUS_SUCCESS;
  }
  if (r->ValueSize >= sizeof(ULONG)) {
    *(PULONG)r->Value = flags;
    r->ValueSize = sizeof(ULONG);
    return STATUS_SUCCESS;
  }
  if (!r->ValueSize) {
    r->ValueSize = full;
    return STATUS_BUFFER_OVERFLOW;
  }
  return STATUS_BUFFER_TOO_SMALL;
}

/* The channel a node property is for: 0 left, 1 right, -1 both. */
static LONG channel(PPCPROPERTY_REQUEST r) {
  LONG ch = r->InstanceSize >= sizeof(LONG) ? *(PLONG)r->Instance : -1;
  return ch == 0 || ch == 1 ? ch : -1;
}

static NTSTATUS NTAPI prop_level(PPCPROPERTY_REQUEST r) {
  ADAPTER *a = ((TOPO *)r->MajorTarget)->Adapter;
  LONG ch = channel(r);
  if (r->Verb & KSPROPERTY_TYPE_BASICSUPPORT) {
    KSPROPERTY_STEPPING_LONG range;
    LONG step;
    level_range(&a->Codec, &range.Bounds.SignedMinimum,
                &range.Bounds.SignedMaximum, &step);
    range.SteppingDelta = (ULONG)step;
    range.Reserved = 0;
    return basic_support(r, VT_I4, &range);
  }
  if (r->ValueSize < sizeof(LONG))
    return STATUS_BUFFER_TOO_SMALL;
  if (r->Verb & KSPROPERTY_TYPE_GET) {
    *(PLONG)r->Value = a->Level[ch == 1];
  } else if (r->Verb & KSPROPERTY_TYPE_SET) {
    LONG v = *(PLONG)r->Value;
    if (ch != 1)
      set_level(a, 0, v);
    if (ch != 0)
      set_level(a, 1, v);
    codec_set_volume(&a->Codec, a->Gain[0], a->Gain[1], a->Mute);
  } else {
    return STATUS_INVALID_DEVICE_REQUEST;
  }
  r->ValueSize = sizeof(LONG);
  return STATUS_SUCCESS;
}

static NTSTATUS NTAPI prop_mute(PPCPROPERTY_REQUEST r) {
  ADAPTER *a = ((TOPO *)r->MajorTarget)->Adapter;
  if (r->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    return basic_support(r, VT_BOOL, NULL);
  if (r->ValueSize < sizeof(BOOL))
    return STATUS_BUFFER_TOO_SMALL;
  if (r->Verb & KSPROPERTY_TYPE_GET) {
    *(PBOOL)r->Value = a->Mute;
  } else if (r->Verb & KSPROPERTY_TYPE_SET) {
    a->Mute = *(PBOOL)r->Value != 0;
    codec_set_volume(&a->Codec, a->Gain[0], a->Gain[1], a->Mute);
  } else {
    return STATUS_INVALID_DEVICE_REQUEST;
  }
  r->ValueSize = sizeof(BOOL);
  return STATUS_SUCCESS;
}

static NTSTATUS NTAPI prop_cpu(PPCPROPERTY_REQUEST r) {
  if (!(r->Verb & KSPROPERTY_TYPE_GET))
    return STATUS_INVALID_DEVICE_REQUEST;
  if (r->ValueSize < sizeof(LONG))
    return STATUS_BUFFER_TOO_SMALL;
  *(PLONG)r->Value = KSAUDIO_CPU_RESOURCES_NOT_HOST_CPU;
  r->ValueSize = sizeof(LONG);
  return STATUS_SUCCESS;
}

static const PCPROPERTY_ITEM level_props[] = {
    {&KSPROPSETID_Audio, KSPROPERTY_AUDIO_VOLUMELEVEL,
     PCPROPERTY_ITEM_FLAG_GET | PCPROPERTY_ITEM_FLAG_SET |
         PCPROPERTY_ITEM_FLAG_BASICSUPPORT,
     prop_level},
    {&KSPROPSETID_Audio, KSPROPERTY_AUDIO_CPU_RESOURCES,
     PCPROPERTY_ITEM_FLAG_GET, prop_cpu},
};
static const PCPROPERTY_ITEM mute_props[] = {
    {&KSPROPSETID_Audio, KSPROPERTY_AUDIO_MUTE,
     PCPROPERTY_ITEM_FLAG_GET | PCPROPERTY_ITEM_FLAG_SET |
         PCPROPERTY_ITEM_FLAG_BASICSUPPORT,
     prop_mute},
    {&KSPROPSETID_Audio, KSPROPERTY_AUDIO_CPU_RESOURCES,
     PCPROPERTY_ITEM_FLAG_GET, prop_cpu},
};
static const PCAUTOMATION_TABLE level_automation = {
    sizeof(PCPROPERTY_ITEM), 2, level_props, 0, 0, NULL, 0, 0, NULL, 0};
static const PCAUTOMATION_TABLE mute_automation = {
    sizeof(PCPROPERTY_ITEM), 2, mute_props, 0, 0, NULL, 0, 0, NULL, 0};

static const PCPIN_DESCRIPTOR topo_pins[] = {
    /* 0: from the wave filter's bridge pin */
    {0, 0, 0, NULL,
     {.DataRangesCount = 1, .DataRanges = bridge_ranges,
      .DataFlow = KSPIN_DATAFLOW_IN, .Communication = KSPIN_COMMUNICATION_NONE,
      .Category = &KSCATEGORY_AUDIO}},
    /* 1: the line out (or speaker) jack */
    {0, 0, 0, NULL,
     {.DataRangesCount = 1, .DataRanges = bridge_ranges,
      .DataFlow = KSPIN_DATAFLOW_OUT,
      .Communication = KSPIN_COMMUNICATION_NONE,
      .Category = &KSNODETYPE_SPEAKER}},
};

static const PCNODE_DESCRIPTOR topo_nodes[] = {
    {0, &level_automation, &KSNODETYPE_VOLUME, &KSAUDFNAME_MASTER_VOLUME},
    {0, &mute_automation, &KSNODETYPE_MUTE, &KSAUDFNAME_MASTER_MUTE},
};

static const PCCONNECTION_DESCRIPTOR topo_connections[] = {
    {PCFILTER_NODE, 0, 0, 1},
    {0, 0, 1, 1},
    {1, 0, PCFILTER_NODE, 1},
};

static const PCFILTER_DESCRIPTOR topo_filter = {
    0, NULL,
    sizeof(PCPIN_DESCRIPTOR), 2, topo_pins,
    sizeof(PCNODE_DESCRIPTOR), 2, topo_nodes,
    3, topo_connections,
    0, NULL,
};

static NTSTATUS NTAPI topo_qi(PMINIPORTTOPOLOGY This, REFIID iid,
                              PVOID *out) {
  if (guid_eq(iid, &IID_IUnknown) || guid_eq(iid, &IID_IMiniport) ||
      guid_eq(iid, &IID_IMiniportTopology)) {
    *out = This;
    This->lpVtbl->AddRef(This);
    return STATUS_SUCCESS;
  }
  *out = NULL;
  return STATUS_INVALID_PARAMETER;
}

static ULONG NTAPI topo_addref(PMINIPORTTOPOLOGY This) {
  return __sync_add_and_fetch(&((TOPO *)This)->Refs, 1);
}

static ULONG NTAPI topo_release(PMINIPORTTOPOLOGY This) {
  TOPO *t = (TOPO *)This;
  LONG r = __sync_sub_and_fetch(&t->Refs, 1);
  if (!r) {
    adapter_release(t->Adapter);
    ExFreePool(t);
  }
  return r;
}

static NTSTATUS NTAPI topo_get_description(PMINIPORTTOPOLOGY This,
                                           PPCFILTER_DESCRIPTOR *out) {
  (void)This;
  *out = (PPCFILTER_DESCRIPTOR)&topo_filter;
  return STATUS_SUCCESS;
}

static NTSTATUS NTAPI topo_intersect(PMINIPORTTOPOLOGY This, ULONG pin,
                                     PKSDATARANGE client, PKSDATARANGE ours,
                                     ULONG outsize, PVOID result,
                                     PULONG resultsize) {
  (void)This, (void)pin, (void)client, (void)ours, (void)outsize,
      (void)result, (void)resultsize;
  return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS NTAPI topo_init(PMINIPORTTOPOLOGY This, PUNKNOWN adapter,
                                PRESOURCELIST resources,
                                PPORTTOPOLOGY port) {
  (void)This, (void)adapter, (void)resources, (void)port;
  return STATUS_SUCCESS;
}

static const IMiniportTopologyVtbl topo_vtbl = {
    .QueryInterface = topo_qi,
    .AddRef = topo_addref,
    .Release = topo_release,
    .GetDescription = topo_get_description,
    .DataRangeIntersection = topo_intersect,
    .Init = topo_init,
};


/* ---- The adapter driver ---- */

/* Port class makes our device in AddDevice and calls StartDevice with it;
   the bus interface comes from the function group's device beneath. */
#define MAX_DEVICES 8
static struct {
  PDEVICE_OBJECT Fdo, Pdo;
} devices[MAX_DEVICES];
static ULONG next;

static PDEVICE_OBJECT pdo_of(PDEVICE_OBJECT fdo) {
  for (ULONG i = 0; i < MAX_DEVICES; i++)
    if (devices[i].Fdo == fdo)
      return devices[i].Pdo;
  return NULL;
}

/* IRP_MN_QUERY_INTERFACE for the bus interface, to the top of the
   function group's stack (it passes our device on its way down). */
static NTSTATUS query_bus(PDEVICE_OBJECT pdo, HDAUDIO_BUS_INTERFACE *bus) {
  PDEVICE_OBJECT top = IoGetAttachedDeviceReference(pdo);
  IO_STATUS_BLOCK io;
  KEVENT done;
  PIO_STACK_LOCATION sp;
  NTSTATUS st;
  PIRP irp;
  KeInitializeEvent(&done, NotificationEvent, FALSE);
  irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP, top, NULL, 0, NULL, &done,
                                     &io);
  if (!irp) {
    ObDereferenceObject(top);
    return STATUS_INSUFFICIENT_RESOURCES;
  }
  irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
  sp = IoGetNextIrpStackLocation(irp);
  sp->MinorFunction = IRP_MN_QUERY_INTERFACE;
  sp->Parameters.QueryInterface.InterfaceType = &GUID_HDAUDIO_BUS_INTERFACE;
  sp->Parameters.QueryInterface.Size = sizeof *bus;
  sp->Parameters.QueryInterface.Version = 0x0100;
  sp->Parameters.QueryInterface.Interface = (PINTERFACE)bus;
  sp->Parameters.QueryInterface.InterfaceSpecificData = NULL;
  st = IoCallDriver(top, irp);
  if (st == STATUS_PENDING) {
    KeWaitForSingleObject(&done, Executive, KernelMode, FALSE, NULL);
    st = io.Status;
  }
  ObDereferenceObject(top);
  if (NT_SUCCESS(st) && (!bus->TransferCodecVerbs || !bus->Context))
    st = STATUS_NOT_SUPPORTED;
  return st;
}

/* Make a port of class clsid with miniport, and register it as name. */
static NTSTATUS subdevice(PDEVICE_OBJECT dev, PIRP irp,
                          PRESOURCELIST resources, REFCLSID clsid,
                          PUNKNOWN miniport, PWCHAR name, PPORT *out) {
  PPORT port;
  NTSTATUS st = PcNewPort(&port, clsid);
  if (!NT_SUCCESS(st))
    return st;
  st = port->lpVtbl->Init(port, dev, irp, miniport, NULL, resources);
  if (NT_SUCCESS(st))
    st = PcRegisterSubdevice(dev, name, (PUNKNOWN)port);
  if (!NT_SUCCESS(st)) {
    port->lpVtbl->Release(port);
    return st;
  }
  *out = port;
  return STATUS_SUCCESS;
}

static NTSTATUS NTAPI StartDevice(PDEVICE_OBJECT dev, PIRP irp,
                                  PRESOURCELIST resources) {
  PDEVICE_OBJECT pdo = pdo_of(dev);
  ADAPTER *a;
  WAVE *w = NULL;
  TOPO *t = NULL;
  PPORT wave_port = NULL, topo_port = NULL;
  UCHAR cad = 0, nid = 0;
  NTSTATUS st;

  if (!pdo)
    return STATUS_NO_SUCH_DEVICE;
  a = alloc(sizeof *a);
  if (!a)
    return STATUS_INSUFFICIENT_RESOURCES;
  a->Refs = 1;
  st = query_bus(pdo, &a->Bus);
  if (!NT_SUCCESS(st)) {
    a->Bus.InterfaceDereference = NULL;
    goto out;
  }
  a->Bus.GetResourceInformation(a->Bus.Context, &cad, &nid);
  a->Codec.Verb = bus_verb;
  a->Codec.VerbContext = a;
  a->Codec.Cad = cad;
  a->Codec.AfgNid = nid;
  st = codec_probe(&a->Codec);
  if (!NT_SUCCESS(st))
    goto out;
  set_level(a, 0, 0);
  set_level(a, 1, 0);
  codec_set_volume(&a->Codec, a->Gain[0], a->Gain[1], FALSE);

  t = alloc(sizeof *t);
  w = alloc(sizeof *w);
  if (!t || !w) {
    st = STATUS_INSUFFICIENT_RESOURCES;
    goto out;
  }
  t->lpVtbl = &topo_vtbl;
  t->Refs = 1;
  t->Adapter = a;
  __sync_add_and_fetch(&a->Refs, 1);
  w->lpVtbl = &wave_vtbl;
  w->Refs = 1;
  w->Adapter = a;
  __sync_add_and_fetch(&a->Refs, 1);

  st = subdevice(dev, irp, resources, &CLSID_PortTopology, (PUNKNOWN)t,
                 L"Topology", &topo_port);
  if (NT_SUCCESS(st))
    st = subdevice(dev, irp, resources, &CLSID_PortWaveCyclic, (PUNKNOWN)w,
                   L"Wave", &wave_port);
  if (NT_SUCCESS(st))
    st = PcRegisterPhysicalConnection(dev, (PUNKNOWN)wave_port, 1,
                                      (PUNKNOWN)topo_port, 0);
out:
  /* The ports hold the miniports, and the miniports the adapter. */
  if (wave_port)
    wave_port->lpVtbl->Release(wave_port);
  if (topo_port)
    topo_port->lpVtbl->Release(topo_port);
  if (w)
    w->lpVtbl->Release((PMINIPORTWAVECYCLIC)w);
  if (t)
    t->lpVtbl->Release((PMINIPORTTOPOLOGY)t);
  adapter_release(a);
  return st;
}

static NTSTATUS NTAPI AddDevice(PDRIVER_OBJECT driver, PDEVICE_OBJECT pdo) {
  NTSTATUS st = PcAddAdapterDevice(driver, pdo, StartDevice, 2, 0);
  if (!NT_SUCCESS(st))
    return st;
  /* The device port class just made is the newest of the driver's. */
  for (ULONG i = 0; i < MAX_DEVICES; i++)
    if (!devices[i].Fdo || devices[i].Pdo == pdo) {
      devices[i].Fdo = driver->DeviceObject;
      devices[i].Pdo = pdo;
      return st;
    }
  /* Full: the oldest entries are of devices long started. */
  devices[next % MAX_DEVICES].Fdo = driver->DeviceObject;
  devices[next++ % MAX_DEVICES].Pdo = pdo;
  return st;
}

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING path) {
  return PcInitializeAdapterDriver(driver, path, AddDevice);
}
