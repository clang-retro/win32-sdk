/* A driver that calls PsGetVersion, which the kernel of Windows 2000 and
   later has but the WDM of Windows 98 and ME does not: run-tests.sh checks
   that check-imports.py --wdm rejects it. */
#include <ntddk.h>

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT drv, PUNICODE_STRING path) {
  ULONG major = 0;
  (void)drv;
  (void)path;
  PsGetVersion(&major, NULL, NULL, NULL);
  return major ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}
