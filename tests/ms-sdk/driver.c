/* Built by run-test.sh with the config file of a fake kernel-mode kit. */
#include <NTDDK.H>

NTSTATUS DriverEntry(PDRIVER_OBJECT drv, PUNICODE_STRING reg) {
  PDEVICE_OBJECT dev;
  (void)reg;
  DbgPrint("driver\n");
  return IoCreateDevice(drv, 0, 0, 0x22, 0, 0, &dev);
}
