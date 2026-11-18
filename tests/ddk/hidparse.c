/* A HID client driver's report parsing, for Windows 98, 98 SE and ME (and
   2000 and later), with HIDPARSE.SYS. run-tests.sh builds it with
   nt4-ddk.cfg and checks with check-imports.py --wdm that it imports only
   what 98's HIDPARSE.SYS has; with -DSE_ONLY it also calls
   HidP_InitializeReportForID, which 98 SE added, and the check for 98
   must fail. */
#include <ntddk.h>
#include <hidusage.h>
#include <hidpi.h>

NTSTATUS buttons(PHIDP_PREPARSED_DATA data, PCHAR report, ULONG size,
                 PUSAGE usages, PULONG n);
NTSTATUS buttons(PHIDP_PREPARSED_DATA data, PCHAR report, ULONG size,
                 PUSAGE usages, PULONG n) {
  HIDP_CAPS c;
  NTSTATUS status = HidP_GetCaps(data, &c);
#ifdef SE_ONLY
  if (NT_SUCCESS(status))
    status = HidP_InitializeReportForID(HidP_Output, 0, data, report, size);
#endif
  if (NT_SUCCESS(status))
    status = HidP_GetUsages(HidP_Input, HID_USAGE_PAGE_BUTTON, 0, usages, n,
                            data, report, size);
  return status;
}

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING path) {
  (void)driver, (void)path;
  return STATUS_SUCCESS;
}
