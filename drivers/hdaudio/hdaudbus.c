/* hdaudbus.sys: a WDM bus driver for Intel High Definition Audio
   controllers on Windows 98, 98 SE and ME, in C. It owns the controller
   (hda.c): the command rings, the stream engines and the interrupt. It
   makes a child device for each function group of each codec, as XP's
   HDAUDBUS.SYS does, with the same hardware IDs:

     HDAUDIO\FUNC_01&VEN_vvvv&DEV_dddd&SUBSYS_ssssssss&REV_rrrr  (audio)
     HDAUDIO\FUNC_02&...                                         (modem)

   and gives each child's function driver (hdaudio.sys for audio, a
   softmodem's driver for a modem) the HD Audio bus interfaces of XP's
   bus driver, GUID_HDAUDIO_BUS_INTERFACE and GUID_HDAUDIO_BUS_INTERFACE_BDL
   (<hdaudio.h>), through IRP_MN_QUERY_INTERFACE. Unsolicited responses
   are read every 20 ms and passed to the callbacks registered for their
   tags. */
#include "hda.h"
#include <initguid.h>
#include <hdaudio.h>

#define TAG 'BDHc'
#define MAX_CHILDREN 16
#define UNSOL_PERIOD_MS 20

typedef struct FDO FDO;

typedef struct {
  BOOLEAN IsFdo;
} COMMON;

/* A child: one function group. */
typedef struct {
  COMMON Common;
  PDEVICE_OBJECT Self;
  FDO *Fdo;
  HDA_FUNCTION Fn;
  BOOLEAN Present; /* still in the bus's relations */
  LONG InterfaceRefs;
} PDO;

/* What the bus keeps for each stream engine besides hda.c's state. */
typedef struct {
  PDO *Owner;
  BOOLEAN Contiguous; /* the BDL interface's buffer: the caller's BDL */
  PVOID Va;
  PHYSICAL_ADDRESS Pa;
  ULONG Size;
  PMDL Mdl;
  PVOID BdlVa; /* the BDL interface's BDL, 256 entries */
  PHYSICAL_ADDRESS BdlPa;
  ULONG Cbl, Lvi;
  PHDAUDIO_BDL_ISR Isr;
  PVOID IsrContext;
} ENGINE;

typedef struct {
  PHDAUDIO_UNSOLICITED_RESPONSE_CALLBACK Routine;
  PVOID Context;
  PDO *Owner;
} EVENT;

struct FDO {
  COMMON Common;
  PDEVICE_OBJECT Self, Lower, Pdo;
  BOOLEAN Started;
  HDA Hda;
  PDMA_ADAPTER Dma;
  PHYSICAL_ADDRESS RingsPa;
  PKINTERRUPT Interrupt;
  KTIMER Timer;
  KDPC TimerDpc;
  ENGINE Engines[HDA_MAX_ENGINES];
  EVENT Events[64]; /* by tag, 1 to 63 */
  ULONG ChildCount;
  PDO *Children[MAX_CHILDREN];
  ULONG Instance;
};

static ULONG instances;

/* ---- Helpers ---- */

/* Strings without swprintf, which 98's NTKERN may not format the same:
   text, a hex number of digits, a decimal one. Each returns the new
   length; the string stays terminated. */
static ULONG wput(PWCHAR b, ULONG n, const WCHAR *s) {
  while (*s)
    b[n++] = *s++;
  b[n] = 0;
  return n;
}

static ULONG whex(PWCHAR b, ULONG n, ULONG v, int digits) {
  while (digits--)
    b[n++] = L"0123456789ABCDEF"[(v >> (digits * 4)) & 0xF];
  b[n] = 0;
  return n;
}

static ULONG wdec(PWCHAR b, ULONG n, ULONG v) {
  WCHAR t[10];
  int k = 0;
  do
    t[k++] = (WCHAR)(L'0' + v % 10);
  while (v /= 10);
  while (k)
    b[n++] = t[--k];
  b[n] = 0;
  return n;
}

static NTSTATUS complete(PIRP irp, NTSTATUS status, ULONG_PTR info) {
  irp->IoStatus.Status = status;
  irp->IoStatus.Information = info;
  IoCompleteRequest(irp, IO_NO_INCREMENT);
  return status;
}

static NTSTATUS NTAPI on_completion(PDEVICE_OBJECT dev, PIRP irp,
                                    PVOID context) {
  (void)dev, (void)irp;
  KeSetEvent((PKEVENT)context, IO_NO_INCREMENT, FALSE);
  return STATUS_MORE_PROCESSING_REQUIRED;
}

/* Pass an IRP down and wait for it. */
static NTSTATUS forward_and_wait(FDO *f, PIRP irp) {
  KEVENT done;
  KeInitializeEvent(&done, NotificationEvent, FALSE);
  IoCopyCurrentIrpStackLocationToNext(irp);
  IoSetCompletionRoutine(irp, on_completion, &done, TRUE, TRUE, TRUE);
  if (IoCallDriver(f->Lower, irp) == STATUS_PENDING)
    KeWaitForSingleObject(&done, Executive, KernelMode, FALSE, NULL);
  return irp->IoStatus.Status;
}

static NTSTATUS pass_down(FDO *f, PIRP irp) {
  IoSkipCurrentIrpStackLocation(irp);
  return IoCallDriver(f->Lower, irp);
}

static ENGINE *engine_of(FDO *f, PDO *p, HANDLE handle, HDA_ENGINE **e) {
  for (ULONG i = 0; i < HDA_MAX_ENGINES; i++)
    if ((HANDLE)&f->Hda.Engines[i] == handle && f->Hda.Engines[i].Used &&
        f->Engines[i].Owner == p) {
      *e = &f->Hda.Engines[i];
      return &f->Engines[i];
    }
  return NULL;
}

static ULONG bus_verb(PVOID context, ULONG verb) {
  return hda_verb(context, verb);
}

/* ---- The bus interfaces (Context is the child's PDO) ---- */

static VOID NTAPI if_reference(PVOID context) {
  __sync_add_and_fetch(&((PDO *)context)->InterfaceRefs, 1);
}

