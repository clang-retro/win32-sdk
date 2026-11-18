/* miniport.h: The types of a SCSI miniport for
   Windows 95, 98 and ME: the NT base types it uses (ULONG, BOOLEAN,
   PHYSICAL_ADDRESS ...), INTERFACE_TYPE, the bus data structures (PCI,
   EISA, MCA) ScsiPortGetBusData returns, and the access ranges.

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   _WIN32_WINDOWS whose DDK has it. */
#ifndef MINIPORT9X_MINIPORT_H
#define MINIPORT9X_MINIPORT_H

#define ANYSIZE_ARRAY 1
#define STRICT 1
#define OBJ_HANDLE_TAGBITS 0x03
#define FALSE 0
#define TRUE 1
#ifndef NULL
#define NULL ((void *)0)
#endif
#define Internal 0
#define Isa 1
#define Eisa 2
#define MicroChannel 3
#define TurboChannel 4
#define PCIBus 5
#define VMEBus 6
#define NuBus 7
#define PCMCIABus 8
#define CBus 9
#define MPIBus 10
#define MPSABus 11
#define MaximumInterfaceType 11
#define Width8Bits 0
#define Width16Bits 1
#define Width32Bits 2
#define MaximumDmaWidth 3
#define Compatible 0
#define TypeA 1
#define TypeB 2
#define TypeC 3
#define MaximumDmaSpeed 4
#define LevelSensitive 0
#define Latched 1
#define EMULATOR_READ_ACCESS 0x01
#define EMULATOR_WRITE_ACCESS 0x02
#define PCI_TYPE0_ADDRESSES 6
#define PCI_COMMON_HDR_LENGTH 0x40
#define PCI_MAX_DEVICES 32
#define PCI_MAX_FUNCTION 8
#define PCI_INVALID_VENDORID 0xFFFF
#define PCI_MULTIFUNCTION 0x80
#define PCI_ENABLE_IO_SPACE 0x01
#define PCI_ENABLE_MEMORY_SPACE 0x02
#define PCI_ENABLE_BUS_MASTER 0x04
#define PCI_ENABLE_SPECIAL_CYCLES 0x08
#define PCI_ENABLE_WRITE_AND_INVALIDATE 0x10
#define PCI_ENABLE_VGA_COMPATIBLE_PALETTE 0x20
#define PCI_ENABLE_PARITY 0x40
#define PCI_ENABLE_WAIT_CYCLE 0x80
#define PCI_ENABLE_SERR 0x0100
#define PCI_ENABLE_FAST_BACK_TO_BACK 0x0200
#define PCI_STATUS_FAST_BACK_TO_BACK 0x80
#define PCI_STATUS_DATA_PARITY_DETECTED 0x0100
#define PCI_STATUS_DEVSEL 0x0600
#define PCI_STATUS_SIGNALED_TARGET_ABORT 0x0800
#define PCI_STATUS_RECEIVED_TARGET_ABORT 0x1000
#define PCI_STATUS_RECEIVED_MASTER_ABORT 0x2000
#define PCI_STATUS_SIGNALED_SYSTEM_ERROR 0x4000
#define PCI_STATUS_DETECTED_PARITY_ERROR 0x8000
#define PCI_ADDRESS_IO_SPACE 0x01
#define PCI_ADDRESS_MEMORY_TYPE_MASK 0x06
#define PCI_ADDRESS_MEMORY_PREFETCHABLE 0x08
#define PCI_TYPE_32BIT 0
#define PCI_TYPE_20BIT 2
#define PCI_TYPE_64BIT 4
#define PCI_ROMADDRESS_ENABLED 0x01
#define EISA_FUNCTION_ENABLED 0x80
#define EISA_FREE_FORM_DATA 0x40
#define EISA_HAS_PORT_INIT_ENTRY 0x20
#define EISA_HAS_PORT_RANGE 0x10
#define EISA_HAS_DMA_ENTRY 0x08
#define EISA_HAS_IRQ_ENTRY 0x04
#define EISA_HAS_MEMORY_ENTRY 0x02
#define EISA_HAS_TYPE_ENTRY 0x01
#define EISA_HAS_INFORMATION 0x1F
#define EISA_MORE_ENTRIES 0x80
#define EISA_SYSTEM_MEMORY 0x00
#define EISA_MEMORY_TYPE_RAM 0x01
#define EISA_INVALID_SLOT 0x80
#define EISA_INVALID_FUNCTION 0x81
#define EISA_INVALID_CONFIGURATION 0x82
#define EISA_EMPTY_SLOT 0x83
#define EISA_INVALID_BIOS_CALL 0x86
typedef void *PVOID;
typedef char CHAR;
typedef short SHORT;
typedef long LONG;
typedef CHAR *PCHAR;
typedef CHAR *LPCH;
typedef CHAR *PCH;
typedef const CHAR *LPCCH;
typedef const CHAR *PCCH;
typedef CHAR *NPSTR;
typedef CHAR *LPSTR;
typedef CHAR *PSTR;
typedef const CHAR *LPCSTR;
typedef const CHAR *PCSTR;
typedef char TCHAR;
typedef char *PTCHAR;
typedef unsigned char TUCHAR;
typedef unsigned char *PTUCHAR;
typedef LPSTR LPTCH;
typedef LPSTR PTCH;
typedef LPSTR PTSTR;
typedef LPSTR LPTSTR;
typedef LPCSTR LPCTSTR;
typedef double DOUBLE;
struct _QUAD {
  double DoNotUseThisField;
};
typedef struct _QUAD QUAD;
typedef SHORT *PSHORT;
typedef LONG *PLONG;
typedef QUAD *PQUAD;
typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef unsigned long ULONG;
typedef QUAD UQUAD;
typedef UCHAR *PUCHAR;
typedef USHORT *PUSHORT;
typedef ULONG *PULONG;
typedef UQUAD *PUQUAD;
typedef signed char SCHAR;
typedef SCHAR *PSCHAR;
typedef void *HANDLE;
typedef HANDLE *PHANDLE;
typedef char CCHAR;
typedef short CSHORT;
typedef ULONG CLONG;
typedef CCHAR *PCCHAR;
typedef CSHORT *PCSHORT;
typedef CLONG *PCLONG;
typedef double LONGLONG;
typedef double ULONGLONG;
union _LARGE_INTEGER {
  struct {
    ULONG LowPart;
    LONG HighPart;
  };
  LONGLONG QuadPart;
};
typedef union _LARGE_INTEGER LARGE_INTEGER;
typedef union _LARGE_INTEGER *PLARGE_INTEGER;
union _ULARGE_INTEGER {
  struct {
    ULONG LowPart;
    ULONG HighPart;
  };
  ULONGLONG QuadPart;
};
typedef union _ULARGE_INTEGER ULARGE_INTEGER;
typedef union _ULARGE_INTEGER *PULARGE_INTEGER;
typedef LARGE_INTEGER PHYSICAL_ADDRESS;
typedef LARGE_INTEGER *PPHYSICAL_ADDRESS;
typedef CCHAR BOOLEAN;
typedef BOOLEAN *PBOOLEAN;
typedef CCHAR KIRQL;
typedef KIRQL *PKIRQL;
typedef ULONG INTERFACE_TYPE;
typedef ULONG *PINTERFACE_TYPE;
typedef ULONG DMA_WIDTH;
typedef ULONG *PDMA_WIDTH;
typedef ULONG DMA_SPEED;
typedef ULONG *PDMA_SPEED;
typedef ULONG KINTERRUPT_MODE;
enum _BUS_DATA_TYPE {
  Cmos = 0,
  EisaConfiguration = 1,
  Pos = 2,
  CbusConfiguration = 3,
  PCIConfiguration = 4,
  VMEConfiguration = 5,
  NuBusConfiguration = 6,
  PCMCIAConfiguration = 7,
  MPIConfiguration = 8,
  MPSAConfiguration = 9,
  MaximumBusDataType = 10,
};
typedef enum _BUS_DATA_TYPE BUS_DATA_TYPE;
typedef enum _BUS_DATA_TYPE *PBUS_DATA_TYPE;
enum _EMULATOR_PORT_ACCESS_TYPE {
  SUchar = 0,
  SUshort = 1,
  SUlong = 2,
};
typedef enum _EMULATOR_PORT_ACCESS_TYPE EMULATOR_PORT_ACCESS_TYPE;
typedef enum _EMULATOR_PORT_ACCESS_TYPE *PEMULATOR_PORT_ACCESS_TYPE;
struct _EMULATOR_ACCESS_ENTRY {
  ULONG BasePort;
  ULONG NumConsecutivePorts;
  EMULATOR_PORT_ACCESS_TYPE AccessType;
  UCHAR AccessMode;
  UCHAR StringSupport;
  PVOID Routine;
};
typedef struct _EMULATOR_ACCESS_ENTRY EMULATOR_ACCESS_ENTRY;
typedef struct _EMULATOR_ACCESS_ENTRY *PEMULATOR_ACCESS_ENTRY;
struct _PCI_SLOT_NUMBER {
  union {
    struct {
      ULONG DeviceNumber : 5;
      ULONG FunctionNumber : 3;
      ULONG Reserved : 24;
    } bits;
    ULONG AsULONG;
  } u;
};
typedef struct _PCI_SLOT_NUMBER PCI_SLOT_NUMBER;
typedef struct _PCI_SLOT_NUMBER *PPCI_SLOT_NUMBER;
#if _WIN32_WINDOWS >= 0x0410
struct _PCI_HEADER_TYPE_0 {
  ULONG BaseAddresses[6];
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
#else
struct _PCI_HEADER_TYPE_0 {
  ULONG BaseAddresses[6];
  ULONG Reserved1[2];
  ULONG ROMBaseAddress;
  ULONG Reserved2[2];
  UCHAR InterruptLine;
  UCHAR InterruptPin;
  UCHAR MinimumGrant;
  UCHAR MaximumLatency;
};
#endif
struct _PCI_COMMON_CONFIG {
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
};
typedef struct _PCI_COMMON_CONFIG PCI_COMMON_CONFIG;
typedef struct _PCI_COMMON_CONFIG *PPCI_COMMON_CONFIG;
#pragma pack(push, 1)
struct _CM_MCA_POS_DATA {
  USHORT AdapterId;
  UCHAR PosData1;
  UCHAR PosData2;
  UCHAR PosData3;
  UCHAR PosData4;
};
#pragma pack(pop)
typedef struct _CM_MCA_POS_DATA CM_MCA_POS_DATA;
typedef struct _CM_MCA_POS_DATA *PCM_MCA_POS_DATA;
#pragma pack(push, 1)
struct _EISA_MEMORY_TYPE {
  UCHAR ReadWrite : 1;
  UCHAR Cached : 1;
  UCHAR Reserved0 : 1;
  UCHAR Type : 2;
  UCHAR Shared : 1;
  UCHAR Reserved1 : 1;
  UCHAR MoreEntries : 1;
};
#pragma pack(pop)
typedef struct _EISA_MEMORY_TYPE EISA_MEMORY_TYPE;
typedef struct _EISA_MEMORY_TYPE *PEISA_MEMORY_TYPE;
#pragma pack(push, 1)
struct _EISA_MEMORY_CONFIGURATION {
  EISA_MEMORY_TYPE ConfigurationByte;
  UCHAR DataSize;
  USHORT AddressLowWord;
  UCHAR AddressHighByte;
  USHORT MemorySize;
};
#pragma pack(pop)
typedef struct _EISA_MEMORY_CONFIGURATION EISA_MEMORY_CONFIGURATION;
typedef struct _EISA_MEMORY_CONFIGURATION *PEISA_MEMORY_CONFIGURATION;
#pragma pack(push, 1)
struct _EISA_IRQ_DESCRIPTOR {
  UCHAR Interrupt : 4;
  UCHAR Reserved : 1;
  UCHAR LevelTriggered : 1;
  UCHAR Shared : 1;
  UCHAR MoreEntries : 1;
};
#pragma pack(pop)
typedef struct _EISA_IRQ_DESCRIPTOR EISA_IRQ_DESCRIPTOR;
typedef struct _EISA_IRQ_DESCRIPTOR *PEISA_IRQ_DESCRIPTOR;
#pragma pack(push, 1)
struct _EISA_IRQ_CONFIGURATION {
  EISA_IRQ_DESCRIPTOR ConfigurationByte;
  UCHAR Reserved;
};
#pragma pack(pop)
typedef struct _EISA_IRQ_CONFIGURATION EISA_IRQ_CONFIGURATION;
typedef struct _EISA_IRQ_CONFIGURATION *PEISA_IRQ_CONFIGURATION;
#pragma pack(push, 1)
struct _DMA_CONFIGURATION_BYTE0 {
  UCHAR Channel : 3;
  UCHAR Reserved : 3;
  UCHAR Shared : 1;
  UCHAR MoreEntries : 1;
};
#pragma pack(pop)
typedef struct _DMA_CONFIGURATION_BYTE0 DMA_CONFIGURATION_BYTE0;
#pragma pack(push, 1)
struct _DMA_CONFIGURATION_BYTE1 {
  UCHAR Reserved0 : 2;
  UCHAR TransferSize : 2;
  UCHAR Timing : 2;
  UCHAR Reserved1 : 2;
};
#pragma pack(pop)
typedef struct _DMA_CONFIGURATION_BYTE1 DMA_CONFIGURATION_BYTE1;
#pragma pack(push, 1)
struct _EISA_DMA_CONFIGURATION {
  DMA_CONFIGURATION_BYTE0 ConfigurationByte0;
  DMA_CONFIGURATION_BYTE1 ConfigurationByte1;
};
#pragma pack(pop)
typedef struct _EISA_DMA_CONFIGURATION EISA_DMA_CONFIGURATION;
typedef struct _EISA_DMA_CONFIGURATION *PEISA_DMA_CONFIGURATION;
#pragma pack(push, 1)
struct _EISA_PORT_DESCRIPTOR {
  UCHAR NumberPorts : 5;
  UCHAR Reserved : 1;
  UCHAR Shared : 1;
  UCHAR MoreEntries : 1;
};
#pragma pack(pop)
typedef struct _EISA_PORT_DESCRIPTOR EISA_PORT_DESCRIPTOR;
typedef struct _EISA_PORT_DESCRIPTOR *PEISA_PORT_DESCRIPTOR;
#pragma pack(push, 1)
struct _EISA_PORT_CONFIGURATION {
  EISA_PORT_DESCRIPTOR Configuration;
  USHORT PortAddress;
};
#pragma pack(pop)
typedef struct _EISA_PORT_CONFIGURATION EISA_PORT_CONFIGURATION;
typedef struct _EISA_PORT_CONFIGURATION *PEISA_PORT_CONFIGURATION;
#pragma pack(push, 1)
struct _CM_EISA_SLOT_INFORMATION {
  UCHAR ReturnCode;
  UCHAR ReturnFlags;
  UCHAR MajorRevision;
  UCHAR MinorRevision;
  USHORT Checksum;
  UCHAR NumberFunctions;
  UCHAR FunctionInformation;
  ULONG CompressedId;
};
#pragma pack(pop)
typedef struct _CM_EISA_SLOT_INFORMATION CM_EISA_SLOT_INFORMATION;
typedef struct _CM_EISA_SLOT_INFORMATION *PCM_EISA_SLOT_INFORMATION;
#pragma pack(push, 1)
struct _CM_EISA_FUNCTION_INFORMATION {
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
};
#pragma pack(pop)
typedef struct _CM_EISA_FUNCTION_INFORMATION CM_EISA_FUNCTION_INFORMATION;
typedef struct _CM_EISA_FUNCTION_INFORMATION *PCM_EISA_FUNCTION_INFORMATION;

#endif
