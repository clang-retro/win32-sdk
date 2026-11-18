/* Made by tools/gen-vxd-headers.py: the layouts of <hidport.h> as the XP DDK's
   compiler laid them out. */
#define _WIN32_WINNT 0x0501
#include <hidport.h>
#include <stddef.h>
#ifdef __cplusplus
#define A(e) static_assert(e, #e);
#else
#define A(e) _Static_assert(e, #e);
#endif
A(sizeof(struct _HID_MINIDRIVER_REGISTRATION) == 20)
A(__alignof__(struct _HID_MINIDRIVER_REGISTRATION) == 4)
A(offsetof(struct _HID_MINIDRIVER_REGISTRATION, Revision) == 0)
A(offsetof(struct _HID_MINIDRIVER_REGISTRATION, DriverObject) == 4)
A(offsetof(struct _HID_MINIDRIVER_REGISTRATION, RegistryPath) == 8)
A(offsetof(struct _HID_MINIDRIVER_REGISTRATION, DeviceExtensionSize) == 12)
A(offsetof(struct _HID_MINIDRIVER_REGISTRATION, DevicesArePolled) == 16)
A(offsetof(struct _HID_MINIDRIVER_REGISTRATION, Reserved) == 17)
A(sizeof(struct _HID_DEVICE_EXTENSION) == 12)
A(__alignof__(struct _HID_DEVICE_EXTENSION) == 4)
A(offsetof(struct _HID_DEVICE_EXTENSION, PhysicalDeviceObject) == 0)
A(offsetof(struct _HID_DEVICE_EXTENSION, NextDeviceObject) == 4)
A(offsetof(struct _HID_DEVICE_EXTENSION, MiniDeviceExtension) == 8)
A(sizeof(struct _HID_DEVICE_ATTRIBUTES) == 32)
A(__alignof__(struct _HID_DEVICE_ATTRIBUTES) == 4)
A(offsetof(struct _HID_DEVICE_ATTRIBUTES, Size) == 0)
A(offsetof(struct _HID_DEVICE_ATTRIBUTES, VendorID) == 4)
A(offsetof(struct _HID_DEVICE_ATTRIBUTES, ProductID) == 6)
A(offsetof(struct _HID_DEVICE_ATTRIBUTES, VersionNumber) == 8)
A(offsetof(struct _HID_DEVICE_ATTRIBUTES, Reserved) == 10)
A(sizeof(struct _HID_DESCRIPTOR_DESC_LIST) == 3)
A(__alignof__(struct _HID_DESCRIPTOR_DESC_LIST) == 1)
A(offsetof(struct _HID_DESCRIPTOR_DESC_LIST, bReportType) == 0)
A(offsetof(struct _HID_DESCRIPTOR_DESC_LIST, wReportLength) == 1)
A(sizeof(struct _HID_DESCRIPTOR) == 9)
A(__alignof__(struct _HID_DESCRIPTOR) == 1)
A(offsetof(struct _HID_DESCRIPTOR, bLength) == 0)
A(offsetof(struct _HID_DESCRIPTOR, bDescriptorType) == 1)
A(offsetof(struct _HID_DESCRIPTOR, bcdHID) == 2)
A(offsetof(struct _HID_DESCRIPTOR, bCountry) == 4)
A(offsetof(struct _HID_DESCRIPTOR, bNumDescriptors) == 5)
A(offsetof(struct _HID_DESCRIPTOR, DescriptorList) == 6)
A(sizeof(struct _HID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO) == 8)
A(__alignof__(struct _HID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO) == 4)
A(offsetof(struct _HID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO, IdleCallback) == 0)
A(offsetof(struct _HID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO, IdleContext) == 4)