static VOID NTAPI if_dereference(PVOID context) {
  __sync_sub_and_fetch(&((PDO *)context)->InterfaceRefs, 1);
}

/* Synchronous: each verb is sent and its response read before the next;
   the callback, if any, is called before this returns. The verbs go to
   the child's codec whatever address they carry. */
static NTSTATUS NTAPI if_transfer_codec_verbs(
    PVOID context, ULONG count, PHDAUDIO_CODEC_TRANSFER transfers,
    PHDAUDIO_TRANSFER_COMPLETE_CALLBACK callback, PVOID callback_context) {
  PDO *p = context;
  if (!transfers && count)
    return STATUS_INVALID_PARAMETER;
  for (ULONG i = 0; i < count; i++) {
    ULONG resp, verb = (transfers[i].Output.Command & 0x0FFFFFFF) |
                       (ULONG)p->Fn.Cad << 28;
    BOOLEAN ok = hda_command(&p->Fdo->Hda, verb, &resp);
    transfers[i].Input.CompleteResponse = 0;
    transfers[i].Input.Response = resp;
    transfers[i].Input.SDataIn = p->Fn.Cad;
    transfers[i].Input.IsValid = ok;
  }
  if (callback)
    callback(transfers, callback_context);
  return STATUS_SUCCESS;
}

static NTSTATUS allocate_engine(PDO *p, BOOLEAN output,
                                PHDAUDIO_STREAM_FORMAT format, PHANDLE handle,
                                PHDAUDIO_CONVERTER_FORMAT converter) {
  FDO *f = p->Fdo;
  USHORT fmt;
  HDA_ENGINE *e;
  if (!format || !handle || !converter)
    return STATUS_INVALID_PARAMETER;
  fmt = hda_format(format->SampleRate, format->ValidBitsPerSample,
                   format->NumberOfChannels);
  if (fmt == 0xFFFF)
    return STATUS_INVALID_PARAMETER;
  e = hda_engine_alloc(&f->Hda, output);
  if (!e)
    return STATUS_INSUFFICIENT_RESOURCES;
  e->Fmt = fmt;
  RtlZeroMemory(&f->Engines[e->Index], sizeof(ENGINE));
  f->Engines[e->Index].Owner = p;
  *handle = (HANDLE)e;
  converter->ConverterFormat = fmt;
  return STATUS_SUCCESS;
}

static NTSTATUS NTAPI if_allocate_capture_dma_engine(
    PVOID context, UCHAR codec, PHDAUDIO_STREAM_FORMAT format,
    PHANDLE handle, PHDAUDIO_CONVERTER_FORMAT converter) {
  (void)codec;
  return allocate_engine(context, FALSE, format, handle, converter);
}

static NTSTATUS NTAPI if_allocate_render_dma_engine(
    PVOID context, PHDAUDIO_STREAM_FORMAT format, BOOLEAN stripe,
    PHANDLE handle, PHDAUDIO_CONVERTER_FORMAT converter) {
  if (stripe)
    return STATUS_NOT_SUPPORTED;
  return allocate_engine(context, TRUE, format, handle, converter);
}

static NTSTATUS NTAPI if_change_bandwidth_allocation(
    PVOID context, HANDLE handle, PHDAUDIO_STREAM_FORMAT format,
    PHDAUDIO_CONVERTER_FORMAT converter) {
  PDO *p = context;
  HDA_ENGINE *e;
  ENGINE *x = engine_of(p->Fdo, p, handle, &e);
  USHORT fmt;
  if (!x || !format || !converter)
    return STATUS_INVALID_HANDLE;
  fmt = hda_format(format->SampleRate, format->ValidBitsPerSample,
                   format->NumberOfChannels);
  if (fmt == 0xFFFF)
    return STATUS_INVALID_PARAMETER;
  if (hda_rd32(&p->Fdo->Hda, HDA_SD(e->Index)) & HDA_SD_RUN)
    return STATUS_INVALID_DEVICE_REQUEST;
  e->Fmt = fmt;
  hda_wr16(&p->Fdo->Hda, HDA_SD(e->Index) + HDA_SD_FMT, fmt);
  converter->ConverterFormat = fmt;
  return STATUS_SUCCESS;
}

static void free_buffer(FDO *f, HDA_ENGINE *e, ENGINE *x) {
  hda_engine_reset(&f->Hda, e);
  e->Size = 0;
  x->Isr = NULL;
  if (x->Mdl) {
    IoFreeMdl(x->Mdl);
    x->Mdl = NULL;
  }
  if (x->Va) {
    f->Dma->DmaOperations->FreeCommonBuffer(f->Dma, x->Size, x->Pa, x->Va,
                                            FALSE);
    x->Va = NULL;
  }
  if (x->BdlVa) {
    f->Dma->DmaOperations->FreeCommonBuffer(f->Dma, PAGE_SIZE, x->BdlPa,
                                            x->BdlVa, FALSE);
    x->BdlVa = NULL;
  }
  x->Contiguous = FALSE;
}

/* The buffer, physically contiguous, in BDL entries of a page; its size
   rounded down to 128 bytes. */
static NTSTATUS NTAPI if_allocate_dma_buffer(PVOID context, HANDLE handle,
                                             SIZE_T size, PMDL *mdl,
                                             PSIZE_T allocated,
                                             PUCHAR stream, PULONG fifo) {
  PDO *p = context;
  FDO *f = p->Fdo;
  HDA_ENGINE *e;
  ENGINE *x = engine_of(f, p, handle, &e);
  if (!x || !mdl || !allocated || !stream || !fifo)
    return STATUS_INVALID_PARAMETER;
  if (x->Va)
    return STATUS_INVALID_DEVICE_REQUEST;
  size &= ~(SIZE_T)127;
  if (size < 256 || size > 0x100000)
    return STATUS_INVALID_PARAMETER;
  x->Va = f->Dma->DmaOperations->AllocateCommonBuffer(f->Dma, (ULONG)size,
                                                      &x->Pa, FALSE);
  if (!x->Va)
    return STATUS_INSUFFICIENT_RESOURCES;
  x->Size = (ULONG)size;
  RtlZeroMemory(x->Va, x->Size);
  x->Mdl = IoAllocateMdl(x->Va, x->Size, FALSE, FALSE, NULL);
  if (!x->Mdl) {
    free_buffer(f, e, x);
    return STATUS_INSUFFICIENT_RESOURCES;
  }
  MmBuildMdlForNonPagedPool(x->Mdl);
  hda_engine_setup(&f->Hda, e, x->Pa.LowPart, x->Size, PAGE_SIZE, FALSE,
                   e->Fmt);
  *mdl = x->Mdl;
  *allocated = x->Size;
  *stream = e->Tag;
  *fifo = hda_rd16(&f->Hda, HDA_SD(e->Index) + HDA_SD_FIFOS);
  return STATUS_SUCCESS;
}

