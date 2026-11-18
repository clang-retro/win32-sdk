/* hdaudio.sys: a WDM audio driver (port class) for Intel High Definition
   Audio controllers on Windows 98, 98 SE and ME, in C. It plays 16-bit
   stereo PCM at 44.1 or 48 kHz through the first analog output path of
   the first codec, with a volume and a mute control.

   The adapter (StartDevice) maps the controller, starts it and its codec
   (hda.c) and makes two subdevices:
     Wave      a WaveCyclic miniport: one render pin, a DAC node and a
               bridge pin. Its stream's cyclic buffer is the one the BDL
               describes, a fragment per notification interval, and the
               controller's interrupt on each fragment notifies the port.
     Topology  a topology miniport: the bridge pin from Wave, a volume and
               a mute node, and the line out pin. The nodes drive the
               first amplifier on the codec's path that has them.
   Objects in C: each one is a struct whose first member is its vtable. */
#include "hda.h"
#include <initguid.h>
#include <portcls.h>
#include <ksmedia.h>

#define TAG 'aDHc'
#define WAVE_BUFFER_MAX 0x10000 /* bytes of DMA buffer for the stream */
#define FRAGMENTS 8             /* notification intervals in the buffer */

typedef struct WAVE WAVE;
typedef struct STREAM STREAM;

typedef struct {
  LONG Refs;
  HDA Hda;
  PDMACHANNEL RingsDma;
  PINTERRUPTSYNC Sync;
  WAVE *Wave; /* for the interrupt; NULL until the wave miniport exists */
  ULONG Gain[2];
  LONG Level[2]; /* 1/65536 dB */
  BOOLEAN Mute;
} ADAPTER;

struct WAVE {
  const IMiniportWaveCyclicVtbl *lpVtbl;
  LONG Refs;
  ADAPTER *Adapter;
  PPORTWAVECYCLIC Port;
  PSERVICEGROUP ServiceGroup;
  PDMACHANNEL Dma;
  STREAM *Stream; /* the one stream there can be */
};

