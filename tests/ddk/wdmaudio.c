/* A WDM audio adapter driver for Windows 98, 98 SE and ME (and 2000 and
   later) with port class. When its device starts, it makes a topology
   port and releases it, through the port's COM interface in C, and it
   registers no subdevices, so the device has no pins. run-tests.sh builds
   it with nt4-ddk.cfg and checks with check-imports.py --wdm that it
   imports only what 98's PORTCLS.SYS and NTKERN have; with -DME_ONLY it
   also registers an I/O timeout, which only ME's port class has, and the
   check for 98 SE must fail. */
#include <ntddk.h>
#include <initguid.h>
#include <portcls.h>

#ifdef ME_ONLY
static void NTAPI timeout(PDEVICE_OBJECT dev, PVOID context) {
  (void)dev, (void)context;
}
#endif

static NTSTATUS NTAPI StartDevice(PDEVICE_OBJECT dev, PIRP irp,
                                  PRESOURCELIST resources) {
  PPORT port;
  NTSTATUS status;
  (void)dev, (void)irp, (void)resources;
#ifdef ME_ONLY
  PcRegisterIoTimeout(dev, timeout, 0);
#endif
  status = PcNewPort(&port, &CLSID_PortTopology);
  if (NT_SUCCESS(status))
    port->lpVtbl->Release(port);
  return status;
}

static NTSTATUS NTAPI AddDevice(PDRIVER_OBJECT driver, PDEVICE_OBJECT pdo) {
  return PcAddAdapterDevice(driver, pdo, StartDevice, 1, 0);
}

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING path) {
  return PcInitializeAdapterDriver(driver, path, AddDevice);
}
