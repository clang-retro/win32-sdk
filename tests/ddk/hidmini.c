/* A HID minidriver (a USB or other bus's HID device, under HIDCLASS.SYS)
   for Windows 98, 98 SE and ME (and 2000 and later), with <hidport.h>: it
   registers with HIDCLASS and answers its internal requests for the
   device attributes, and passes the rest down. run-tests.sh builds it with
   nt4-ddk.cfg and -lhidclass and checks with check-imports.py --wdm that
   98's HIDCLASS.SYS has its imports. */
#include <ntddk.h>
#include <hidport.h>

static NTSTATUS NTAPI InternalIoctl(PDEVICE_OBJECT dev, PIRP irp) {
  PHID_DEVICE_EXTENSION ext = dev->DeviceExtension;
  PIO_STACK_LOCATION s = IoGetCurrentIrpStackLocation(irp);
  if (s->Parameters.DeviceIoControl.IoControlCode ==
      IOCTL_HID_GET_DEVICE_ATTRIBUTES) {
    PHID_DEVICE_ATTRIBUTES a = irp->UserBuffer;
    if (s->Parameters.DeviceIoControl.OutputBufferLength < sizeof *a) {
      irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
    } else {
      RtlZeroMemory(a, sizeof *a);
      a->Size = sizeof *a;
      a->VendorID = 0x1234;
      a->ProductID = 0x5678;
      irp->IoStatus.Information = sizeof *a;
      irp->IoStatus.Status = STATUS_SUCCESS;
    }
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return irp->IoStatus.Status;
  }
  IoSkipCurrentIrpStackLocation(irp);
  return IoCallDriver(ext->NextDeviceObject, irp);
}

static NTSTATUS NTAPI AddDevice(PDRIVER_OBJECT driver, PDEVICE_OBJECT fdo) {
  (void)driver;
  fdo->Flags &= ~DO_DEVICE_INITIALIZING;
  return STATUS_SUCCESS;
}

static void NTAPI Unload(PDRIVER_OBJECT driver) { (void)driver; }

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING path) {
  HID_MINIDRIVER_REGISTRATION r;
  driver->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = InternalIoctl;
  driver->DriverExtension->AddDevice = AddDevice;
  driver->DriverUnload = Unload;
  RtlZeroMemory(&r, sizeof r);
  r.Revision = HID_REVISION;
  r.DriverObject = driver;
  r.RegistryPath = path;
  r.DeviceExtensionSize = 0;
  r.DevicesArePolled = FALSE;
  return HidRegisterMinidriver(&r);
}
