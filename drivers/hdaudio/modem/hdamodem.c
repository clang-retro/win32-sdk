/* hdamodem.sys: a skeleton modem function driver for the modem function
   group (HDAUDIO\FUNC_02) that hdaudbus.sys finds on a codec, for Windows
   98, 98 SE and ME (and XP's HDAUDBUS.SYS). It shows the handoff a soft
   modem's driver gets: it holds no hardware of its own and reaches the
   codec and the controller's DMA engines only through the bus driver's
   HDAUDIO_BUS_INTERFACE_BDL (<hdaudio.h>), the variant that lets it build
   its own buffer descriptor list and take each buffer's interrupt.

   On IRP_MN_START_DEVICE it gets the interface, reads the function
   group's vendor ID and modem capabilities, registers for the codec's
   unsolicited responses (a modem codec's ring and line events), and sets
   up a render and a capture engine at 8 kHz 16-bit mono, each with a
   cyclic buffer of two halves that interrupt. A real modem driver's data
   pump starts the engines (SetDmaEngineState RunState) and fills and
   drains the halves in Isr; this one stops at setting them up and logs
   what it got with DbgPrint. On stop and remove it gives it all back. */
#include <wdm.h>
#include <initguid.h>
#include <hdaudio.h>

#define TAG 'mDHc'
#define HALF 1024 /* bytes: 64 ms of 8 kHz 16-bit mono */

typedef struct {
  HANDLE Engine;
  PVOID Buffer;
  PHDAUDIO_BUFFER_DESCRIPTOR Bdl;
  UCHAR StreamId;
  volatile LONG Interrupts;
} ENGINE;

typedef struct {
  PDEVICE_OBJECT Self, Lower;
  HDAUDIO_BUS_INTERFACE_BDL Bus;
  BOOLEAN HaveBus;
  UCHAR Cad, Nid, EventTag;
  ENGINE Render, Capture;
  volatile LONG Events;
} MODEM;

static ULONG verb(MODEM *m, ULONG nid, ULONG v12, ULONG data) {
  HDAUDIO_CODEC_TRANSFER t;
  RtlZeroMemory(&t, sizeof t);
  t.Output.Command = (ULONG)m->Cad << 28 | nid << 20 | v12 << 8 | data;
  if (!NT_SUCCESS(m->Bus.TransferCodecVerbs(m->Bus.Context, 1, &t, NULL,
                                            NULL)) ||
      !t.Input.IsValid)
    return 0xFFFFFFFF;
  return t.Input.Response;
}

/* Each half of the buffer done: here the data pump moves the next one. */
static void NTAPI isr(PVOID context, ULONG status) {
  ENGINE *e = context;
  (void)status;
  InterlockedIncrement((PLONG)&e->Interrupts);
}

/* An unsolicited response from the codec: ring, line in use, ... */
static VOID NTAPI event(HDAUDIO_CODEC_RESPONSE r, PVOID context) {
  MODEM *m = context;
  InterlockedIncrement((PLONG)&m->Events);
  DbgPrint("hdamodem: event %08lx\n", (ULONG)r.Response);
}

static void engine_free(MODEM *m, ENGINE *e) {
  if (!e->Engine)
    return;
  m->Bus.SetDmaEngineState(m->Bus.Context, ResetState, 1, &e->Engine);
  if (e->Buffer)
    m->Bus.FreeContiguousDmaBuffer(m->Bus.Context, e->Engine);
  m->Bus.FreeDmaEngine(m->Bus.Context, e->Engine);
  RtlZeroMemory(e, sizeof *e);
}

