/* hidport.h: What a HID
   minidriver (HIDUSB.SYS's kind, under HIDCLASS.SYS) needs:
   HID_MINIDRIVER_REGISTRATION for HidRegisterMinidriver (-lhidclass),
   HID_DEVICE_EXTENSION, the HID descriptor, and the internal IOCTL_HID_*
   requests HIDCLASS sends the minidriver.

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 2000 and XP DDKs; each declaration is under the first
   _WIN32_WINNT whose DDK has it. */
#ifndef WDM_HIDPORT_H
#define WDM_HIDPORT_H

#include <ntddk.h>
#include <hidusage.h>
#include <hidclass.h>
#define IOCTL_HID_ACTIVATE_DEVICE 0x000B001F
#define IOCTL_HID_DEACTIVATE_DEVICE 0x000B0023
#if _WIN32_WINNT >= 0x0501
#define IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST 0x000B002B
#endif
#define HID_HID_DESCRIPTOR_TYPE 0x21
#define HID_REPORT_DESCRIPTOR_TYPE 0x22
#define HID_PHYSICAL_DESCRIPTOR_TYPE 0x23
struct _HID_MINIDRIVER_REGISTRATION {
  ULONG Revision;
  PDRIVER_OBJECT DriverObject;
  PUNICODE_STRING RegistryPath;
  ULONG DeviceExtensionSize;
  BOOLEAN DevicesArePolled;
  UCHAR Reserved[3];
};
typedef struct _HID_MINIDRIVER_REGISTRATION HID_MINIDRIVER_REGISTRATION;
typedef struct _HID_MINIDRIVER_REGISTRATION *PHID_MINIDRIVER_REGISTRATION;
struct _HID_DEVICE_EXTENSION {
  PDEVICE_OBJECT PhysicalDeviceObject;
  PDEVICE_OBJECT NextDeviceObject;
  PVOID MiniDeviceExtension;
};
typedef struct _HID_DEVICE_EXTENSION HID_DEVICE_EXTENSION;
typedef struct _HID_DEVICE_EXTENSION *PHID_DEVICE_EXTENSION;
struct _HID_DEVICE_ATTRIBUTES {
  ULONG Size;
  USHORT VendorID;
  USHORT ProductID;
  USHORT VersionNumber;
  USHORT Reserved[11];
};
typedef struct _HID_DEVICE_ATTRIBUTES HID_DEVICE_ATTRIBUTES;
typedef struct _HID_DEVICE_ATTRIBUTES *PHID_DEVICE_ATTRIBUTES;
#pragma pack(push, 1)
struct _HID_DESCRIPTOR_DESC_LIST {
  UCHAR bReportType;
  USHORT wReportLength;
};
#pragma pack(pop)
#pragma pack(push, 1)
struct _HID_DESCRIPTOR {
  UCHAR bLength;
  UCHAR bDescriptorType;
  USHORT bcdHID;
  UCHAR bCountry;
  UCHAR bNumDescriptors;
  struct _HID_DESCRIPTOR_DESC_LIST DescriptorList[1];
};
#pragma pack(pop)
typedef struct _HID_DESCRIPTOR HID_DESCRIPTOR;
typedef struct _HID_DESCRIPTOR *PHID_DESCRIPTOR;
#if _WIN32_WINNT >= 0x0501
typedef void (*HID_SEND_IDLE_CALLBACK)(PVOID) __attribute__((stdcall));
struct _HID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO {
  HID_SEND_IDLE_CALLBACK IdleCallback;
  PVOID IdleContext;
};
typedef struct _HID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO HID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO;
typedef struct _HID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO *PHID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO;
#endif
NTSTATUS HidRegisterMinidriver(PHID_MINIDRIVER_REGISTRATION) __attribute__((stdcall));
#if _WIN32_WINNT >= 0x0501
NTSTATUS HidNotifyPresence(PDEVICE_OBJECT, BOOLEAN) __attribute__((stdcall));
#endif

#endif