static NTSTATUS NTAPI if_free_dma_buffer(PVOID context, HANDLE handle) {
  PDO *p = context;
  HDA_ENGINE *e;
  ENGINE *x = engine_of(p->Fdo, p, handle, &e);
  if (!x)
    return STATUS_INVALID_HANDLE;
  free_buffer(p->Fdo, e, x);
  return STATUS_SUCCESS;
}

static NTSTATUS NTAPI if_free_dma_engine(PVOID context, HANDLE handle) {
  PDO *p = context;
  HDA_ENGINE *e;
  ENGINE *x = engine_of(p->Fdo, p, handle, &e);
  if (!x)
    return STATUS_INVALID_HANDLE;
  if (x->Va)
    return STATUS_INVALID_DEVICE_REQUEST; /* free the buffer first */
  hda_engine_free(&p->Fdo->Hda, e);
  x->Owner = NULL;
  return STATUS_SUCCESS;
}

/* Put an engine back as its buffer left it, after a reset. */
static void reprogram(FDO *f, HDA_ENGINE *e, ENGINE *x) {
  if (x->Contiguous && x->Cbl)
    hda_engine_program(&f->Hda, e, x->BdlPa.LowPart, x->Cbl, x->Lvi,
                       x->Isr != NULL, e->Fmt);
  else if (x->Va)
    hda_engine_setup(&f->Hda, e, x->Pa.LowPart, x->Size, PAGE_SIZE, FALSE,
                     e->Fmt);
}

static NTSTATUS NTAPI if_set_dma_engine_state(PVOID context,
                                              HDAUDIO_STREAM_STATE state,
                                              ULONG count, PHANDLE handles) {
  PDO *p = context;
  FDO *f = p->Fdo;
  HDA_ENGINE *e;
  ENGINE *x;
  for (ULONG i = 0; i < count; i++)
    if (!engine_of(f, p, handles[i], &e))
      return STATUS_INVALID_HANDLE;
  for (ULONG i = 0; i < count; i++) {
    x = engine_of(f, p, handles[i], &e);
    switch (state) {
    case RunState:
      if (!x->Va && !x->Cbl)
        return STATUS_INVALID_DEVICE_REQUEST;
      hda_engine_run(&f->Hda, e, TRUE);
      break;
    case StopState:
      hda_engine_run(&f->Hda, e, FALSE);
      break;
    case ResetState:
      hda_engine_reset(&f->Hda, e);
      reprogram(f, e, x);
      break;
    default:
      return STATUS_INVALID_PARAMETER;
    }
  }
  return STATUS_SUCCESS;
}

static VOID NTAPI if_get_wall_clock_register(PVOID context, PULONG *reg) {
  PDO *p = context;
  *reg = (PULONG)(p->Fdo->Hda.Regs + HDA_WALCLK);
}

static NTSTATUS NTAPI if_get_link_position_register(PVOID context,
                                                    HANDLE handle,
                                                    PULONG *reg) {
  PDO *p = context;
  HDA_ENGINE *e;
  if (!engine_of(p->Fdo, p, handle, &e))
    return STATUS_INVALID_HANDLE;
  *reg = (PULONG)(p->Fdo->Hda.Regs + HDA_SD(e->Index) + HDA_SD_LPIB);
  return STATUS_SUCCESS;
}

static NTSTATUS NTAPI if_register_event_callback(
    PVOID context, PHDAUDIO_UNSOLICITED_RESPONSE_CALLBACK routine,
    PVOID callback_context, PUCHAR tag) {
  PDO *p = context;
  FDO *f = p->Fdo;
  KIRQL irql;
  if (!routine || !tag)
    return STATUS_INVALID_PARAMETER;
  KeRaiseIrql(DISPATCH_LEVEL, &irql);
  for (UCHAR t = 1; t < 64; t++)
    if (!f->Events[t].Routine) {
      f->Events[t].Context = callback_context;
      f->Events[t].Owner = p;
      f->Events[t].Routine = routine;
      *tag = t;
      KeLowerIrql(irql);
      return STATUS_SUCCESS;
    }
  KeLowerIrql(irql);
  return STATUS_INSUFFICIENT_RESOURCES;
}

static NTSTATUS NTAPI if_unregister_event_callback(PVOID context,
                                                   UCHAR tag) {
  PDO *p = context;
  FDO *f = p->Fdo;
  KIRQL irql;
  if (tag == 0 || tag >= 64 || f->Events[tag].Owner != p)
    return STATUS_INVALID_PARAMETER;
  KeRaiseIrql(DISPATCH_LEVEL, &irql);
  f->Events[tag].Routine = NULL;
  f->Events[tag].Owner = NULL;
  KeLowerIrql(irql);
  return STATUS_SUCCESS;
}

static NTSTATUS NTAPI if_get_device_information(
    PVOID context, PHDAUDIO_DEVICE_INFORMATION info) {
  PDO *p = context;
  USHORT n = 0;
  if (!info || info->Size < sizeof *info)
    return STATUS_BUFFER_TOO_SMALL;
  for (ULONG c = p->Fdo->Hda.Codecs; c; c >>= 1)
    n += c & 1;
  info->DeviceVersion = p->Fdo->Hda.Version;
  info->DriverVersion = 0x0100;
  info->CodecsDetected = n;
  info->IsStripingSupported = FALSE;
  return STATUS_SUCCESS;
}

