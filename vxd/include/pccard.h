/* pccard.h: PC Card (PCMCIA) Socket Services and Card
   Services: their function codes, return codes and structures.

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   VXD_WINVER whose DDK has it. */
#ifndef VXD_PCCARD_H
#define VXD_PCCARD_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
#define PCCARD_DEVICE_ID 0x097C
#if VXD_WINVER >= 0x040A
#define ACMF_READ 0x00
#define ACMF_WRITE 0x01
#endif
#define F_CLOSE_MEMORY 0x00
#define F_COPY_MEMORY 0x01
#define F_DEREGISTER_CLIENT 0x02
#define F_GET_CLIENT_INFO 0x03
#define F_GET_CONFIGURATION_INFO 0x04
#define F_GET_FIRST_PARTITION 0x05
#define F_GET_FIRST_REGION 0x06
#define F_GET_FIRST_TUPLE 0x07
#define F_GET_NEXT_PARTITION 0x08
#define F_GET_NEXT_REGION 0x09
#define F_GET_NEXT_TUPLE 0x0A
#define F_GET_CARD_SERVICES_INFO 0x0B
#define F_CS_GET_STATUS 0x0C
#define F_GET_TUPLE_DATA 0x0D
#define F_GET_FIRST_CLIENT 0x0E
#define F_REGISTER_ERASE_QUEUE 0x0F
#define F_REGISTER_CLIENT 0x10
#define F_RESET_CARD 0x11
#define F_MAP_LOG_SOCKET 0x12
#define F_MAP_LOG_WINDOW 0x13
#define F_MAP_MEM_PAGE 0x14
#define F_MAP_PHY_SOCKET 0x15
#define F_MAP_PHY_WINDOW 0x16
#define F_MODIFY_WINDOW 0x17
#define F_OPEN_MEMORY 0x18
#define F_READ_MEMORY 0x19
#define F_REGISTER_MTD 0x1A
#define F_RELEASE_IO 0x1B
#define F_RELEASE_IRQ 0x1C
#define F_RELEASE_WINDOW 0x1D
#define F_RELEASE_CONFIGURATION 0x1E
#define F_REQUEST_IO 0x1F
#define F_REQUEST_IRQ 0x20
#define F_REQUEST_WINDOW 0x21
#define F_REQUEST_SOCKET_MASK 0x22
#define F_RETURN_SS_ENTRY 0x23
#define F_WRITE_MEMORY 0x24
#define F_DEREGISTER_ERASE_QUEUE 0x25
#define F_CHECK_ERASE_QUEUE 0x26
#define F_MODIFY_CONFIGURATION 0x27
#define F_REGISTER_TIMER 0x28
#define F_SET_REGION 0x29
#define F_GET_NEXT_CLIENT 0x2A
#define F_VALIDATE_CIS 0x2B
#define F_REQUEST_EXCLUSIVE 0x2C
#define F_RELEASE_EXCLUSIVE 0x2D
#define F_GET_EVENT_MASK 0x2E
#define F_RELEASE_SOCKET_MASK 0x2F
#define F_REQUEST_CONFIGURATION 0x30
#define F_SET_EVENT_MASK 0x31
#define F_ADD_SOCKET_SERVICES 0x32
#define F_REPLACE_SOCKET_SERVICES 0x33
#define F_CS_VENDOR_SPECIFIC 0x34
#define F_ADJUST_RESOURCE_INFO 0x35
#define F_ACCESS_CONFIGURATION_REGISTER 0x36
#define MAX_CS_FUNCTIONS 0x37
#define EV_BATTERY_DEAD 0x01
#define EV_BATTERY_LOW 0x02
#define EV_CARD_LOCK 0x03
#define EV_CARD_READY 0x04
#define EV_CARD_REMOVAL 0x05
#define EV_CARD_UNLOCK 0x06
#define EV_EJECTION_COMPLETE 0x07
#define EV_EJECTION_REQUEST 0x08
#define EV_INSERTION_COMPLETE 0x09
#define EV_INSERTION_REQUEST 0x0A
#define EV_PM_RESUME 0x0B
#define EV_PM_SUSPEND 0x0C
#define EV_EXCLUSIVE_COMPLETE 0x0D
#define EV_EXCLUSIVE_REQUEST 0x0E
#define EV_RESET_PHYSICAL 0x0F
#define EV_RESET_REQUEST 0x10
#define EV_CARD_RESET 0x11
#define EV_MTD_REQUEST 0x12
#define EV_CLIENT_INFO 0x14
#define EV_TIMER_EXPIRED 0x15
#define EV_SS_UPDATED 0x16
#define EV_WRITE_PROTECT 0x17
#define EV_CARD_INSERTION 0x40
#define EV_RESET_COMPLETE 0x80
#define EV_ERASE_COMPLETE 0x81
#define EV_REGISTRATION_COMPLETE 0x82
#define R_SUCCESS 0x00
#define R_BAD_ADAPTER 0x01
#define R_BAD_ATTRIBUTE 0x02
#define R_BAD_BASE 0x03
#define R_BAD_EDC 0x04
#define R_RESERVED_05 0x05
#define R_BAD_IRQ 0x06
#define R_BAD_OFFSET 0x07
#define R_BAD_PAGE 0x08
#define R_READ_FAILURE 0x09
#define R_BAD_SIZE 0x0A
#define R_BAD_SOCKET 0x0B
#define R_RESERVED_0C 0x0C
#define R_BAD_TYPE 0x0D
#define R_BAD_VCC 0x0E
#define R_BAD_VPP 0x0F
#define R_RESERVED_10 0x10
#define R_BAD_WINDOW 0x11
#define R_WRITE_FAILURE 0x12
#define R_RESERVED_13 0x13
#define R_NO_CARD 0x14
#define R_UNSUPPORTED_FUNCTION 0x15
#define R_UNSUPPORTED_MODE 0x16
#define R_BAD_SPEED 0x17
#define R_BUSY 0x18
#define R_GENERAL_FAILURE 0x19
#define R_WRITE_PROTECTED 0x1A
#define R_BAD_ARG_LENGTH 0x1B
#define R_BAD_ARGS 0x1C
#define R_CONFIGURATION_LOCKED 0x1D
#define R_IN_USE 0x1E
#define R_NO_MORE_ITEMS 0x1F
#define R_OUT_OF_RESOURCE 0x20
#define R_BAD_HANDLE 0x21
#define ACRA_READ 0
#define ACRA_WRITE 1
#define SSA_REAL_MODE 0
#define SSA_1616_PROTECT_MODE 1
#define SSA_1632_PROTECT_MODE 2
#define SSA_FLAT_PROTECT_MODE 3
#define ARIR_MEM_RANGE 0
#define ARIR_IO_RANGE 1
#define ARIR_IRQ 2
#define MEMA_DISABLE_ERASE 0x04
#define MEMA_VERIFY 0x08
#define CLTA_MEM_DRIVER 0x01
#define CLTA_MTD_DRIVER 0x02
#define CLTA_IO_DRIVER 0x04
#define CLTA_INSERT_SHARED 0x08
#define CLTA_INSERT_EXCLUSIVE 0x10
#define CLTA_INFO_SUBFUNCTION 0xFF00
#define CLTA_DRIVER_MASK 0x07
#define IOPA_SHARED 0x01
#define IOPA_FIRST_SHARED 0x02
#define IOPA_FORCE_ALIAS 0x04
#define IOPA_16BIT 0x08
#define MAX_IO_WDWS_PER_SKT 2
#define CFGA_EXCLUSIVELY_USED 0x01
#define CFGA_ENABLE_IRQ 0x02
#define CFGA_VALID_CLIENT 0x0100
#define CNFG_OPTION_VALID 0x01
#define CNFG_STATUS_VALID 0x02
#define CNFG_PIN_VALID 0x04
#define CNFG_COPY_VALID 0x08
#define CNFG_PRESENT_MASK 0x0F
#if VXD_WINVER >= 0x040A
#define CNFG_STATUS_SIGCHG 0x40
#define CNFG_STATUS_PWRDWN 0x04
#endif
#define EVTA_SOCKET_ONLY 0x01
#define EVTM_WRITE_PROTECT 0x01
#define EVTM_CARD_LOCK_CHG 0x02
#define EVTM_EJECT_REQUEST 0x04
#define EVTM_INSERT_REQUEST 0x08
#define EVTM_BATTERY_DEAD 0x10
#define EVTM_BATTERY_LOW 0x20
#define EVTM_READY_CHG 0x40
#define EVTM_CARD_DETECT_CHG 0x80
#define EVTM_PWR_MGMT_CHG 0x0100
#define EVTM_RESET_EVENTS 0x0200
#define EVTM_SS_UPDATED 0x0400
#define CLTA_SOCKET_ONLY 0x01
#define MEMA_ATTRIB_MEM 0x01
#define MEMA_VIRTUAL 0x0100
#define MEMA_WRITE_ERASE_MASK 0x0600
#define MEMA_WE_WRITE 0x0200
#define MEMA_WE_WRITE_ERASE 0x0400
#define MEMA_WE_DISABLE_ERASE 0x0600
#define MEMA_WRITE_VERIFY 0x0800
#define MEMA_ERASE_SUPPORTED 0x1000
#define MEMM_ACCESS_SPEED 0x01
#define MEMM_CARD_OFFSET 0x02
#define MEMM_PART_SIZE 0x04
#define MEMM_EFF_BLK_SIZE 0x08
#define MEMM_PART_MULTIPLE 0x10
#define MEMM_JEDEC_ID 0x20
#define MEMM_PART_TYPE 0x40
#define ASP_CODE_EXP 0x07
#define ASP_MANTISSA 0x71
#define ASP_WAIT 0x80
#define ASPC_250NS 0x01
#define ASPC_200NS 0x02
#define ASPC_150NS 0x03
#define ASPC_100NS 0x04
#define PRTT_NO_PART 0x00
#define PRTT_DOS_PART 0x01
#define PRTT_FFS1_PART 0x02
#define PRTT_FFS2_PART 0x03
#define PRTT_XIP_PART 0x04
#define PRTT_UNKNOWN_PART 0x7FFF
#define TPLA_RET_LINKS 0x01
#define TPLC_ANY_TUPLE 0xFF
#define CRDS_WRITE_PROTECTED 0x01
#define CRDS_CARD_LOCKED 0x02
#define CRDS_EJECT_REQUEST 0x04
#define CRDS_INSERT_REQUEST 0x08
#define CRDS_BATTERY_DEAD 0x10
#define CRDS_BATTERY_WARNING 0x20
#define CRDS_READY 0x40
#define CRDS_CARD_DETECTED 0x80
#define SKTS_WRITE_PROTECT 0x01
#define SKTS_CARD_LOCK 0x02
#define SKTS_EJECT_REQUEST 0x04
#define SKTS_INSERT_REQUEST 0x08
#define SKTS_BATTERY_DEAD 0x10
#define SKTS_BATTERY_WARNING 0x20
#define SKTS_READY 0x40
#define SKTS_CARD_DETECT 0x80
#define CFGA_IRQ_CHG 0x04
#define CFGA_VCC_CHG 0x08
#define CFGA_VPP1_CHG 0x10
#define CFGA_VPP2_CHG 0x20
#define WDWA_ATTRIB_MEM 0x02
#define WDWA_ENABLED 0x04
#define WDWA_SPEED_VALID 0x08
#define OPMA_ATTRIB_MEM 0x01
#define OPMA_EXCLUSIVE 0x02
#define EQES_QUEUED_FOR_ERASE 0x00
#define EQES_ERASE_IN_PROGRESS 0x01
#define EQES_MEDIA_MISSING 0x80
#define EQES_MEDIA_WRITE_PROTECTED 0x84
#define EQES_MEDIA_NOT_ERASABLE 0x86
#define EQES_MEDIA_NOT_WRITABLE 0x87
#define EQES_BAD_OFFSET 0xC1
#define EQES_BAD_TECHNOLOGY 0xC2
#define EQES_BAD_SOCKET 0xC3
#define EQES_BAD_VCC 0xC4
#define EQES_BAD_VPP 0xC5
#define EQES_BAD_SIZE 0xC6
#define EQES_ERASE_PASSED 0xE0
#define EQES_ERASE_FAILED 0xE1
#define EQES_IDLE 0xFF
#define MTDF_FUNC 0x03
#define MTDF_ERASE 0x00
#define MTDF_READ 0x01
#define MTDF_WRITE 0x02
#define MTDF_COPY 0x03
#define MTDF_DISABLE_ERASE 0x04
#define MTDF_VERIFY 0x08
#define MTDF_READY_CONT 0x10
#define MTDF_TIMEOUT_CONT 0x20
#define MTDF_FIRST 0x40
#define MTDF_LAST 0x80
#define MTDS_WAITREQ 0
#define MTDS_WAITTIMER 1
#define MTDS_WAITRDY 2
#define MTDS_WAITPOWER 3
#define IRQA_IRQ_TYPE_MASK 0x03
#define IRQA_EXCLUSIVE 0
#define IRQA_TIME_MUX 1
#define IRQA_DYNAMIC 2
#define IRQA_FORCE_PULSE 0x04
#define IRQA_FIRST_SHARED 0x08
#define IRQA_PULSE_ALLOC 0x0100
#define CFGI_MEMORY 0x01
#define CFGI_MEMORY_AND_IO 0x02
#define IRQI_NUMBER_MASK 0x0F
#define IRQI_NMI 0x01
#define IRQI_LOCK 0x02
#define IRQI_BERR 0x04
#define IRQI_VEND 0x08
#define IRQI_MASK 0x10
#define IRQI_LEVEL 0x20
#define IRQI_PULSE 0x40
#define IRQI_SHARE 0x80
#define WDWA_16BIT 0x08
#define WDWA_PAGED 0x10
#define WDWA_SHARED 0x20
#define WDWA_FIRST_SHARED 0x40
#define WDWA_OFFSETS_SIZED 0x0100
#define MEMA_DELETE_REGION 0x02
typedef WORD LOG_SOCKET;
typedef BYTE PHY_ADAPTER;
typedef BYTE PHY_SOCKET;
typedef BYTE PHY_WINDOW;
struct AccessConfigurationRegister_s {
  LOG_SOCKET Socket;
  BYTE Action;
  BYTE RegOffset;
  BYTE Value;
};
typedef struct AccessConfigurationRegister_s ACCESS_CONFIG_REG_PKT;
struct AddSSData_s {
  DWORD devnode;
};
typedef struct AddSSData_s ADD_SS_DATA;
struct AddSocketServices_s {
  WORD Attributes;
  ADD_SS_DATA *DataPointer;
};
typedef struct AddSocketServices_s ADD_SS_PKT;
struct CopyMemory_s {
  DWORD SourceOffset;
  DWORD DestOffset;
  DWORD Count;
  WORD Attributes;
};
typedef struct CopyMemory_s COPY_MEM_PKT;
struct GetCardServicesInfo_s {
  WORD InfoLen;
  BYTE Signature[2];
  WORD Count;
  WORD Revision;
  WORD CSLevel;
  WORD VStrOff;
  WORD VStrLen;
};
typedef struct GetCardServicesInfo_s GET_CS_INFO_PKT;
struct GetClientInfo_s {
  WORD MaxLen;
  WORD InfoLen;
  WORD Attributes;
  WORD Revision;
  WORD CSLevel;
  WORD RevDate;
  WORD NameOff;
  WORD NameLen;
  WORD VStringOff;
  WORD VStringLen;
};
typedef struct GetClientInfo_s GET_CLT_INFO_PKT;
struct IOParms_s {
  WORD BasePort;
  BYTE NumPorts;
  BYTE Attributes;
};
typedef struct IOParms_s IO_PARMS;
struct GetConfigurationInfo_s {
  LOG_SOCKET Socket;
  WORD Attributes;
  BYTE Vcc;
  BYTE Vpp1;
  BYTE Vpp2;
  BYTE IntType;
  DWORD ConfigBase;
  BYTE Status;
  BYTE Pin;
  BYTE Copy;
  BYTE ConfigIndex;
  BYTE Present;
  BYTE FirstDevType;
  BYTE FuncCode;
  BYTE SysInitMask;
  WORD ManufCode;
  WORD ManufInfo;
  BYTE CardValues;
  BYTE AssignedIRQ;
  WORD IRQAttributes;
  struct IOParms_s IOParms[2];
  BYTE IOAddrLines;
};
typedef struct GetConfigurationInfo_s GET_CONFIG_INFO_PKT;
struct EventMask_s {
  WORD Attributes;
  WORD EventMask;
  LOG_SOCKET Socket;
};
typedef struct EventMask_s EVT_MASK_PKT;
struct GetClient_s {
  LOG_SOCKET Socket;
  WORD Attributes;
};
typedef struct GetClient_s GET_CLT_PKT;
struct GetPartition_s {
  LOG_SOCKET Socket;
  WORD Attributes;
  BYTE TupleMask;
  BYTE AccessSpeed;
  WORD Flags;
  DWORD LinkOffset;
  DWORD CISOffset;
  DWORD CardOffset;
  DWORD PartSize;
  DWORD EffBlockSize;
  WORD PartMultiple;
  WORD JedecID;
  WORD PartType;
};
typedef struct GetPartition_s GET_PARTITION_PKT;
struct GetRegion_s {
  LOG_SOCKET Socket;
  WORD Attributes;
  BYTE TupleMask;
  BYTE AccessSpeed;
  WORD Flags;
  DWORD LinkOffset;
  DWORD CISOffset;
  DWORD CardOffset;
  DWORD RegionSize;
  DWORD EffBlockSize;
  WORD PartMultiple;
  WORD JedecID;
};
typedef struct GetRegion_s GET_REGION_PKT;
struct GetTuple_s {
  LOG_SOCKET Socket;
  WORD Attributes;
  BYTE DesiredTuple;
  BYTE Reserved;
  WORD Flags;
  DWORD LinkOffset;
  DWORD CISOffset;
  BYTE TupleCode;
  BYTE TupleLink;
};
typedef struct GetTuple_s GET_TUPLE_PKT;
struct GetStatus_s {
  LOG_SOCKET Socket;
  WORD CardState;
  WORD SocketState;
};
typedef struct GetStatus_s GET_STATUS_PKT;
struct GetTupleData_s {
  LOG_SOCKET Socket;
  WORD Attributes;
  BYTE DesiredTuple;
  BYTE TupleOffset;
  WORD Flags;
  DWORD LinkOffset;
  DWORD CISOffset;
  WORD TupleDataMax;
  WORD TupleDataLen;
};
typedef struct GetTupleData_s GET_TUPLE_DATA_PKT;
struct MapSocket_s {
  LOG_SOCKET LogSocket;
  PHY_ADAPTER PhyAdapter;
  PHY_SOCKET PhySocket;
};
typedef struct MapSocket_s MAP_SKT_PKT;
struct MapWindow_s {
  PHY_ADAPTER PhyAdapter;
  PHY_WINDOW PhyWindow;
};
typedef struct MapWindow_s MAP_WDW_PKT;
struct MapMemPage_s {
  DWORD CardOffset;
  BYTE PageNum;
};
typedef struct MapMemPage_s MAP_MEM_PAGE_PKT;
struct ModifyConfiguration_s {
  LOG_SOCKET Socket;
  WORD Attributes;
  BYTE Vcc;
  BYTE Vpp1;
  BYTE Vpp2;
};
typedef struct ModifyConfiguration_s MODIFY_CONFIG_PKT;
struct ModifyWindow_s {
  WORD Attributes;
  BYTE AccessSpeed;
};
typedef struct ModifyWindow_s MODIFY_WDW_PKT;
struct OpenMemory_s {
  LOG_SOCKET Socket;
  WORD Attributes;
  DWORD CardOffset;
};
typedef struct OpenMemory_s OPEN_MEM_PKT;
struct ReadMemory_s {
  DWORD CardOffset;
  DWORD Count;
};
typedef struct ReadMemory_s READ_MEM_PKT;
struct ClientData_s {
  WORD ReferenceData;
  WORD ReservedWord;
  DWORD DataOffset;
};
typedef struct ClientData_s CLIENT_DATA;
struct RegisterClient_s {
  WORD Attributes;
  WORD EventMask;
  struct ClientData_s ClientData;
  WORD Version;
};
typedef struct RegisterClient_s REGISTER_CLT_PKT;
struct QueueEntry_s {
  WORD MemoryHandle;
  BYTE EntryState;
  BYTE EraseSize;
  DWORD EraseOffset;
};
typedef struct QueueEntry_s ERASE_Q_ENTRY;
struct RegisterEraseQueue_s {
  WORD QueueEntryLen;
  WORD QueueEntryCnt;
};
typedef struct RegisterEraseQueue_s REGISTER_ERASE_Q_PKT;
struct RegisterMTD_s {
  LOG_SOCKET Socket;
  WORD Attributes;
  DWORD CardOffset;
  WORD MTDMediaID;
};
typedef struct RegisterMTD_s REGISTER_MTD_PKT;
struct MTDRequest_s {
  WORD PktLength;
  WORD Socket;
  DWORD SourceOffset;
  DWORD DestOffset;
  DWORD XferLength;
  UCHAR Function;
  UCHAR AccessSpeed;
  WORD MTDID;
  WORD MTDStatus;
  WORD TimeoutCount;
  void *MATDataPtr;
  void *CardSetAddressPtr;
  void *CardSetAutoIncPtr;
  void *CardReadBytePtr;
  void *CardReadWordPtr;
  void *CardReadWordsPtr;
  void *CardReadByteAIPtr;
  void *CardReadWordAIPtr;
  void *CardReadWordsAIPtr;
  void *CardWriteBytePtr;
  void *CardWriteWordPtr;
  void *CardWriteWordsPtr;
  void *CardWriteByteAIPtr;
  void *CardWriteWordAIPtr;
  void *CardWriteWordsAIPtr;
  void *CardCompareBytePtr;
  void *CardCompareByteAIPtr;
  void *CardCompareWordsPtr;
  void *CardCompareWordsAIPtr;
  void *MTDHelperEntryPtr;
};
typedef struct MTDRequest_s MTD_REQUEST;
struct MTDInit_s {
  LOG_SOCKET Socket;
  DWORD CardOffset;
};
typedef struct MTDInit_s MTD_INIT;
struct RegisterTimer_s {
  WORD WaitTicks;
};
typedef struct RegisterTimer_s REGISTER_TIMER_PKT;
struct ReleaseConfiguration_s {
  LOG_SOCKET Socket;
};
typedef struct ReleaseConfiguration_s RELEASE_CONFIG_PKT;
struct Exclusive_s {
  LOG_SOCKET Socket;
  WORD Attributes;
};
typedef struct Exclusive_s EXCLUSIVE_PKT;
struct ReleaseIO_s {
  LOG_SOCKET Socket;
  struct IOParms_s IOParms[2];
  BYTE IOAddrLines;
};
typedef struct ReleaseIO_s RELEASE_IO_PKT;
struct ReleaseIRQ_s {
  LOG_SOCKET Socket;
  WORD Attributes;
  BYTE AssignedIRQ;
};
typedef struct ReleaseIRQ_s RELEASE_IRQ_PKT;
struct ReleaseSocketMask_s {
  LOG_SOCKET Socket;
};
typedef struct ReleaseSocketMask_s RELEASE_SKT_MASK_PKT;
struct ReplaceSocketServices_s {
  LOG_SOCKET Socket;
  WORD NumSockets;
  WORD Attributes;
  void *DataPointer;
};
typedef struct ReplaceSocketServices_s REPLACE_SS_PKT;
struct RequestConfiguration_s {
  LOG_SOCKET Socket;
  WORD Attributes;
  BYTE Vcc;
  BYTE Vpp1;
  BYTE Vpp2;
  BYTE IntType;
  DWORD ConfigBase;
  BYTE Status;
  BYTE Pin;
  BYTE Copy;
  BYTE ConfigIndex;
  BYTE Present;
};
typedef struct RequestConfiguration_s REQUEST_CONFIG_PKT;
struct RequestIO_s {
  LOG_SOCKET Socket;
  struct IOParms_s IOParms[2];
  BYTE IOAddrLines;
};
typedef struct RequestIO_s REQUEST_IO_PKT;
struct RequestIRQ_s {
  LOG_SOCKET Socket;
  WORD Attributes;
  BYTE AssignedIRQ;
  BYTE IRQInfo1;
  WORD IRQInfo2;
};
typedef struct RequestIRQ_s REQUEST_IRQ_PKT;
struct RequestSocketMask_s {
  LOG_SOCKET Socket;
  WORD EventMask;
};
typedef struct RequestSocketMask_s REQUEST_SKT_MASK_PKT;
struct RequestWindow_s {
  LOG_SOCKET Socket;
  WORD Attributes;
  DWORD Base;
  DWORD MemSize;
  BYTE AccessSpeed;
};
typedef struct RequestWindow_s REQUEST_WDW_PKT;
struct ResetCard_s {
  LOG_SOCKET Socket;
  WORD Attributes;
};
typedef struct ResetCard_s RESET_CARD_PKT;
struct ReturnSSEntry_s {
  WORD Attributes;
};
typedef struct ReturnSSEntry_s RETURN_SS_ENTRY_PKT;
struct SetRegion_s {
  LOG_SOCKET Socket;
  WORD Attributes;
  DWORD CardOffset;
  DWORD RegionSize;
  DWORD EffBlockSize;
  WORD PartMultiple;
  WORD JedecID;
  DWORD BiasOffset;
  BYTE AccessSpeed;
};
typedef struct SetRegion_s SET_REGION_PKT;
struct ValidateCIS_s {
  LOG_SOCKET Socket;
  WORD Chains;
};
typedef struct ValidateCIS_s VALIDATE_CIS_PKT;
struct WriteMemory_s {
  DWORD CardOffset;
  DWORD Count;
  WORD Attributes;
};
typedef struct WriteMemory_s WRITE_MEM_PKT;

#pragma pack(pop)

#endif
