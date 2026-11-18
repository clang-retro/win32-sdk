/* srb.h: The SCSI request block (SRB) and
   what a miniport hands SCSIPORT: HW_INITIALIZATION_DATA, the
   PORT_CONFIGURATION_INFORMATION it fills in, the SRB_* functions,
   statuses and flags, and the ScsiPort* functions' notification types.
   The functions are in SCSIPORT.SYS (-lscsiport); Windows 9x's
   SCSIPORT.PDR has the 45 that tools/scsiport9x-exports.txt lists.

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   _WIN32_WINDOWS whose DDK has it. */
#ifndef MINIPORT9X_SRB_H
#define MINIPORT9X_SRB_H

#include <miniport.h>
#define SCSI_MAXIMUM_LOGICAL_UNITS 8
#define SCSI_MAXIMUM_TARGETS_PER_BUS 32
#define SCSI_MAXIMUM_BUSES 8
#define SCSI_MAXIMUM_TARGETS 8
#define MAXIMUM_CDB_SIZE 12
#define CONFIG_INFO_VERSION_2 0x7C
#define SP_UNINITIALIZED_VALUE 0xFFFFFFFF
#define SP_UNTAGGED 0xFF
#define SRBEV_BUS_RESET 0x01
#define SRBEV_SCSI_ASYNC_NOTIFICATION 0x02
#define SCSI_REQUEST_BLOCK_SIZE 0x40
#define SRB_FUNCTION_EXECUTE_SCSI 0x00
#define SRB_FUNCTION_CLAIM_DEVICE 0x01
#define SRB_FUNCTION_IO_CONTROL 0x02
#define SRB_FUNCTION_RECEIVE_EVENT 0x03
#define SRB_FUNCTION_RELEASE_QUEUE 0x04
#define SRB_FUNCTION_ATTACH_DEVICE 0x05
#define SRB_FUNCTION_RELEASE_DEVICE 0x06
#define SRB_FUNCTION_SHUTDOWN 0x07
#define SRB_FUNCTION_FLUSH 0x08
#define SRB_FUNCTION_ABORT_COMMAND 0x10
#define SRB_FUNCTION_RELEASE_RECOVERY 0x11
#define SRB_FUNCTION_RESET_BUS 0x12
#define SRB_FUNCTION_RESET_DEVICE 0x13
#define SRB_FUNCTION_TERMINATE_IO 0x14
#define SRB_FUNCTION_FLUSH_QUEUE 0x15
#define SRB_FUNCTION_REMOVE_DEVICE 0x16
#define SRB_STATUS_PENDING 0x00
#define SRB_STATUS_SUCCESS 0x01
#define SRB_STATUS_ABORTED 0x02
#define SRB_STATUS_ABORT_FAILED 0x03
#define SRB_STATUS_ERROR 0x04
#define SRB_STATUS_BUSY 0x05
#define SRB_STATUS_INVALID_REQUEST 0x06
#define SRB_STATUS_INVALID_PATH_ID 0x07
#define SRB_STATUS_NO_DEVICE 0x08
#define SRB_STATUS_TIMEOUT 0x09
#define SRB_STATUS_SELECTION_TIMEOUT 0x0A
#define SRB_STATUS_COMMAND_TIMEOUT 0x0B
#define SRB_STATUS_MESSAGE_REJECTED 0x0D
#define SRB_STATUS_BUS_RESET 0x0E
#define SRB_STATUS_PARITY_ERROR 0x0F
#define SRB_STATUS_REQUEST_SENSE_FAILED 0x10
#define SRB_STATUS_NO_HBA 0x11
#define SRB_STATUS_DATA_OVERRUN 0x12
#define SRB_STATUS_UNEXPECTED_BUS_FREE 0x13
#define SRB_STATUS_PHASE_SEQUENCE_FAILURE 0x14
#define SRB_STATUS_BAD_SRB_BLOCK_LENGTH 0x15
#define SRB_STATUS_REQUEST_FLUSHED 0x16
#define SRB_STATUS_INVALID_LUN 0x20
#define SRB_STATUS_INVALID_TARGET_ID 0x21
#define SRB_STATUS_BAD_FUNCTION 0x22
#define SRB_STATUS_ERROR_RECOVERY 0x23
#define SRB_STATUS_QUEUE_FROZEN 0x40
#define SRB_STATUS_AUTOSENSE_VALID 0x80
#define SRB_FLAGS_QUEUE_ACTION_ENABLE 0x02
#define SRB_FLAGS_DISABLE_DISCONNECT 0x04
#define SRB_FLAGS_DISABLE_SYNCH_TRANSFER 0x08
#define SRB_FLAGS_BYPASS_FROZEN_QUEUE 0x10
#define SRB_FLAGS_DISABLE_AUTOSENSE 0x20
#define SRB_FLAGS_DATA_IN 0x40
#define SRB_FLAGS_DATA_OUT 0x80
#define SRB_FLAGS_NO_DATA_TRANSFER 0x00
#define SRB_FLAGS_NO_QUEUE_FREEZE 0x0100
#define SRB_FLAGS_UNSPECFIED_DIRECTION 0xC0
#define SRB_FLAGS_UNSPECIFIED_DIRECTION 0xC0
#define SRB_FLAGS_ADAPTER_CACHE_ENABLE 0x0200
#define SRB_FLAGS_BIT_IS_ACTIVE 16
#define SRB_FLAGS_IS_ACTIVE 0x00010000
#define SRB_FLAGS_ALLOCATED_FROM_ZONE 0x00020000
#define SRB_FLAGS_SGLIST_FROM_POOL 0x00040000
#define SRB_SIMPLE_TAG_REQUEST 0x20
#define SRB_HEAD_OF_QUEUE_TAG_REQUEST 0x21
#define SRB_ORDERED_QUEUE_TAG_REQUEST 0x22
#define SP_BUS_PARITY_ERROR 0x01
#define SP_UNEXPECTED_DISCONNECT 0x02
#define SP_INVALID_RESELECTION 0x03
#define SP_BUS_TIME_OUT 0x04
#define SP_PROTOCOL_ERROR 0x05
#define SP_INTERNAL_ADAPTER_ERROR 0x06
#define SP_REQUEST_TIMEOUT 0x07
#define SP_IRQ_NOT_RESPONDING 0x08
#define SP_BAD_FW_WARNING 0x09
#define SP_BAD_FW_ERROR 0x0A
#define SP_RETURN_NOT_FOUND 0
#define SP_RETURN_FOUND 1
#define SP_RETURN_ERROR 2
#define SP_RETURN_BAD_CONFIG 3
typedef PHYSICAL_ADDRESS SCSI_PHYSICAL_ADDRESS;
typedef PHYSICAL_ADDRESS *PSCSI_PHYSICAL_ADDRESS;
struct _ACCESS_RANGE {
  SCSI_PHYSICAL_ADDRESS RangeStart;
  ULONG RangeLength;
  BOOLEAN RangeInMemory;
};
typedef struct _ACCESS_RANGE ACCESS_RANGE;
typedef struct _ACCESS_RANGE *PACCESS_RANGE;
struct _PORT_CONFIGURATION_INFORMATION {
  ULONG Length;
  ULONG SystemIoBusNumber;
  INTERFACE_TYPE AdapterInterfaceType;
  ULONG BusInterruptLevel;
  ULONG BusInterruptVector;
  KINTERRUPT_MODE InterruptMode;
  ULONG MaximumTransferLength;
  ULONG NumberOfPhysicalBreaks;
  ULONG DmaChannel;
  ULONG DmaPort;
  DMA_WIDTH DmaWidth;
  DMA_SPEED DmaSpeed;
  ULONG AlignmentMask;
  ULONG NumberOfAccessRanges;
  ACCESS_RANGE (*AccessRanges)[];
  PVOID Reserved;
  UCHAR NumberOfBuses;
  CCHAR InitiatorBusId[8];
  BOOLEAN ScatterGather;
  BOOLEAN Master;
  BOOLEAN CachesData;
  BOOLEAN AdapterScansDown;
  BOOLEAN AtdiskPrimaryClaimed;
  BOOLEAN AtdiskSecondaryClaimed;
  BOOLEAN Dma32BitAddresses;
  BOOLEAN DemandMode;
  BOOLEAN MapBuffers;
  BOOLEAN NeedPhysicalAddresses;
  BOOLEAN TaggedQueuing;
  BOOLEAN AutoRequestSense;
  BOOLEAN MultipleRequestPerLu;
  BOOLEAN ReceiveEvent;
  BOOLEAN RealModeInitialized;
  BOOLEAN BufferAccessScsiPortControlled;
  UCHAR MaximumNumberOfTargets;
  UCHAR ReservedUchars[2];
  ULONG SlotNumber;
  ULONG BusInterruptLevel2;
  ULONG BusInterruptVector2;
  KINTERRUPT_MODE InterruptMode2;
  ULONG DmaChannel2;
  ULONG DmaPort2;
  DMA_WIDTH DmaWidth2;
  DMA_SPEED DmaSpeed2;
};
typedef struct _PORT_CONFIGURATION_INFORMATION PORT_CONFIGURATION_INFORMATION;
typedef struct _PORT_CONFIGURATION_INFORMATION *PPORT_CONFIGURATION_INFORMATION;
struct _SCSI_REQUEST_BLOCK {
  USHORT Length;
  UCHAR Function;
  UCHAR SrbStatus;
  UCHAR ScsiStatus;
  UCHAR PathId;
  UCHAR TargetId;
  UCHAR Lun;
  UCHAR QueueTag;
  UCHAR QueueAction;
  UCHAR CdbLength;
  UCHAR SenseInfoBufferLength;
  ULONG SrbFlags;
  ULONG DataTransferLength;
  ULONG TimeOutValue;
  PVOID DataBuffer;
  PVOID SenseInfoBuffer;
  struct _SCSI_REQUEST_BLOCK *NextSrb;
  PVOID OriginalRequest;
  PVOID SrbExtension;
  ULONG QueueSortKey;
  UCHAR Cdb[16];
};
typedef struct _SCSI_REQUEST_BLOCK SCSI_REQUEST_BLOCK;
typedef struct _SCSI_REQUEST_BLOCK *PSCSI_REQUEST_BLOCK;
typedef BOOLEAN (*PHW_INITIALIZE)(PVOID) __attribute__((stdcall));
typedef BOOLEAN (*PHW_STARTIO)(PVOID, PSCSI_REQUEST_BLOCK) __attribute__((stdcall));
typedef BOOLEAN (*PHW_INTERRUPT)(PVOID) __attribute__((stdcall));
typedef void (*PHW_TIMER)(PVOID) __attribute__((stdcall));
typedef void (*PHW_DMA_STARTED)(PVOID) __attribute__((stdcall));
typedef ULONG (*PHW_FIND_ADAPTER)(PVOID, PVOID, PVOID, PCHAR, PPORT_CONFIGURATION_INFORMATION, PBOOLEAN) __attribute__((stdcall));
typedef BOOLEAN (*PHW_RESET_BUS)(PVOID, ULONG) __attribute__((stdcall));
typedef BOOLEAN (*PHW_ADAPTER_STATE)(PVOID, PVOID, BOOLEAN) __attribute__((stdcall));
enum _SCSI_NOTIFICATION_TYPE {
  RequestComplete = 0,
  NextRequest = 1,
  NextLuRequest = 2,
  ResetDetected = 3,
  CallDisableInterrupts = 4,
  CallEnableInterrupts = 5,
  RequestTimerCall = 6,
};
typedef enum _SCSI_NOTIFICATION_TYPE SCSI_NOTIFICATION_TYPE;
typedef enum _SCSI_NOTIFICATION_TYPE *PSCSI_NOTIFICATION_TYPE;
struct _HW_INITIALIZATION_DATA {
  ULONG HwInitializationDataSize;
  INTERFACE_TYPE AdapterInterfaceType;
  PHW_INITIALIZE HwInitialize;
  PHW_STARTIO HwStartIo;
  PHW_INTERRUPT HwInterrupt;
  PHW_FIND_ADAPTER HwFindAdapter;
  PHW_RESET_BUS HwResetBus;
  PHW_DMA_STARTED HwDmaStarted;
  PHW_ADAPTER_STATE HwAdapterState;
  ULONG DeviceExtensionSize;
  ULONG SpecificLuExtensionSize;
  ULONG SrbExtensionSize;
  ULONG NumberOfAccessRanges;
  PVOID Reserved;
  BOOLEAN MapBuffers;
  BOOLEAN NeedPhysicalAddresses;
  BOOLEAN TaggedQueuing;
  BOOLEAN AutoRequestSense;
  BOOLEAN MultipleRequestPerLu;
  BOOLEAN ReceiveEvent;
  USHORT VendorIdLength;
  PVOID VendorId;
  USHORT ReservedUshort;
  USHORT DeviceIdLength;
  PVOID DeviceId;
};
typedef struct _HW_INITIALIZATION_DATA HW_INITIALIZATION_DATA;
typedef struct _HW_INITIALIZATION_DATA *PHW_INITIALIZATION_DATA;
ULONG ScsiPortInitialize(PVOID, PVOID, struct _HW_INITIALIZATION_DATA *, PVOID) __attribute__((stdcall));
void ScsiPortFreeDeviceBase(PVOID, PVOID) __attribute__((stdcall));
ULONG ScsiPortGetBusData(PVOID, ULONG, ULONG, ULONG, PVOID, ULONG) __attribute__((stdcall));
ULONG ScsiPortSetBusDataByOffset(PVOID, ULONG, ULONG, ULONG, PVOID, ULONG, ULONG) __attribute__((stdcall));
PVOID ScsiPortGetDeviceBase(PVOID, INTERFACE_TYPE, ULONG, SCSI_PHYSICAL_ADDRESS, ULONG, BOOLEAN) __attribute__((stdcall));
PVOID ScsiPortGetLogicalUnit(PVOID, UCHAR, UCHAR, UCHAR) __attribute__((stdcall));
PSCSI_REQUEST_BLOCK ScsiPortGetSrb(PVOID, UCHAR, UCHAR, UCHAR, LONG) __attribute__((stdcall));
SCSI_PHYSICAL_ADDRESS ScsiPortGetPhysicalAddress(PVOID, PSCSI_REQUEST_BLOCK, PVOID, ULONG *) __attribute__((stdcall));
PVOID ScsiPortGetVirtualAddress(PVOID, SCSI_PHYSICAL_ADDRESS) __attribute__((stdcall));
PVOID ScsiPortGetUncachedExtension(PVOID, PPORT_CONFIGURATION_INFORMATION, ULONG) __attribute__((stdcall));
void ScsiPortFlushDma(PVOID) __attribute__((stdcall));
void ScsiPortIoMapTransfer(PVOID, PSCSI_REQUEST_BLOCK, PVOID, ULONG) __attribute__((stdcall));
void ScsiPortNotification(SCSI_NOTIFICATION_TYPE, PVOID, ...);
void ScsiPortLogError(PVOID, PSCSI_REQUEST_BLOCK, UCHAR, UCHAR, UCHAR, ULONG, ULONG) __attribute__((stdcall));
void ScsiPortCompleteRequest(PVOID, UCHAR, UCHAR, UCHAR, UCHAR) __attribute__((stdcall));
void ScsiPortMoveMemory(PVOID, PVOID, ULONG) __attribute__((stdcall));
UCHAR ScsiPortReadPortUchar(PUCHAR) __attribute__((stdcall));
USHORT ScsiPortReadPortUshort(PUSHORT) __attribute__((stdcall));
ULONG ScsiPortReadPortUlong(PULONG) __attribute__((stdcall));
void ScsiPortReadPortBufferUchar(PUCHAR, PUCHAR, ULONG) __attribute__((stdcall));
void ScsiPortReadPortBufferUshort(PUSHORT, PUSHORT, ULONG) __attribute__((stdcall));
void ScsiPortReadPortBufferUlong(PULONG, PULONG, ULONG) __attribute__((stdcall));
UCHAR ScsiPortReadRegisterUchar(PUCHAR) __attribute__((stdcall));
USHORT ScsiPortReadRegisterUshort(PUSHORT) __attribute__((stdcall));
ULONG ScsiPortReadRegisterUlong(PULONG) __attribute__((stdcall));
void ScsiPortReadRegisterBufferUchar(PUCHAR, PUCHAR, ULONG) __attribute__((stdcall));
void ScsiPortReadRegisterBufferUshort(PUSHORT, PUSHORT, ULONG) __attribute__((stdcall));
void ScsiPortReadRegisterBufferUlong(PULONG, PULONG, ULONG) __attribute__((stdcall));
void ScsiPortStallExecution(ULONG) __attribute__((stdcall));
void ScsiPortWritePortUchar(PUCHAR, UCHAR) __attribute__((stdcall));
void ScsiPortWritePortUshort(PUSHORT, USHORT) __attribute__((stdcall));
void ScsiPortWritePortUlong(PULONG, ULONG) __attribute__((stdcall));
void ScsiPortWritePortBufferUchar(PUCHAR, PUCHAR, ULONG) __attribute__((stdcall));
void ScsiPortWritePortBufferUshort(PUSHORT, PUSHORT, ULONG) __attribute__((stdcall));
void ScsiPortWritePortBufferUlong(PULONG, PULONG, ULONG) __attribute__((stdcall));
void ScsiPortWriteRegisterUchar(PUCHAR, UCHAR) __attribute__((stdcall));
void ScsiPortWriteRegisterUshort(PUSHORT, USHORT) __attribute__((stdcall));
void ScsiPortWriteRegisterUlong(PULONG, ULONG) __attribute__((stdcall));
void ScsiPortWriteRegisterBufferUchar(PUCHAR, PUCHAR, ULONG) __attribute__((stdcall));
void ScsiPortWriteRegisterBufferUshort(PUSHORT, PUSHORT, ULONG) __attribute__((stdcall));
void ScsiPortWriteRegisterBufferUlong(PULONG, PULONG, ULONG) __attribute__((stdcall));
SCSI_PHYSICAL_ADDRESS ScsiPortConvertUlongToPhysicalAddress(ULONG) __attribute__((stdcall));
ULONG ScsiPortConvertPhysicalAddressToUlong(SCSI_PHYSICAL_ADDRESS) __attribute__((stdcall));
BOOLEAN ScsiPortValidateRange(PVOID, INTERFACE_TYPE, ULONG, SCSI_PHYSICAL_ADDRESS, ULONG, BOOLEAN) __attribute__((stdcall));
void ScsiDebugPrint(ULONG, PCCHAR, ...);

#endif