static VOID NTAPI if_get_resource_information(PVOID context, PUCHAR codec,
                                              PUCHAR start) {
  PDO *p = context;
  if (codec)
    *codec = p->Fn.Cad;
  if (start)
    *start = p->Fn.Nid;
}

/* The BDL interface: a data buffer and a BDL of up to 256 entries that
   the caller fills, and its own interrupt routine for the engine. */
static NTSTATUS NTAPI if_allocate_contiguous_dma_buffer(
    PVOID context, HANDLE handle, ULONG size, PVOID *data,
    PHDAUDIO_BUFFER_DESCRIPTOR *bdl) {
  PDO *p = context;
  FDO *f = p->Fdo;
  HDA_ENGINE *e;
  ENGINE *x = engine_of(f, p, handle, &e);
  if (!x || !data || !bdl || !size || size > 0x100000)
    return STATUS_INVALID_PARAMETER;
  if (x->Va)
    return STATUS_INVALID_DEVICE_REQUEST;
  x->Va = f->Dma->DmaOperations->AllocateCommonBuffer(f->Dma, size, &x->Pa,
                                                      FALSE);
  x->BdlVa = f->Dma->DmaOperations->AllocateCommonBuffer(f->Dma, PAGE_SIZE,
                                                         &x->BdlPa, FALSE);
  x->Size = size;
  x->Contiguous = TRUE;
  if (!x->Va || !x->BdlVa) {
    free_buffer(f, e, x);
    return STATUS_INSUFFICIENT_RESOURCES;
  }
  RtlZeroMemory(x->Va, size);
  RtlZeroMemory(x->BdlVa, PAGE_SIZE);
  /* The first entry comes filled with the whole buffer: 98's WDM has no
     MmGetPhysicalAddress, so this is how a caller there learns where the
     buffer is. One written for XP overwrites it. */
  ((PHDAUDIO_BUFFER_DESCRIPTOR)x->BdlVa)->Address = x->Pa;
  ((PHDAUDIO_BUFFER_DESCRIPTOR)x->BdlVa)->Length = size;
  *data = x->Va;
  *bdl = x->BdlVa;
  return STATUS_SUCCESS;
}

static NTSTATUS NTAPI if_setup_dma_engine_with_bdl(
    PVOID context, HANDLE handle, ULONG length, ULONG lvi,
    PHDAUDIO_BDL_ISR isr, PVOID isr_context, PUCHAR stream, PULONG fifo) {
  PDO *p = context;
  FDO *f = p->Fdo;
  HDA_ENGINE *e;
  ENGINE *x = engine_of(f, p, handle, &e);
  if (!x || !x->Contiguous || !stream || !fifo || lvi > 255 || !length)
    return STATUS_INVALID_PARAMETER;
  hda_engine_reset(&f->Hda, e);
  x->Cbl = length;
  x->Lvi = lvi;
  x->IsrContext = isr_context;
  x->Isr = isr;
  hda_engine_program(&f->Hda, e, x->BdlPa.LowPart, length, lvi, isr != NULL,
                     e->Fmt);
  *stream = e->Tag;
  *fifo = hda_rd16(&f->Hda, HDA_SD(e->Index) + HDA_SD_FIFOS);
  return STATUS_SUCCESS;
}

static NTSTATUS NTAPI if_free_contiguous_dma_buffer(PVOID context,
                                                    HANDLE handle) {
  return if_free_dma_buffer(context, handle);
}

static NTSTATUS query_interface(PDO *p, PIO_STACK_LOCATION s) {
  const GUID *g = s->Parameters.QueryInterface.InterfaceType;
  if (IsEqualGUID(g, &GUID_HDAUDIO_BUS_INTERFACE)) {
    PHDAUDIO_BUS_INTERFACE i =
        (PHDAUDIO_BUS_INTERFACE)s->Parameters.QueryInterface.Interface;
    if (s->Parameters.QueryInterface.Size < sizeof *i)
      return STATUS_INVALID_PARAMETER;
    i->Size = sizeof *i;
    i->Version = 0x0100;
    i->Context = p;
    i->InterfaceReference = if_reference;
    i->InterfaceDereference = if_dereference;
    i->TransferCodecVerbs = if_transfer_codec_verbs;
    i->AllocateCaptureDmaEngine = if_allocate_capture_dma_engine;
    i->AllocateRenderDmaEngine = if_allocate_render_dma_engine;
    i->ChangeBandwidthAllocation = if_change_bandwidth_allocation;
    i->AllocateDmaBuffer = if_allocate_dma_buffer;
    i->FreeDmaBuffer = if_free_dma_buffer;
    i->FreeDmaEngine = if_free_dma_engine;
    i->SetDmaEngineState = if_set_dma_engine_state;
    i->GetWallClockRegister = if_get_wall_clock_register;
    i->GetLinkPositionRegister = if_get_link_position_register;
    i->RegisterEventCallback = if_register_event_callback;
    i->UnregisterEventCallback = if_unregister_event_callback;
    i->GetDeviceInformation = if_get_device_information;
    i->GetResourceInformation = if_get_resource_information;
    if_reference(p);
    return STATUS_SUCCESS;
  }
  if (IsEqualGUID(g, &GUID_HDAUDIO_BUS_INTERFACE_BDL)) {
    PHDAUDIO_BUS_INTERFACE_BDL i =
        (PHDAUDIO_BUS_INTERFACE_BDL)s->Parameters.QueryInterface.Interface;
    if (s->Parameters.QueryInterface.Size < sizeof *i)
      return STATUS_INVALID_PARAMETER;
    i->Size = sizeof *i;
    i->Version = 0x0100;
    i->Context = p;
    i->InterfaceReference = if_reference;
    i->InterfaceDereference = if_dereference;
    i->TransferCodecVerbs = if_transfer_codec_verbs;
    i->AllocateCaptureDmaEngine = if_allocate_capture_dma_engine;
    i->AllocateRenderDmaEngine = if_allocate_render_dma_engine;
    i->ChangeBandwidthAllocation = if_change_bandwidth_allocation;
    i->AllocateContiguousDmaBuffer = if_allocate_contiguous_dma_buffer;
    i->SetupDmaEngineWithBdl = if_setup_dma_engine_with_bdl;
    i->FreeContiguousDmaBuffer = if_free_contiguous_dma_buffer;
    i->FreeDmaEngine = if_free_dma_engine;
    i->SetDmaEngineState = if_set_dma_engine_state;
    i->GetWallClockRegister = if_get_wall_clock_register;
    i->GetLinkPositionRegister = if_get_link_position_register;
    i->RegisterEventCallback = if_register_event_callback;
    i->UnregisterEventCallback = if_unregister_event_callback;
    i->GetDeviceInformation = if_get_device_information;
    i->GetResourceInformation = if_get_resource_information;
    if_reference(p);
    return STATUS_SUCCESS;
  }
  return STATUS_NOT_SUPPORTED;
}

