/* hidport.h: the interface between HIDCLASS.SYS and a HID minidriver
 * (the kind of driver that HIDUSB.SYS is), for Windows 98, ME, 2000 and
 * later.
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) clang-retro contributors.
 *
 * Written for clang-retro from Microsoft's published documentation of
 * HID minidrivers. No text of a Microsoft header is used.
 *
 * A minidriver calls HidRegisterMinidriver from DriverEntry (link with
 * -lhidclass). HIDCLASS then owns the driver object and sends the
 * minidriver IRP_MJ_INTERNAL_DEVICE_CONTROL requests with the
 * IOCTL_HID_* codes below. The device extension that HIDCLASS gives the
 * minidriver is HID_DEVICE_EXTENSION.MiniDeviceExtension. */
#ifndef WDM_HIDPORT_H
#define WDM_HIDPORT_H

#include <ntddk.h>
#include <hidusage.h>
#include <hidclass.h>

#ifdef __cplusplus
extern "C" {
#endif

/* HID_MINIDRIVER_REGISTRATION.Revision. */
#define HID_REVISION 0x00000001

/* What a minidriver gives to HidRegisterMinidriver. */
typedef struct _HID_MINIDRIVER_REGISTRATION {
  ULONG Revision;               /* HID_REVISION */
  PDRIVER_OBJECT DriverObject;  /* from DriverEntry */
  PUNICODE_STRING RegistryPath; /* from DriverEntry */
  ULONG DeviceExtensionSize;    /* bytes of MiniDeviceExtension */
  BOOLEAN DevicesArePolled;     /* TRUE: HIDCLASS polls for reports */
  UCHAR Reserved[3];
} HID_MINIDRIVER_REGISTRATION, *PHID_MINIDRIVER_REGISTRATION;

/* The device extension of each device object that HIDCLASS makes. */
typedef struct _HID_DEVICE_EXTENSION {
  PDEVICE_OBJECT PhysicalDeviceObject; /* the PDO of the bus */
  PDEVICE_OBJECT NextDeviceObject;     /* send IRPs down to this one */
  PVOID MiniDeviceExtension;           /* the minidriver's own data */
} HID_DEVICE_EXTENSION, *PHID_DEVICE_EXTENSION;

/* What the minidriver returns for IOCTL_HID_GET_DEVICE_ATTRIBUTES. */
typedef struct _HID_DEVICE_ATTRIBUTES {
  ULONG Size;          /* sizeof(HID_DEVICE_ATTRIBUTES) */
  USHORT VendorID;
  USHORT ProductID;
  USHORT VersionNumber;
  USHORT Reserved[11];
} HID_DEVICE_ATTRIBUTES, *PHID_DEVICE_ATTRIBUTES;

/* The HID descriptor of the device, as in the HID specification, that
 * the minidriver returns for IOCTL_HID_GET_DEVICE_DESCRIPTOR. */
#pragma pack(push, 1)
struct _HID_DESCRIPTOR_DESC_LIST {
  UCHAR bReportType;     /* a report descriptor: 0x22 */
  USHORT wReportLength;  /* its length in bytes */
};
typedef struct _HID_DESCRIPTOR {
  UCHAR bLength;
  UCHAR bDescriptorType;
  USHORT bcdHID;
  UCHAR bCountry;
  UCHAR bNumDescriptors;
  struct _HID_DESCRIPTOR_DESC_LIST DescriptorList[1];
} HID_DESCRIPTOR, *PHID_DESCRIPTOR;
#pragma pack(pop)

/* IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST (Windows XP and later): the
 * function that the minidriver calls when the device can go idle. */
typedef void (*HID_SEND_IDLE_CALLBACK)(PVOID Context);
typedef struct _HID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO {
  HID_SEND_IDLE_CALLBACK IdleCallback;
  PVOID IdleContext;
} HID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO,
    *PHID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO;

/* Register the minidriver with HIDCLASS. */
NTSTATUS NTAPI HidRegisterMinidriver(
    PHID_MINIDRIVER_REGISTRATION MinidriverRegistration);

/* The internal device control codes that HIDCLASS sends. Some copies
 * of <hidclass.h> give placeholder values for some of these codes. This
 * header replaces them with the values that HIDCLASS uses. */
#ifndef HID_CTL_CODE
#define HID_CTL_CODE(id) \
  CTL_CODE(FILE_DEVICE_KEYBOARD, (id), METHOD_NEITHER, FILE_ANY_ACCESS)
#endif
#undef IOCTL_HID_GET_DEVICE_DESCRIPTOR
#define IOCTL_HID_GET_DEVICE_DESCRIPTOR HID_CTL_CODE(0)
#undef IOCTL_HID_GET_REPORT_DESCRIPTOR
#define IOCTL_HID_GET_REPORT_DESCRIPTOR HID_CTL_CODE(1)
#undef IOCTL_HID_READ_REPORT
#define IOCTL_HID_READ_REPORT HID_CTL_CODE(2)
#undef IOCTL_HID_WRITE_REPORT
#define IOCTL_HID_WRITE_REPORT HID_CTL_CODE(3)
#undef IOCTL_HID_GET_STRING
#define IOCTL_HID_GET_STRING HID_CTL_CODE(4)
#undef IOCTL_HID_ACTIVATE_DEVICE
#define IOCTL_HID_ACTIVATE_DEVICE HID_CTL_CODE(7)
#undef IOCTL_HID_DEACTIVATE_DEVICE
#define IOCTL_HID_DEACTIVATE_DEVICE HID_CTL_CODE(8)
#undef IOCTL_HID_GET_DEVICE_ATTRIBUTES
#define IOCTL_HID_GET_DEVICE_ATTRIBUTES HID_CTL_CODE(9)
#undef IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST
#define IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST HID_CTL_CODE(10)

#ifdef __cplusplus
}
#endif

#endif
