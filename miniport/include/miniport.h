/* miniport.h: the types of a SCSI miniport for Windows 95, 98 and ME.
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) clang-retro contributors.
 *
 * Written for clang-retro. No text of a Microsoft header is used.
 *
 * - The NT base types (ULONG, BOOLEAN, PHYSICAL_ADDRESS ...) come from
 *   mingw-w64's <ntdef.h>.
 * - INTERFACE_TYPE, KINTERRUPT_MODE, DMA_WIDTH, DMA_SPEED, BUS_DATA_TYPE
 *   and the emulator access entries come from mingw-w64's
 *   <ddk/miniport.h>.
 * - This file adds the bus data structures that ScsiPortGetBusData
 *   returns: PCI configuration space (from the PCI Local Bus
 *   Specification and Microsoft's published documentation of
 *   PCI_COMMON_CONFIG and PCI_SLOT_NUMBER), EISA slot and function
 *   information, and MCA POS data. The EISA structures are copied from
 *   mingw-w64's <ddk/wdm.h>, which is in the public domain.
 *
 * Windows 9x has the PCI configuration header of PCI 2.0: offset 0x34
 * is part of Reserved2, not a capabilities pointer. */
#ifndef MINIPORT9X_MINIPORT_H
#define MINIPORT9X_MINIPORT_H

#include <ntdef.h>
#include <ddk/miniport.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The device and function number of a PCI device (the bus data slot). */
typedef struct _PCI_SLOT_NUMBER {
  union {
    struct {
      ULONG DeviceNumber : 5;
      ULONG FunctionNumber : 3;
      ULONG Reserved : 24;
    } bits;
    ULONG AsULONG;
  } u;
} PCI_SLOT_NUMBER, *PPCI_SLOT_NUMBER;

/* The number of base address registers of a type 0 header. */
#define PCI_TYPE0_ADDRESSES 6

/* The type 0 (normal device) part of the PCI configuration header. */
struct _PCI_HEADER_TYPE_0 {
  ULONG BaseAddresses[PCI_TYPE0_ADDRESSES];
  ULONG CIS;
  USHORT SubVendorID;
  USHORT SubSystemID;
  ULONG ROMBaseAddress;
  ULONG Reserved2[2];
  UCHAR InterruptLine;
  UCHAR InterruptPin;
  UCHAR MinimumGrant;
  UCHAR MaximumLatency;
};

/* The 256 bytes of the PCI configuration space of a function. */
typedef struct _PCI_COMMON_CONFIG {
  USHORT VendorID;
  USHORT DeviceID;
  USHORT Command;
  USHORT Status;
  UCHAR RevisionID;
  UCHAR ProgIf;
  UCHAR SubClass;
  UCHAR BaseClass;
  UCHAR CacheLineSize;
  UCHAR LatencyTimer;
  UCHAR HeaderType;
  UCHAR BIST;
  union {
    struct _PCI_HEADER_TYPE_0 type0;
  } u;
  UCHAR DeviceSpecific[192];
} PCI_COMMON_CONFIG, *PPCI_COMMON_CONFIG;

/* The length of the standard part of the configuration space. */
#define PCI_COMMON_HDR_LENGTH 64

/* The limits of PCI addressing. */
#define PCI_MAX_DEVICES 32
#define PCI_MAX_FUNCTION 8

/* The vendor ID of an empty slot. */
#define PCI_INVALID_VENDORID 0xFFFF

/* PCI_COMMON_CONFIG.HeaderType. */
#define PCI_MULTIFUNCTION 0x80
#define PCI_DEVICE_TYPE 0x00
#define PCI_BRIDGE_TYPE 0x01

/* PCI_COMMON_CONFIG.Command. */
#define PCI_ENABLE_IO_SPACE 0x0001
#define PCI_ENABLE_MEMORY_SPACE 0x0002
#define PCI_ENABLE_BUS_MASTER 0x0004

/* The adapter ID and POS bytes of a Micro Channel slot (byte aligned). */
#pragma pack(push, 1)
typedef struct _CM_MCA_POS_DATA {
  USHORT AdapterId;
  UCHAR PosData1;
  UCHAR PosData2;
  UCHAR PosData3;
  UCHAR PosData4;
} CM_MCA_POS_DATA, *PCM_MCA_POS_DATA;
#pragma pack(pop)

/* The EISA structures (from mingw-w64, public domain). */
#pragma pack(push, 1)