/* ---- Interrupts and unsolicited responses ---- */

static BOOLEAN NTAPI isr(PKINTERRUPT interrupt, PVOID context) {
  FDO *f = context;
  UCHAR status[HDA_MAX_ENGINES];
  ULONG bits;
  (void)interrupt;
  bits = hda_interrupt(&f->Hda, status);
  if (!bits)
    return FALSE;
  for (ULONG i = 0; i < HDA_MAX_ENGINES; i++)
    if ((bits & (1u << i)) && f->Engines[i].Isr)
      f->Engines[i].Isr(f->Engines[i].IsrContext, status[i]);
  return TRUE;
}

static VOID NTAPI unsol_dpc(PKDPC dpc, PVOID context, PVOID a, PVOID b) {
  FDO *f = context;
  ULONG resp, cad;
  (void)dpc, (void)a, (void)b;
  while (hda_unsolicited(&f->Hda, &resp, &cad)) {
    UCHAR tag = (UCHAR)(resp >> 26);
    EVENT *ev = &f->Events[tag];
    if (tag && ev->Routine && ev->Owner && ev->Owner->Fn.Cad == cad) {
      HDAUDIO_CODEC_RESPONSE r;
      r.CompleteResponse = 0;
      r.Response = resp;
      r.SDataIn = cad;
      r.IsUnsolicitedResponse = 1;
      r.IsValid = 1;
      ev->Routine(r, ev->Context);
    }
  }
}

/* ---- The children ---- */

static PDO *new_child(FDO *f, const HDA_FUNCTION *fn) {
  WCHAR name[40];
  UNICODE_STRING us;
  PDEVICE_OBJECT dev;
  PDO *p;
  ULONG n = wput(name, 0, L"\\Device\\HdAudioFn");
  n = wdec(name, n, f->Instance);
  n = wput(name, n, L"_");
  n = wdec(name, n, fn->Cad);
  n = wput(name, n, L"_");
  wdec(name, n, fn->Nid);
  RtlInitUnicodeString(&us, name);
  if (!NT_SUCCESS(IoCreateDevice(f->Self->DriverObject, sizeof(PDO), &us,
                                 FILE_DEVICE_SOUND, 0, FALSE, &dev)))
    return NULL;
  p = dev->DeviceExtension;
  RtlZeroMemory(p, sizeof *p);
  p->Self = dev;
  p->Fdo = f;
  p->Fn = *fn;
  p->Present = TRUE;
  dev->Flags |= DO_POWER_PAGABLE;
  dev->Flags &= ~DO_DEVICE_INITIALIZING;
  return p;
}

/* A MULTI_SZ (or, with multi FALSE, a string) in paged pool, which the
   PnP manager frees. */
static PWCHAR pool_string(const WCHAR *s, ULONG chars) {
  PWCHAR p = ExAllocatePoolWithTag(PagedPool, chars * sizeof(WCHAR), TAG);
  if (p)
    RtlCopyMemory(p, s, chars * sizeof(WCHAR));
  return p;
}

/* HDAUDIO\FUNC_ff&VEN_vvvv&DEV_dddd, then the parts parts says. */
static ULONG hwid(PWCHAR b, ULONG n, const HDA_FUNCTION *fn, BOOLEAN subsys,
                  BOOLEAN rev) {
  n = wput(b, n, L"HDAUDIO\\FUNC_");
  n = whex(b, n, fn->Type & 0xFF, 2);
  n = wput(b, n, L"&VEN_");
  n = whex(b, n, fn->Vendor >> 16, 4);
  n = wput(b, n, L"&DEV_");
  n = whex(b, n, fn->Vendor & 0xFFFF, 4);
  if (subsys) {
    n = wput(b, n, L"&SUBSYS_");
    n = whex(b, n, fn->Subsystem, 8);
  }
  if (rev) {
    n = wput(b, n, L"&REV_");
    n = whex(b, n, (fn->Revision >> 8) & 0xFFFF, 4);
  }
  return n;
}

static NTSTATUS query_id(PDO *p, PIRP irp, BUS_QUERY_ID_TYPE type) {
  WCHAR buf[512];
  ULONG n = 0;
  const HDA_FUNCTION *fn = &p->Fn;
  switch (type) {
  case BusQueryDeviceID:
    n = hwid(buf, 0, fn, TRUE, TRUE) + 1;
    break;
  case BusQueryHardwareIDs:
    n = hwid(buf, n, fn, TRUE, TRUE) + 1;
    n = hwid(buf, n, fn, TRUE, FALSE) + 1;
    n = hwid(buf, n, fn, FALSE, TRUE) + 1;
    n = hwid(buf, n, fn, FALSE, FALSE) + 1;
    n = wput(buf, n, L"HDAUDIO\\FUNC_");
    n = whex(buf, n, fn->Type & 0xFF, 2);
    n = wput(buf, n, L"&VEN_");
    n = whex(buf, n, fn->Vendor >> 16, 4) + 1;
    buf[n++] = 0;
    break;
  case BusQueryCompatibleIDs:
    n = wput(buf, 0, L"HDAUDIO\\FUNC_");
    n = whex(buf, n, fn->Type & 0xFF, 2) + 1;
    buf[n++] = 0;
    break;
  case BusQueryInstanceID:
    n = wdec(buf, 0, p->Fdo->Instance);
    n = wput(buf, n, L"&");
    n = whex(buf, n, fn->Cad, 2);
    n = whex(buf, n, fn->Nid, 2) + 1;
    break;
  default:
    return complete(irp, irp->IoStatus.Status, irp->IoStatus.Information);
  }
  PWCHAR s = pool_string(buf, n);
  if (!s)
    return complete(irp, STATUS_INSUFFICIENT_RESOURCES, 0);
  return complete(irp, STATUS_SUCCESS, (ULONG_PTR)s);
}