static NTSTATUS engine_setup(MODEM *m, ENGINE *e, BOOLEAN render) {
  HDAUDIO_STREAM_FORMAT f = {8000, 16, 16, 1};
  HDAUDIO_CONVERTER_FORMAT conv;
  PHYSICAL_ADDRESS base;
  ULONG fifo;
  NTSTATUS st;
  st = render ? m->Bus.AllocateRenderDmaEngine(m->Bus.Context, &f, FALSE,
                                               &e->Engine, &conv)
              : m->Bus.AllocateCaptureDmaEngine(m->Bus.Context, m->Cad, &f,
                                                &e->Engine, &conv);
  if (!NT_SUCCESS(st)) {
    e->Engine = NULL;
    return st;
  }
  st = m->Bus.AllocateContiguousDmaBuffer(m->Bus.Context, e->Engine,
                                          2 * HALF, &e->Buffer, &e->Bdl);
  if (!NT_SUCCESS(st)) {
    e->Buffer = NULL;
    engine_free(m, e);
    return st;
  }
  RtlZeroMemory(e->Buffer, 2 * HALF);
  /* The buffer is physically contiguous: two entries, both interrupting.
     98's WDM has no MmGetPhysicalAddress; hdaudbus.sys hands the BDL over
     with its first entry filled with the whole buffer instead (XP's
     HDAUDBUS.SYS does not, and there MmGetPhysicalAddress(e->Buffer)
     gives it). */
  base = e->Bdl[0].Address;
  for (ULONG i = 0; i < 2; i++) {
    e->Bdl[i].Address.QuadPart = base.QuadPart + i * HALF;
    e->Bdl[i].Length = HALF;
    e->Bdl[i].InterruptOnCompletion = 1;
  }
  st = m->Bus.SetupDmaEngineWithBdl(m->Bus.Context, e->Engine, 2 * HALF, 1,
                                    isr, e, &e->StreamId, &fifo);
  if (!NT_SUCCESS(st)) {
    engine_free(m, e);
    return st;
  }
  DbgPrint("hdamodem: %s engine on stream %u, format %04x, FIFO %lu\n",
           render ? "render" : "capture", e->StreamId,
           conv.ConverterFormat, fifo);
  /* The codec's converters for the line go on e->StreamId here
     (SET_CONVERTER_STREAM_CHANNEL and SET_CONVERTER_FORMAT, verbs 706 and
     2), on the nodes the modem function group lists. */
  return STATUS_SUCCESS;
}

static void stop(MODEM *m) {
  if (!m->HaveBus)
    return;
  engine_free(m, &m->Render);
  engine_free(m, &m->Capture);
  if (m->EventTag)
    m->Bus.UnregisterEventCallback(m->Bus.Context, m->EventTag);
  m->EventTag = 0;
  m->Bus.InterfaceDereference(m->Bus.Context);
  m->HaveBus = FALSE;
}

/* IRP_MN_QUERY_INTERFACE for the BDL bus interface, down our stack. */
static NTSTATUS query_bus(MODEM *m) {
  IO_STATUS_BLOCK io;
  KEVENT done;
  PIO_STACK_LOCATION sp;
  NTSTATUS st;
  PIRP irp;
  KeInitializeEvent(&done, NotificationEvent, FALSE);
  irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP, m->Lower, NULL, 0, NULL,
                                     &done, &io);
  if (!irp)
    return STATUS_INSUFFICIENT_RESOURCES;
  irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
  sp = IoGetNextIrpStackLocation(irp);
  sp->MinorFunction = IRP_MN_QUERY_INTERFACE;
  sp->Parameters.QueryInterface.InterfaceType =
      &GUID_HDAUDIO_BUS_INTERFACE_BDL;
  sp->Parameters.QueryInterface.Size = sizeof m->Bus;
  sp->Parameters.QueryInterface.Version = 0x0100;
  sp->Parameters.QueryInterface.Interface = (PINTERFACE)&m->Bus;
  sp->Parameters.QueryInterface.InterfaceSpecificData = NULL;
  st = IoCallDriver(m->Lower, irp);
  if (st == STATUS_PENDING) {
    KeWaitForSingleObject(&done, Executive, KernelMode, FALSE, NULL);
    st = io.Status;
  }
  return st;
}

static NTSTATUS start(MODEM *m) {
  NTSTATUS st = query_bus(m);
  if (!NT_SUCCESS(st))
    return st;
  m->HaveBus = TRUE;
  m->Bus.GetResourceInformation(m->Bus.Context, &m->Cad, &m->Nid);
  DbgPrint("hdamodem: codec %u node %u, vendor %08lx, type %08lx\n", m->Cad,
           m->Nid, verb(m, 0, 0xF00, 0x00), verb(m, m->Nid, 0xF00, 0x05));
  st = m->Bus.RegisterEventCallback(m->Bus.Context, event, m, &m->EventTag);
  if (NT_SUCCESS(st))
    st = engine_setup(m, &m->Render, TRUE);
  if (NT_SUCCESS(st))
    st = engine_setup(m, &m->Capture, FALSE);
  if (!NT_SUCCESS(st))
    stop(m);
  return st;
}

