/* A WDM USB client driver for Windows 98, 98 SE and ME (and 2000 and
   later): when its device starts it would select its first
   configuration, with USBD.SYS's helpers. run-tests.sh builds it with
   nt4-ddk.cfg and -lusbd9x and checks with check-imports.py --wdm that it
   imports only what 98's USBD.SYS has (USBD_CreateConfigurationRequestEx
   by its decorated name, as 98 and 98 SE export it); built with
   mingw-w64's -lusbd instead, the check for 98 SE must fail. */
#include <ntddk.h>
#include <usbdi.h>
#include <usbdlib.h>

PURB select_config(PUSB_CONFIGURATION_DESCRIPTOR config);
PURB select_config(PUSB_CONFIGURATION_DESCRIPTOR config) {
  USBD_INTERFACE_LIST_ENTRY list[2] = {{0}};
  list[0].InterfaceDescriptor = USBD_ParseConfigurationDescriptorEx(
      config, config, -1, -1, -1, -1, -1);
  if (!list[0].InterfaceDescriptor)
    return NULL;
  return USBD_CreateConfigurationRequestEx(config, list);
}

static void NTAPI Unload(PDRIVER_OBJECT driver) { (void)driver; }

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING path) {
  USBD_VERSION_INFORMATION v;
  (void)path;
  USBD_GetUSBDIVersion(&v);
  driver->DriverUnload = Unload;
  return v.USBDI_Version >= 0x100 ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED;
}