static NTSTATUS query_text(PDO *p, PIRP irp, PIO_STACK_LOCATION s) {
  WCHAR buf[96];
  ULONG n;
  if (s->Parameters.QueryDeviceText.DeviceTextType ==
      DeviceTextDescription) {
    n = wput(buf, 0, L"High Definition Audio ");
    n = wput(buf, n, (p->Fn.Type & 0xFF) == HDA_FG_AUDIO   ? L"audio"
                     : (p->Fn.Type & 0xFF) == HDA_FG_MODEM ? L"modem"
                                                           : L"vendor");
    n = wput(buf, n, L" function (codec ");
    n = wdec(buf, n, p->Fn.Cad);
    n = wput(buf, n, L")") + 1;
  } else if (s->Parameters.QueryDeviceText.DeviceTextType ==
             DeviceTextLocationInformation) {
    n = wput(buf, 0, L"Codec ");
    n = wdec(buf, n, p->Fn.Cad);
    n = wput(buf, n, L", node ");
    n = wdec(buf, n, p->Fn.Nid) + 1;
  } else
    return complete(irp, irp->IoStatus.Status, irp->IoStatus.Information);
  PWCHAR t = pool_string(buf, n);
  return t ? complete(irp, STATUS_SUCCESS, (ULONG_PTR)t)
           : complete(irp, STATUS_INSUFFICIENT_RESOURCES, 0);
}

static NTSTATUS pdo_pnp(PDO *p, PIRP irp) {
  PIO_STACK_LOCATION s = IoGetCurrentIrpStackLocation(irp);
  switch (s->MinorFunction) {
  case IRP_MN_START_DEVICE:
  case IRP_MN_QUERY_STOP_DEVICE:
  case IRP_MN_CANCEL_STOP_DEVICE:
  case IRP_MN_STOP_DEVICE:
  case IRP_MN_QUERY_REMOVE_DEVICE:
  case IRP_MN_CANCEL_REMOVE_DEVICE:
  case IRP_MN_SURPRISE_REMOVAL:
    return complete(irp, STATUS_SUCCESS, 0);
  case IRP_MN_REMOVE_DEVICE:
    /* Only gone for good once the bus no longer reports it. */
    if (!p->Present) {
      complete(irp, STATUS_SUCCESS, 0);
      IoDeleteDevice(p->Self);
      return STATUS_SUCCESS;
    }
    return complete(irp, STATUS_SUCCESS, 0);
  case IRP_MN_QUERY_ID:
    return query_id(p, irp, s->Parameters.QueryId.IdType);
  case IRP_MN_QUERY_DEVICE_TEXT:
    return query_text(p, irp, s);
  case IRP_MN_QUERY_INTERFACE:
    {
      NTSTATUS st = query_interface(p, s);
      if (st == STATUS_NOT_SUPPORTED)
        return complete(irp, irp->IoStatus.Status, irp->IoStatus.Information);
      return complete(irp, st, 0);
    }
  case IRP_MN_QUERY_CAPABILITIES: {
    PDEVICE_CAPABILITIES c = s->Parameters.DeviceCapabilities.Capabilities;
    c->Address = (ULONG)p->Fn.Cad << 8 | p->Fn.Nid;
    c->UINumber = p->Fn.Cad;
    c->UniqueID = FALSE;
    c->SurpriseRemovalOK = TRUE;
    c->DeviceState[PowerSystemWorking] = PowerDeviceD0;
    for (int i = PowerSystemSleeping1; i < PowerSystemMaximum; i++)
      c->DeviceState[i] = PowerDeviceD3;
    return complete(irp, STATUS_SUCCESS, 0);
  }
  case IRP_MN_QUERY_DEVICE_RELATIONS:
    if (s->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation) {
      PDEVICE_RELATIONS r =
          ExAllocatePoolWithTag(PagedPool, sizeof(DEVICE_RELATIONS), TAG);
      if (!r)
        return complete(irp, STATUS_INSUFFICIENT_RESOURCES, 0);
      r->Count = 1;
      r->Objects[0] = p->Self;
      ObReferenceObject(p->Self);
      return complete(irp, STATUS_SUCCESS, (ULONG_PTR)r);
    }
    return complete(irp, irp->IoStatus.Status, irp->IoStatus.Information);
  case IRP_MN_QUERY_BUS_INFORMATION: {
    PPNP_BUS_INFORMATION b =
        ExAllocatePoolWithTag(PagedPool, sizeof(PNP_BUS_INFORMATION), TAG);
    if (!b)
      return complete(irp, STATUS_INSUFFICIENT_RESOURCES, 0);
    b->BusTypeGuid = GUID_HDAUDIO_BUS_CLASS;
    b->LegacyBusType = PNPBus;
    b->BusNumber = p->Fdo->Instance;
    return complete(irp, STATUS_SUCCESS, (ULONG_PTR)b);
  }
  default:
    return complete(irp, irp->IoStatus.Status, irp->IoStatus.Information);
  }
}

/* ---- The controller (FDO) ---- */

static void stop_controller(FDO *f) {
  if (!f->Started)
    return;
  f->Started = FALSE;
  KeCancelTimer(&f->Timer);
  for (ULONG i = 0; i < HDA_MAX_ENGINES; i++)
    if (f->Hda.Engines[i].Used) {
      free_buffer(f, &f->Hda.Engines[i], &f->Engines[i]);
      hda_engine_free(&f->Hda, &f->Hda.Engines[i]);
      f->Engines[i].Owner = NULL;
    }
  hda_shutdown(&f->Hda);
  if (f->Interrupt) {
    IoDisconnectInterrupt(f->Interrupt);
    f->Interrupt = NULL;
  }
  if (f->Hda.Rings) {
    f->Dma->DmaOperations->FreeCommonBuffer(f->Dma, HDA_RINGS_SIZE,
                                            f->RingsPa, f->Hda.Rings, FALSE);
    f->Hda.Rings = NULL;
  }
  if (f->Dma) {
    f->Dma->DmaOperations->PutDmaAdapter(f->Dma);
    f->Dma = NULL;
  }
  if (f->Hda.Regs) {
    MmUnmapIoSpace((PVOID)f->Hda.Regs, f->Hda.RegsLength);
    f->Hda.Regs = NULL;
  }
}

