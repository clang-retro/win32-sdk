/* An HD Audio function driver (<hdaudio.h>) for Windows XP and later: it
   attaches to the codec's function group, which Microsoft's HD Audio bus
   driver (HDAUDBUS.SYS) enumerates. When the device starts, it asks the
   bus for GUID_HDAUDIO_BUS_INTERFACE with IRP_MN_QUERY_INTERFACE, then
   reads the controller's information and sends the codec one verb, Get
   Parameter (0xF00) Vendor ID, to the function group's node.
   run-tests.sh builds it with winxp-ddk.cfg and checks its imports for
   XP (check-imports.py --nt 5.1); the static asserts hold the layouts
   to what Microsoft documents for 32-bit and 64-bit drivers. */
#include <initguid.h>
#include <ntddk.h>
#include <hdaudio.h>

_Static_assert(sizeof(HDAUDIO_CODEC_COMMAND) == 4, "command");
_Static_assert(sizeof(HDAUDIO_CODEC_RESPONSE) == 8, "response");
_Static_assert(sizeof(HDAUDIO_CODEC_TRANSFER) == 16, "transfer");
_Static_assert(sizeof(HDAUDIO_CONVERTER_FORMAT) == 2, "converter format");
_Static_assert(sizeof(HDAUDIO_STREAM_FORMAT) == 12, "stream format");
_Static_assert(sizeof(HDAUDIO_DEVICE_INFORMATION) == 10, "device info");
_Static_assert(sizeof(HDAUDIO_BUFFER_DESCRIPTOR) == 16, "BDL entry");
_Static_assert(sizeof(HDAUDIO_BUS_INTERFACE) == 18 * sizeof(PVOID), "interface");
_Static_assert(sizeof(HDAUDIO_BUS_INTERFACE_BDL) == 19 * sizeof(PVOID), "BDL interface");

typedef struct {
  PDEVICE_OBJECT lower;
  HDAUDIO_BUS_INTERFACE bus;
  ULONG vendor;
} EXT;

static NTSTATUS NTAPI Done(PDEVICE_OBJECT dev, PIRP irp, PVOID event) {
  (void)dev;
  (void)irp;
  KeSetEvent((PKEVENT)event, IO_NO_INCREMENT, FALSE);
  return STATUS_MORE_PROCESSING_REQUIRED;
}

/* Send IRP down and wait for it. */
static NTSTATUS Down(EXT *x, PIRP irp) {
  KEVENT event;
  KeInitializeEvent(&event, NotificationEvent, FALSE);
  IoCopyCurrentIrpStackLocationToNext(irp);
  IoSetCompletionRoutine(irp, Done, &event, TRUE, TRUE, TRUE);
  if (IoCallDriver(x->lower, irp) == STATUS_PENDING)
    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
  return irp->IoStatus.Status;
}

static NTSTATUS QueryBus(EXT *x) {
  KEVENT event;
  IO_STATUS_BLOCK io;
  PIRP irp;
  PIO_STACK_LOCATION s;
  NTSTATUS status;
  KeInitializeEvent(&event, NotificationEvent, FALSE);
  irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP, x->lower, NULL, 0, NULL,
                                     &event, &io);
  if (!irp)
    return STATUS_INSUFFICIENT_RESOURCES;
  irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
  s = IoGetNextIrpStackLocation(irp);
  s->MinorFunction = IRP_MN_QUERY_INTERFACE;
  s->Parameters.QueryInterface.InterfaceType = &GUID_HDAUDIO_BUS_INTERFACE;
  s->Parameters.QueryInterface.Size = sizeof x->bus;
  s->Parameters.QueryInterface.Version = 0x0100;
  s->Parameters.QueryInterface.Interface = (PINTERFACE)&x->bus;
  s->Parameters.QueryInterface.InterfaceSpecificData = NULL;
  status = IoCallDriver(x->lower, irp);
  if (status == STATUS_PENDING) {
    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
    status = io.Status;
  }
  return status;
}

static NTSTATUS Start(EXT *x) {
  HDAUDIO_DEVICE_INFORMATION info = {sizeof info, 0, 0, 0, 0};
  HDAUDIO_CODEC_TRANSFER verb;
  UCHAR codec, node;
  NTSTATUS status;
  RtlZeroMemory(&verb, sizeof verb);
  status = QueryBus(x);
  if (!NT_SUCCESS(status))
    return status;
  status = x->bus.GetDeviceInformation(x->bus.Context, &info);
  if (NT_SUCCESS(status)) {
    x->bus.GetResourceInformation(x->bus.Context, &codec, &node);
    verb.Output.Verb8.CodecAddress = codec;
    verb.Output.Verb8.Node = node;
    verb.Output.Verb8.VerbId = 0xF00;
    verb.Output.Verb8.Data = 0; /* Vendor ID */
    status = x->bus.TransferCodecVerbs(x->bus.Context, 1, &verb, NULL, NULL);
    if (NT_SUCCESS(status) && verb.Input.IsValid)
      x->vendor = verb.Input.Response;
  }
  x->bus.InterfaceDereference(x->bus.Context);
  return status;
}

static NTSTATUS NTAPI Pnp(PDEVICE_OBJECT dev, PIRP irp) {
  EXT *x = (EXT *)dev->DeviceExtension;
  PIO_STACK_LOCATION s = IoGetCurrentIrpStackLocation(irp);
  NTSTATUS status;
  switch (s->MinorFunction) {
  case IRP_MN_START_DEVICE:
    status = Down(x, irp);
    if (NT_SUCCESS(status))
      status = Start(x);
    irp->IoStatus.Status = status;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return status;
  case IRP_MN_REMOVE_DEVICE:
    IoSkipCurrentIrpStackLocation(irp);
    status = IoCallDriver(x->lower, irp);
    IoDetachDevice(x->lower);
    IoDeleteDevice(dev);
    return status;
  }
  IoSkipCurrentIrpStackLocation(irp);
  return IoCallDriver(x->lower, irp);
}

static NTSTATUS NTAPI Power(PDEVICE_OBJECT dev, PIRP irp) {
  EXT *x = (EXT *)dev->DeviceExtension;
  PoStartNextPowerIrp(irp);
  IoSkipCurrentIrpStackLocation(irp);
  return PoCallDriver(x->lower, irp);
}

static NTSTATUS NTAPI AddDevice(PDRIVER_OBJECT driver, PDEVICE_OBJECT pdo) {
  PDEVICE_OBJECT fdo;
  EXT *x;
  NTSTATUS status = IoCreateDevice(driver, sizeof(EXT), NULL,
                                   FILE_DEVICE_SOUND, 0, FALSE, &fdo);
  if (!NT_SUCCESS(status))
    return status;
  x = (EXT *)fdo->DeviceExtension;
  RtlZeroMemory(x, sizeof *x);
  x->lower = IoAttachDeviceToDeviceStack(fdo, pdo);
  if (!x->lower) {
    IoDeleteDevice(fdo);
    return STATUS_NO_SUCH_DEVICE;
  }
  fdo->Flags |= DO_POWER_PAGABLE;
  fdo->Flags &= ~DO_DEVICE_INITIALIZING;
  return STATUS_SUCCESS;
}

static VOID NTAPI Unload(PDRIVER_OBJECT driver) { (void)driver; }

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING path) {
  (void)path;
  driver->DriverExtension->AddDevice = AddDevice;
  driver->MajorFunction[IRP_MJ_PNP] = Pnp;
  driver->MajorFunction[IRP_MJ_POWER] = Power;
  driver->DriverUnload = Unload;
  return STATUS_SUCCESS;
}