struct STREAM {
  const IMiniportWaveCyclicStreamVtbl *lpVtbl;
  LONG Refs;
  WAVE *Wave;
  KSSTATE State;
  BOOLEAN Programmed;
  USHORT Fmt;
  ULONG Rate, Interval, FrameSize;
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

/* IDmaChannel::PhysicalAddress returns a PHYSICAL_ADDRESS, a struct. Port
   class is C++ built with Microsoft's compiler, whose member functions
   return any struct through a hidden pointer that comes after this; a C
   call through the vtable would expect it in EDX:EAX instead. */
typedef PHYSICAL_ADDRESS *(NTAPI *PHYSICAL_ADDRESS_METHOD)(
    PDMACHANNEL This, PHYSICAL_ADDRESS *Result);

static ULONG dma_phys(PDMACHANNEL dma) {
  PHYSICAL_ADDRESS pa;
  pa.QuadPart = 0;
  ((PHYSICAL_ADDRESS_METHOD)(void (*)(void))dma->lpVtbl->PhysicalAddress)(
      dma, &pa);
  return pa.LowPart;
}

/* ---- The adapter ---- */

static void adapter_release(ADAPTER *a) {
  if (__sync_sub_and_fetch(&a->Refs, 1))
    return;
  if (a->Sync) {
    a->Sync->lpVtbl->Disconnect(a->Sync);
    a->Sync->lpVtbl->Release(a->Sync);
  }
  hda_shutdown(&a->Hda);
  if (a->RingsDma) {
    a->RingsDma->lpVtbl->FreeBuffer(a->RingsDma);
    a->RingsDma->lpVtbl->Release(a->RingsDma);
  }
  if (a->Hda.Regs)
    MmUnmapIoSpace((PVOID)a->Hda.Regs, a->Hda.RegsLength);
  ExFreePool(a);
}

static NTSTATUS NTAPI isr(PINTERRUPTSYNC sync, PVOID context) {
  ADAPTER *a = context;
  WAVE *w;
  (void)sync;
  if (!hda_interrupt(&a->Hda))
    return STATUS_UNSUCCESSFUL; /* not ours: the line may be shared */
  w = a->Wave;
  if (w && w->Port && w->ServiceGroup)
    w->Port->lpVtbl->Notify(w->Port, w->ServiceGroup);
  return STATUS_SUCCESS;
}

/* The amplifier's range in 1/65536 dB: 0 dB is its offset and each step
   (size + 1) / 4 dB. A path without one gets a fixed 0 dB. */
static void level_range(HDA *h, LONG *min, LONG *max, LONG *step) {
  ULONG c = h->VolNid ? h->VolCaps : 0;
  *step = (LONG)(HDA_AMPCAP_SIZE(c) + 1) * 16384;
  *min = -(LONG)HDA_AMPCAP_OFFSET(c) * *step;
  *max = ((LONG)HDA_AMPCAP_STEPS(c) - (LONG)HDA_AMPCAP_OFFSET(c)) * *step;
}

static void set_level(ADAPTER *a, ULONG ch, LONG level) {
  LONG min, max, step;
  level_range(&a->Hda, &min, &max, &step);
  if (level < min)
    level = min;
  if (level > max)
    level = max;
  a->Gain[ch] = (ULONG)((level - min + step / 2) / step);
  a->Level[ch] = min + (LONG)a->Gain[ch] * step;
}

/* ---- The wave stream (IMiniportWaveCyclicStream) ---- */

static const IMiniportWaveCyclicStreamVtbl stream_vtbl;

/* The HDA stream format of a KSDATAFORMAT, or 0 when it is not 16-bit
   stereo PCM at a rate the DAC has. */
static USHORT stream_format(HDA *h, PKSDATAFORMAT df, ULONG *rate) {
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
  *rate = wfx->nSamplesPerSec;
  if (*rate == 48000 && (h->DacPcm & (1u << 6)))
    return HDA_FMT_48K | HDA_FMT_16BIT | HDA_FMT_STEREO;
  if (*rate == 44100 && (h->DacPcm & (1u << 5)))
    return HDA_FMT_44K1 | HDA_FMT_16BIT | HDA_FMT_STEREO;
  return 0;
}

/* The fragment for the notification interval, in whole 128-byte units
   (the BDL's alignment), and the buffer of FRAGMENTS of them. */
static void stream_size(STREAM *s) {
  PDMACHANNEL dma = s->Wave->Dma;
  ULONG frame = s->Rate * 4 / 1000 * (s->Interval ? s->Interval : 10);
  ULONG max = dma->lpVtbl->AllocatedBufferSize(dma);
  frame = (frame + 127) & ~127u;
  while (frame * FRAGMENTS > max && frame > 128)
    frame -= 128;
  s->FrameSize = frame;
  dma->lpVtbl->SetBufferSize(dma, frame * FRAGMENTS);
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
    hda_stream_reset(&s->Wave->Adapter->Hda);
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
  USHORT fmt;
  if (s->State == KSSTATE_RUN)
    return STATUS_INVALID_DEVICE_REQUEST;
  fmt = stream_format(&s->Wave->Adapter->Hda, df, &rate);
  if (!fmt)
    return STATUS_INVALID_PARAMETER;
  s->Fmt = fmt;
  s->Rate = rate;
  stream_size(s);
  s->Programmed = FALSE;
  return STATUS_SUCCESS;
}

static ULONG NTAPI stream_set_notification_freq(
    PMINIPORTWAVECYCLICSTREAM This, ULONG interval, PULONG frame) {
  STREAM *s = (STREAM *)This;
  s->Interval = interval;
  stream_size(s);
  s->Programmed = FALSE;
  *frame = s->FrameSize;
  return interval;
}

static NTSTATUS NTAPI stream_set_state(PMINIPORTWAVECYCLICSTREAM This,
                                       KSSTATE state) {
  STREAM *s = (STREAM *)This;
  HDA *h = &s->Wave->Adapter->Hda;
  PDMACHANNEL dma = s->Wave->Dma;
  NTSTATUS st = STATUS_SUCCESS;
  switch (state) {
  case KSSTATE_RUN:
    if (!s->Programmed) {
      st = hda_stream_start(h, dma_phys(dma),
                            dma->lpVtbl->BufferSize(dma), s->FrameSize,
                            s->Fmt);
      if (!NT_SUCCESS(st))
        return st;
      s->Programmed = TRUE;
    }
    hda_stream_run(h, TRUE);
    break;
  case KSSTATE_ACQUIRE:
  case KSSTATE_PAUSE:
    if (s->State == KSSTATE_RUN)
      hda_stream_run(h, FALSE);
    break;
  case KSSTATE_STOP:
    hda_stream_reset(h); /* back to the start of the buffer */
    s->Programmed = FALSE;
    break;
  }
  s->State = state;
  return st;
}

static NTSTATUS NTAPI stream_get_position(PMINIPORTWAVECYCLICSTREAM This,
                                          PULONG position) {
  STREAM *s = (STREAM *)This;
  PDMACHANNEL dma = s->Wave->Dma;
  ULONG size = dma->lpVtbl->BufferSize(dma);
  ULONG pos = s->Programmed ? hda_stream_position(&s->Wave->Adapter->Hda) : 0;
  *position = size ? pos % size : 0;
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
    if (w->Dma) {
      w->Dma->lpVtbl->FreeBuffer(w->Dma);
      w->Dma->lpVtbl->Release(w->Dma);
    }
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
  ULONG sizebit = rate == 48000 ? 1u << 6 : 1u << 5;
  if (pin != 0 || !guid_eq(&client->Specifier,
                           &KSDATAFORMAT_SPECIFIER_WAVEFORMATEX))
    return STATUS_NOT_IMPLEMENTED; /* port class's own intersection */
  if (!(w->Adapter->Hda.DacPcm & sizebit))
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
  NTSTATUS st;
  (void)adapter;
  w->Port = port;
  port->lpVtbl->AddRef(port);
  st = PcNewServiceGroup(&w->ServiceGroup, NULL);
  if (!NT_SUCCESS(st))
    return st;
  st = port->lpVtbl->NewMasterDmaChannel(port, &w->Dma, NULL, resources,
                                         WAVE_BUFFER_MAX, TRUE, FALSE,
                                         Width32Bits, MaximumDmaSpeed);
  if (!NT_SUCCESS(st))
    return st;
  st = w->Dma->lpVtbl->AllocateBuffer(w->Dma, WAVE_BUFFER_MAX, NULL);
  if (!NT_SUCCESS(st))
    return st;
  w->Adapter->Wave = w;
  return STATUS_SUCCESS;
}

static NTSTATUS NTAPI wave_new_stream(PMINIPORTWAVECYCLIC This,
                                      PMINIPORTWAVECYCLICSTREAM *out,
                                      PUNKNOWN outer, POOL_TYPE pool,
                                      ULONG pin, BOOLEAN capture,
                                      PKSDATAFORMAT df, PDMACHANNEL *dma,
                                      PSERVICEGROUP *group) {
  WAVE *w = (WAVE *)This;
  STREAM *s;
  ULONG rate;
  USHORT fmt;
  (void)outer, (void)pool;
  if (capture || pin != 0)
    return STATUS_INVALID_DEVICE_REQUEST;
  if (w->Stream)
    return STATUS_INSUFFICIENT_RESOURCES;
  fmt = stream_format(&w->Adapter->Hda, df, &rate);
  if (!fmt)
    return STATUS_INVALID_PARAMETER;
  s = alloc(sizeof *s);
  if (!s)
    return STATUS_INSUFFICIENT_RESOURCES;
  s->lpVtbl = &stream_vtbl;
  s->Refs = 1;
  s->Wave = w;
  This->lpVtbl->AddRef(This);
  s->State = KSSTATE_STOP;
  s->Fmt = fmt;
  s->Rate = rate;
  s->Interval = 10;
  stream_size(s);
  w->Stream = s;
  *out = (PMINIPORTWAVECYCLICSTREAM)s;
  *dma = w->Dma;
  w->Dma->lpVtbl->AddRef(w->Dma);
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
    level_range(&a->Hda, &range.Bounds.SignedMinimum,
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
    hda_set_volume(&a->Hda, a->Gain[0], a->Gain[1], a->Mute);
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
    hda_set_volume(&a->Hda, a->Gain[0], a->Gain[1], a->Mute);
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
  PCM_PARTIAL_RESOURCE_DESCRIPTOR mem;
  DEVICE_DESCRIPTION dd;
  ADAPTER *a;
  WAVE *w = NULL;
  TOPO *t = NULL;
  PPORT wave_port = NULL, topo_port = NULL;
  NTSTATUS st;

  mem = resources->lpVtbl->FindTranslatedEntry(resources,
                                               CmResourceTypeMemory, 0);
  if (!mem || !resources->lpVtbl->NumberOfEntriesOfType(
                  resources, CmResourceTypeInterrupt))
    return STATUS_DEVICE_CONFIGURATION_ERROR;
  a = alloc(sizeof *a);
  if (!a)
    return STATUS_INSUFFICIENT_RESOURCES;
  a->Refs = 1;
  a->Hda.RegsLength = mem->u.Memory.Length;
  a->Hda.Regs = MmMapIoSpace(mem->u.Memory.Start, mem->u.Memory.Length,
                             MmNonCached);
  if (!a->Hda.Regs) {
    st = STATUS_INSUFFICIENT_RESOURCES;
    goto out;
  }

  /* One page for the CORB, RIRB and BDL. */
  RtlZeroMemory(&dd, sizeof dd);
  dd.Version = DEVICE_DESCRIPTION_VERSION;
  dd.Master = TRUE;
  dd.ScatterGather = TRUE;
  dd.Dma32BitAddresses = TRUE;
  dd.InterfaceType = PCIBus;
  dd.MaximumLength = PAGE_SIZE;
  st = PcNewDmaChannel(&a->RingsDma, NULL, NonPagedPool, &dd, dev);
  if (!NT_SUCCESS(st))
    goto out;
  st = a->RingsDma->lpVtbl->AllocateBuffer(a->RingsDma, PAGE_SIZE, NULL);
  if (!NT_SUCCESS(st))
    goto out;
  a->Hda.Rings = a->RingsDma->lpVtbl->SystemAddress(a->RingsDma);
  a->Hda.RingsPhys = dma_phys(a->RingsDma);

  st = hda_init(&a->Hda);
  if (!NT_SUCCESS(st))
    goto out;
  set_level(a, 0, 0);
  set_level(a, 1, 0);
  hda_set_volume(&a->Hda, a->Gain[0], a->Gain[1], FALSE);

  st = PcNewInterruptSync(&a->Sync, NULL, resources, 0,
                          InterruptSyncModeNormal);
  if (!NT_SUCCESS(st))
    goto out;
  st = a->Sync->lpVtbl->RegisterServiceRoutine(a->Sync, isr, a, FALSE);
  if (NT_SUCCESS(st))
    st = a->Sync->lpVtbl->Connect(a->Sync);
  if (!NT_SUCCESS(st)) {
    a->Sync->lpVtbl->Release(a->Sync);
    a->Sync = NULL;
    goto out;
  }

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
  return PcAddAdapterDevice(driver, pdo, StartDevice, 2, 0);
}

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING path) {
  return PcInitializeAdapterDriver(driver, path, AddDevice);
}
