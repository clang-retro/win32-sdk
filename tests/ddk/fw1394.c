/* An IEEE 1394 (FireWire) client driver's request for Windows 98, 98 SE
   and ME (and 2000 and later), with <1394.h>: it asks the bus driver
   below it for the bus's generation count with an IRB
   (REQUEST_GET_GENERATION_COUNT) in an IOCTL_1394_CLASS request.
   1394 client drivers import nothing from 1394BUS.SYS; run-tests.sh
   builds it with nt4-ddk.cfg and checks with check-imports.py --wdm that
   its kernel imports are 98's. */
#include <ntddk.h>
#include <1394.h>

NTSTATUS generation(PDEVICE_OBJECT below, PULONG count);
NTSTATUS generation(PDEVICE_OBJECT below, PULONG count) {
  IRB irb;
  KEVENT done;
  IO_STATUS_BLOCK iosb;
  PIRP irp;
  NTSTATUS status;
  RtlZeroMemory(&irb, sizeof irb);
  irb.FunctionNumber = REQUEST_GET_GENERATION_COUNT;
  KeInitializeEvent(&done, NotificationEvent, FALSE);
  irp = IoBuildDeviceIoControlRequest(IOCTL_1394_CLASS, below, NULL, 0, NULL,
                                      0, TRUE, &done, &iosb);
  if (!irp)
    return STATUS_INSUFFICIENT_RESOURCES;
  IoGetNextIrpStackLocation(irp)->Parameters.Others.Argument1 = &irb;
  status = IoCallDriver(below, irp);
  if (status == STATUS_PENDING) {
    KeWaitForSingleObject(&done, Executive, KernelMode, FALSE, NULL);
    status = iosb.Status;
  }
  if (NT_SUCCESS(status))
    *count = irb.u.GetGenerationCount.GenerationCount;
  return status;
}

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING path) {
  (void)driver, (void)path;
  return STATUS_SUCCESS;
}