typedef struct _EISA_MEMORY_TYPE {
  UCHAR ReadWrite : 1;
  UCHAR Cached : 1;
  UCHAR Reserved0 : 1;
  UCHAR Type : 2;
  UCHAR Shared : 1;
  UCHAR Reserved1 : 1;
  UCHAR MoreEntries : 1;
} EISA_MEMORY_TYPE, *PEISA_MEMORY_TYPE;

typedef struct _EISA_MEMORY_CONFIGURATION {
  EISA_MEMORY_TYPE ConfigurationByte;
  UCHAR DataSize;
  USHORT AddressLowWord;
  UCHAR AddressHighByte;
  USHORT MemorySize;
} EISA_MEMORY_CONFIGURATION, *PEISA_MEMORY_CONFIGURATION;

typedef struct _EISA_IRQ_DESCRIPTOR {
  UCHAR Interrupt : 4;
  UCHAR Reserved : 1;
  UCHAR LevelTriggered : 1;
  UCHAR Shared : 1;
  UCHAR MoreEntries : 1;
} EISA_IRQ_DESCRIPTOR, *PEISA_IRQ_DESCRIPTOR;

typedef struct _EISA_IRQ_CONFIGURATION {
  EISA_IRQ_DESCRIPTOR ConfigurationByte;
  UCHAR Reserved;
} EISA_IRQ_CONFIGURATION, *PEISA_IRQ_CONFIGURATION;

typedef struct _DMA_CONFIGURATION_BYTE0 {
  UCHAR Channel : 3;
  UCHAR Reserved : 3;
  UCHAR Shared : 1;
  UCHAR MoreEntries : 1;
} DMA_CONFIGURATION_BYTE0;

typedef struct _DMA_CONFIGURATION_BYTE1 {
  UCHAR Reserved0 : 2;
  UCHAR TransferSize : 2;
  UCHAR Timing : 2;
  UCHAR Reserved1 : 2;
} DMA_CONFIGURATION_BYTE1;

typedef struct _EISA_DMA_CONFIGURATION {
  DMA_CONFIGURATION_BYTE0 ConfigurationByte0;
  DMA_CONFIGURATION_BYTE1 ConfigurationByte1;
} EISA_DMA_CONFIGURATION, *PEISA_DMA_CONFIGURATION;

typedef struct _EISA_PORT_DESCRIPTOR {
  UCHAR NumberPorts : 5;
  UCHAR Reserved : 1;
  UCHAR Shared : 1;
  UCHAR MoreEntries : 1;
} EISA_PORT_DESCRIPTOR, *PEISA_PORT_DESCRIPTOR;

typedef struct _EISA_PORT_CONFIGURATION {
  EISA_PORT_DESCRIPTOR Configuration;
  USHORT PortAddress;
} EISA_PORT_CONFIGURATION, *PEISA_PORT_CONFIGURATION;

typedef struct _CM_EISA_SLOT_INFORMATION {
  UCHAR ReturnCode;
  UCHAR ReturnFlags;
  UCHAR MajorRevision;
  UCHAR MinorRevision;
  USHORT Checksum;
  UCHAR NumberFunctions;
  UCHAR FunctionInformation;
  ULONG CompressedId;
} CM_EISA_SLOT_INFORMATION, *PCM_EISA_SLOT_INFORMATION;

typedef struct _CM_EISA_FUNCTION_INFORMATION {
  ULONG CompressedId;
  UCHAR IdSlotFlags1;
  UCHAR IdSlotFlags2;
  UCHAR MinorRevision;
  UCHAR MajorRevision;
  UCHAR Selections[26];
  UCHAR FunctionFlags;
  UCHAR TypeString[80];
  EISA_MEMORY_CONFIGURATION EisaMemory[9];
  EISA_IRQ_CONFIGURATION EisaIrq[7];
  EISA_DMA_CONFIGURATION EisaDma[4];
  EISA_PORT_CONFIGURATION EisaPort[20];
  UCHAR InitializationData[60];
} CM_EISA_FUNCTION_INFORMATION, *PCM_EISA_FUNCTION_INFORMATION;

#pragma pack(pop)

/* CM_EISA_FUNCTION_INFORMATION.FunctionFlags. */
#define EISA_FUNCTION_ENABLED 0x80
#define EISA_FREE_FORM_DATA 0x40
#define EISA_HAS_PORT_INIT_ENTRY 0x20
#define EISA_HAS_PORT_RANGE 0x10
#define EISA_HAS_DMA_ENTRY 0x08
#define EISA_HAS_IRQ_ENTRY 0x04
#define EISA_HAS_MEMORY_ENTRY 0x02
#define EISA_HAS_TYPE_ENTRY 0x01

#ifdef __cplusplus
}
#endif

#endif