static NTSTATUS NTAPI started(PDEVICE_OBJECT dev, PIRP irp, PVOID context) {
  (void)dev, (void)irp;
  KeSetEvent(context, IO_NO_INCREMENT, FALSE);
  return STATUS_MORE_PROCESSING_REQUIRED;
}

static NTSTATUS NTAPI pnp(PDEVICE_OBJECT dev, PIRP irp) {
  MODEM *m = dev->DeviceExtension;
  PIO_STACK_LOCATION sp = IoGetCurrentIrpStackLocation(irp);
  NTSTATUS st;
  KEVENT done;
  switch (sp->MinorFunction) {
  case IRP_MN_START_DEVICE:
    KeInitializeEvent(&done, NotificationEvent, FALSE);
    IoCopyCurrentIrpStackLocationToNext(irp);
    IoSetCompletionRoutine(irp, started, &done, TRUE, TRUE, TRUE);
    if (IoCallDriver(m->Lower, irp) == STATUS_PENDING)
      KeWaitForSingleObject(&done, Executive, KernelMode, FALSE, NULL);
    st = irp->IoStatus.Status;
    if (NT_SUCCESS(st))
      st = start(m);
    irp->IoStatus.Status = st;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return st;
  case IRP_MN_STOP_DEVICE:
  case IRP_MN_SURPRISE_REMOVAL:
    stop(m);
    break;
  case IRP_MN_REMOVE_DEVICE:
    stop(m);
    irp->IoStatus.Status = STATUS_SUCCESS;
    IoSkipCurrentIrpStackLocation(irp);
    st = IoCallDriver(m->Lower, irp);
    IoDetachDevice(m->Lower);
    IoDeleteDevice(dev);
    return st;
  case IRP_MN_QUERY_STOP_DEVICE:
  case IRP_MN_QUERY_REMOVE_DEVICE:
  case IRP_MN_CANCEL_STOP_DEVICE:
  case IRP_MN_CANCEL_REMOVE_DEVICE:
    irp->IoStatus.Status = STATUS_SUCCESS;
    break;
  }
  IoSkipCurrentIrpStackLocation(irp);
  return IoCallDriver(m->Lower, irp);
}

static NTSTATUS NTAPI power(PDEVICE_OBJECT dev, PIRP irp) {
  MODEM *m = dev->DeviceExtension;
  PoStartNextPowerIrp(irp);
  IoSkipCurrentIrpStackLocation(irp);
  return PoCallDriver(m->Lower, irp);
}

/* Create, close and the rest: the modem's own interface (a serial port,
   say) would go here. */
static NTSTATUS NTAPI other(PDEVICE_OBJECT dev, PIRP irp) {
  PIO_STACK_LOCATION sp = IoGetCurrentIrpStackLocation(irp);
  NTSTATUS st = sp->MajorFunction == IRP_MJ_CREATE ||
                        sp->MajorFunction == IRP_MJ_CLOSE
                    ? STATUS_SUCCESS
                    : STATUS_INVALID_DEVICE_REQUEST;
  (void)dev;
  irp->IoStatus.Status = st;
  irp->IoStatus.Information = 0;
  IoCompleteRequest(irp, IO_NO_INCREMENT);
  return st;
}

static NTSTATUS NTAPI add_device(PDRIVER_OBJECT driver, PDEVICE_OBJECT pdo) {
  PDEVICE_OBJECT dev;
  MODEM *m;
  NTSTATUS st = IoCreateDevice(driver, sizeof(MODEM), NULL,
                               FILE_DEVICE_MODEM, 0, FALSE, &dev);
  if (!NT_SUCCESS(st))
    return st;
  m = dev->DeviceExtension;
  RtlZeroMemory(m, sizeof *m);
  m->Self = dev;
  m->Lower = IoAttachDeviceToDeviceStack(dev, pdo);
  if (!m->Lower) {
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
    driver->MajorFunction[i] = other;
  driver->MajorFunction[IRP_MJ_PNP] = pnp;
  driver->MajorFunction[IRP_MJ_POWER] = power;
  driver->DriverExtension->AddDevice = add_device;
  driver->DriverUnload = unload;
  return STATUS_SUCCESS;
}