static NTSTATUS start_controller(FDO *f, PCM_RESOURCE_LIST raw,
                                 PCM_RESOURCE_LIST list) {
  PCM_PARTIAL_RESOURCE_DESCRIPTOR mem = NULL, irq = NULL;
  DEVICE_DESCRIPTION dd;
  ULONG maps;
  NTSTATUS st;
  HDA_FUNCTION fns[MAX_CHILDREN];
  ULONG n = 0;
  (void)raw;
  if (!list || !list->Count)
    return STATUS_DEVICE_CONFIGURATION_ERROR;
  PCM_PARTIAL_RESOURCE_LIST l = &list->List[0].PartialResourceList;
  for (ULONG i = 0; i < l->Count; i++) {
    PCM_PARTIAL_RESOURCE_DESCRIPTOR d = &l->PartialDescriptors[i];
    if (d->Type == CmResourceTypeMemory && !mem)
      mem = d;
    else if (d->Type == CmResourceTypeInterrupt && !irq)
      irq = d;
  }
  if (!mem || !irq)
    return STATUS_DEVICE_CONFIGURATION_ERROR;
  f->Hda.RegsLength = mem->u.Memory.Length;
  f->Hda.Regs = MmMapIoSpace(mem->u.Memory.Start, mem->u.Memory.Length,
                             MmNonCached);
  if (!f->Hda.Regs)
    return STATUS_INSUFFICIENT_RESOURCES;

  RtlZeroMemory(&dd, sizeof dd);
  dd.Version = DEVICE_DESCRIPTION_VERSION;
  dd.Master = TRUE;
  dd.ScatterGather = TRUE;
  dd.Dma32BitAddresses = TRUE;
  dd.InterfaceType = PCIBus;
  dd.MaximumLength = 0x100000;
  f->Dma = IoGetDmaAdapter(f->Pdo, &dd, &maps);
  if (!f->Dma)
    return STATUS_INSUFFICIENT_RESOURCES;
  f->Hda.Rings = f->Dma->DmaOperations->AllocateCommonBuffer(
      f->Dma, HDA_RINGS_SIZE, &f->RingsPa, FALSE);
  if (!f->Hda.Rings)
    return STATUS_INSUFFICIENT_RESOURCES;
  f->Hda.RingsPhys = f->RingsPa.LowPart;
  RtlZeroMemory(f->Hda.Rings, HDA_RINGS_SIZE);

  st = hda_init(&f->Hda);
  if (!NT_SUCCESS(st))
    return st;
  f->Started = TRUE;
  st = IoConnectInterrupt(&f->Interrupt, isr, f, NULL,
                          irq->u.Interrupt.Vector,
                          (KIRQL)irq->u.Interrupt.Level,
                          (KIRQL)irq->u.Interrupt.Level,
                          irq->Flags & CM_RESOURCE_INTERRUPT_LATCHED
                              ? Latched
                              : LevelSensitive,
                          irq->ShareDisposition == CmResourceShareShared,
                          irq->u.Interrupt.Affinity, FALSE);
  if (!NT_SUCCESS(st)) {
    f->Interrupt = NULL;
    return st;
  }

  /* The children: every function group of every codec, kept across a
     stop and start when the same ones are found again. */
  for (ULONG cad = 0; cad < 15; cad++)
    if (f->Hda.Codecs & (1u << cad))
      n += codec_functions(bus_verb, &f->Hda, cad, fns + n,
                           MAX_CHILDREN - n);
  for (ULONG i = 0; i < f->ChildCount; i++) {
    PDO *p = f->Children[i];
    BOOLEAN found = FALSE;
    for (ULONG j = 0; j < n; j++)
      found |= fns[j].Cad == p->Fn.Cad && fns[j].Nid == p->Fn.Nid &&
               fns[j].Vendor == p->Fn.Vendor;
    p->Present = found;
  }
  for (ULONG j = 0; j < n; j++) {
    BOOLEAN have = FALSE;
    for (ULONG i = 0; i < f->ChildCount; i++)
      have |= f->Children[i]->Present && fns[j].Cad == f->Children[i]->Fn.Cad &&
              fns[j].Nid == f->Children[i]->Fn.Nid;
    if (!have && f->ChildCount < MAX_CHILDREN) {
      PDO *p = new_child(f, &fns[j]);
      if (p)
        f->Children[f->ChildCount++] = p;
    }
  }

  LARGE_INTEGER due;
  due.QuadPart = -10000LL * UNSOL_PERIOD_MS;
  KeSetTimerEx(&f->Timer, due, UNSOL_PERIOD_MS, &f->TimerDpc);
  IoInvalidateDeviceRelations(f->Pdo, BusRelations);
  return STATUS_SUCCESS;
}

/* The children the bus has now; the ones gone are dropped from the list
   (the PnP manager then removes them). */
static NTSTATUS bus_relations(FDO *f, PIRP irp) {
  PDEVICE_RELATIONS old = (PDEVICE_RELATIONS)irp->IoStatus.Information, r;
  ULONG keep = 0, prev = old ? old->Count : 0;
  for (ULONG i = 0; i < f->ChildCount; i++)
    keep += f->Children[i]->Present;
  r = ExAllocatePoolWithTag(PagedPool,
                            sizeof(DEVICE_RELATIONS) +
                                (prev + keep) * sizeof(PDEVICE_OBJECT),
                            TAG);
  if (!r)
    return STATUS_INSUFFICIENT_RESOURCES;
  r->Count = 0;
  for (ULONG i = 0; i < prev; i++)
    r->Objects[r->Count++] = old->Objects[i];
  for (ULONG i = 0; i < f->ChildCount; i++)
    if (f->Children[i]->Present) {
      r->Objects[r->Count++] = f->Children[i]->Self;
      ObReferenceObject(f->Children[i]->Self);
    }
  if (old)
    ExFreePool(old);
  /* Forget the ones gone; their remove IRP deletes them. */
  ULONG j = 0;
  for (ULONG i = 0; i < f->ChildCount; i++)
    if (f->Children[i]->Present)
      f->Children[j++] = f->Children[i];
  f->ChildCount = j;
  irp->IoStatus.Information = (ULONG_PTR)r;
  irp->IoStatus.Status = STATUS_SUCCESS;
  return STATUS_SUCCESS;
}

static NTSTATUS fdo_pnp(FDO *f, PIRP irp) {
  PIO_STACK_LOCATION s = IoGetCurrentIrpStackLocation(irp);
  NTSTATUS st;
  switch (s->MinorFunction) {
  case IRP_MN_START_DEVICE:
    st = forward_and_wait(f, irp);
    if (NT_SUCCESS(st)) {
      st = start_controller(
          f, s->Parameters.StartDevice.AllocatedResources,
          s->Parameters.StartDevice.AllocatedResourcesTranslated);
      if (!NT_SUCCESS(st))
        stop_controller(f);
    }
    return complete(irp, st, 0);
  case IRP_MN_QUERY_DEVICE_RELATIONS:
    if (s->Parameters.QueryDeviceRelations.Type == BusRelations) {
      st = bus_relations(f, irp);
      if (!NT_SUCCESS(st))
        return complete(irp, st, 0);
    }
    return pass_down(f, irp);
  case IRP_MN_STOP_DEVICE:
  case IRP_MN_SURPRISE_REMOVAL:
    stop_controller(f);
    irp->IoStatus.Status = STATUS_SUCCESS;
    return pass_down(f, irp);
  case IRP_MN_QUERY_STOP_DEVICE:
  case IRP_MN_CANCEL_STOP_DEVICE:
  case IRP_MN_QUERY_REMOVE_DEVICE:
  case IRP_MN_CANCEL_REMOVE_DEVICE:
    irp->IoStatus.Status = STATUS_SUCCESS;
    return pass_down(f, irp);
  case IRP_MN_REMOVE_DEVICE: {
    PDEVICE_OBJECT lower = f->Lower;
    stop_controller(f);
    for (ULONG i = 0; i < f->ChildCount; i++)
      IoDeleteDevice(f->Children[i]->Self);
    f->ChildCount = 0;
    irp->IoStatus.Status = STATUS_SUCCESS;
    IoSkipCurrentIrpStackLocation(irp);
    st = IoCallDriver(lower, irp);
    IoDetachDevice(lower);
    IoDeleteDevice(f->Self);
    return st;
  }
  default:
    return pass_down(f, irp);
  }
}

/* ---- Dispatch ---- */

static NTSTATUS NTAPI dispatch_pnp(PDEVICE_OBJECT dev, PIRP irp) {
  COMMON *c = dev->DeviceExtension;
  return c->IsFdo ? fdo_pnp((FDO *)c, irp) : pdo_pnp((PDO *)c, irp);
}

static NTSTATUS NTAPI dispatch_power(PDEVICE_OBJECT dev, PIRP irp) {
  COMMON *c = dev->DeviceExtension;
  PIO_STACK_LOCATION s = IoGetCurrentIrpStackLocation(irp);
  PoStartNextPowerIrp(irp);
  if (c->IsFdo) {
    IoSkipCurrentIrpStackLocation(irp);
    return PoCallDriver(((FDO *)c)->Lower, irp);
  }
  if (s->MinorFunction == IRP_MN_SET_POWER ||
      s->MinorFunction == IRP_MN_QUERY_POWER)
    return complete(irp, STATUS_SUCCESS, 0);
  return complete(irp, irp->IoStatus.Status, irp->IoStatus.Information);
}

static NTSTATUS NTAPI dispatch_other(PDEVICE_OBJECT dev, PIRP irp) {
  COMMON *c = dev->DeviceExtension;
  if (c->IsFdo)
    return pass_down((FDO *)c, irp);
  return complete(irp, irp->IoStatus.Status, irp->IoStatus.Information);
}

static NTSTATUS NTAPI dispatch_create_close(PDEVICE_OBJECT dev, PIRP irp) {
  (void)dev;
  return complete(irp, STATUS_SUCCESS, 0);
}

static NTSTATUS NTAPI add_device(PDRIVER_OBJECT driver, PDEVICE_OBJECT pdo) {
  PDEVICE_OBJECT dev;
  FDO *f;
  NTSTATUS st = IoCreateDevice(driver, sizeof(FDO), NULL, FILE_DEVICE_BUS_EXTENDER,
                               0, FALSE, &dev);
  if (!NT_SUCCESS(st))
    return st;
  f = dev->DeviceExtension;
  RtlZeroMemory(f, sizeof *f);
  f->Common.IsFdo = TRUE;
  f->Self = dev;
  f->Pdo = pdo;
  f->Instance = instances++;
  KeInitializeTimerEx(&f->Timer, NotificationTimer);
  KeInitializeDpc(&f->TimerDpc, unsol_dpc, f);
  f->Lower = IoAttachDeviceToDeviceStack(dev, pdo);
  if (!f->Lower) {
    IoDeleteDevice(dev);
    return STATUS_NO_SUCH_DEVICE;
  }
  dev->Flags |= DO_POWER_PAGABLE;
  dev->Flags &= ~DO_DEVICE_INITIALIZING;
  return STATUS_SUCCESS;
}

static VOID NTAPI unload(PDRIVER_OBJECT driver) { (void)driver; }

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING path) {
  (void)path;
  for (ULONG i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
    driver->MajorFunction[i] = dispatch_other;
  driver->MajorFunction[IRP_MJ_CREATE] = dispatch_create_close;
  driver->MajorFunction[IRP_MJ_CLOSE] = dispatch_create_close;
  driver->MajorFunction[IRP_MJ_PNP] = dispatch_pnp;
  driver->MajorFunction[IRP_MJ_POWER] = dispatch_power;
  driver->DriverExtension->AddDevice = add_device;
  driver->DriverUnload = unload;
  return STATUS_SUCCESS;
}
