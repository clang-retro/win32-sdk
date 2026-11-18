/* 1394.h: The IEEE 1394 (FireWire) bus:
   the I/O request block (IRB) a 1394 client driver sends the bus driver
   in IOCTL_1394_CLASS requests, the REQUEST_* functions, address ranges,
   isochronous resources, and the bus and node structures. Windows 98,
   98 SE and ME (1394BUS.SYS) and 2000 and later take the same IRBs.

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 2000 and XP DDKs; each declaration is under the first
   _WIN32_WINNT whose DDK has it. */
#ifndef WDM_1394_H
#define WDM_1394_H

#include <ntddk.h>
#include <guiddef.h>
#if _WIN32_WINNT >= 0x0501
DEFINE_GUID(BUS1394_CLASS_GUID, 0x6bdd1fc1, 0x810f, 0x11d0, 0xbe, 0xc7, 0x08, 0x00, 0x2b, 0xe2, 0x09, 0x2f);
#endif
#if _WIN32_WINNT >= 0x0501
#define BUS1394_CURRENT_DDI_VERSION 2
#endif
#define IRP_MN_BUS_RESET 0x87
#define BASE_DEVICE_NAME L"\\Device\\1394BUS"
#define BASE_SYMBOLIC_LINK_NAME L"\\DosDevices\\1394BUS"
#define MAX_SUFFIX_SIZE 0x08
#define IOCTL_1394_CLASS 0x0022021D
#define IOCTL_1394_TOGGLE_ENUM_TEST_ON 0x00220220
#define IOCTL_1394_TOGGLE_ENUM_TEST_OFF 0x00220224
#define TCODE_WRITE_REQUEST_QUADLET 0
#define TCODE_WRITE_REQUEST_BLOCK 1
#define TCODE_WRITE_RESPONSE 2
#define TCODE_RESERVED1 3
#define TCODE_READ_REQUEST_QUADLET 4
#define TCODE_READ_REQUEST_BLOCK 5
#define TCODE_READ_RESPONSE_QUADLET 6
#define TCODE_READ_RESPONSE_BLOCK 7
#define TCODE_CYCLE_START 8
#define TCODE_LOCK_REQUEST 9
#define TCODE_ISOCH_DATA_BLOCK 10
#define TCODE_LOCK_RESPONSE 11
#define TCODE_RESERVED2 12
#define TCODE_RESERVED3 13
#define TCODE_SELFID 14
#define TCODE_RESERVED4 15
#define TCODE_REQUEST_BLOCK_MASK 1
#define TCODE_RESPONSE_MASK 2
#define EXT_TCODE_RESERVED0 0
#define EXT_TCODE_MASK_SWAP 1
#define EXT_TCODE_COMPARE_SWAP 2
#define EXT_TCODE_FETCH_ADD 3
#define EXT_TCODE_LITTLE_ADD 4
#define EXT_TCODE_BOUNDED_ADD 5
#define EXT_TCODE_WRAP_ADD 6
#define ACODE_RESERVED_0 0
#define ACODE_ACK_COMPLETE 1
#define ACODE_ACK_PENDING 2
#define ACODE_RESERVED_3 3
#define ACODE_ACK_BUSY_X 4
#define ACODE_ACK_BUSY_A 5
#define ACODE_ACK_BUSY_B 6
#define ACODE_RESERVED_7 7
#define ACODE_RESERVED_8 8
#define ACODE_RESERVED_9 9
#define ACODE_RESERVED_10 10
#define ACODE_RESERVED_11 11
#define ACODE_RESERVED_12 12
#define ACODE_ACK_DATA_ERROR 13
#define ACODE_ACK_TYPE_ERROR 14
#define ACODE_RESERVED_15 15
#define ACODE_STATUS_MASK (-1072562064)
#define RCODE_RESPONSE_COMPLETE 0
#define RCODE_RESERVED1 1
#define RCODE_RESERVED2 2
#define RCODE_RESERVED3 3
#define RCODE_CONFLICT_ERROR 4
#define RCODE_DATA_ERROR 5
#define RCODE_TYPE_ERROR 6
#define RCODE_ADDRESS_ERROR 7
#define RCODE_TIMED_OUT 15
#define RCODE_STATUS_MASK (-1072562048)
#if _WIN32_WINNT >= 0x0501
#define STATUS_INVALID_GENERATION (-1072562032)
#else
#define STATUS_INVALID_GENERATION (-1072529264)
#endif
#define SCODE_100_RATE 0
#define SCODE_200_RATE 1
#define SCODE_400_RATE 2
#define SCODE_800_RATE 3
#define SCODE_1600_RATE 4
#define SCODE_3200_RATE 5
#define SELF_ID_CONNECTED_TO_CHILD 3
#define SELF_ID_CONNECTED_TO_PARENT 2
#define SELF_ID_NOT_CONNECTED 1
#define SELF_ID_NOT_PRESENT 0
#define POWER_CLASS_NOT_NEED_NOT_REPEAT 0
#define POWER_CLASS_SELF_POWER_PROVIDE_15W 1
#define POWER_CLASS_SELF_POWER_PROVIDE_30W 2
#define POWER_CLASS_SELF_POWER_PROVIDE_45W 3
#define POWER_CLASS_MAYBE_POWERED_UPTO_1W 4
#define POWER_CLASS_IS_POWERED_UPTO_1W_NEEDS_2W 5
#define POWER_CLASS_IS_POWERED_UPTO_1W_NEEDS_5W 6
#define POWER_CLASS_IS_POWERED_UPTO_1W_NEEDS_9W 7
#define PHY_PACKET_ID_CONFIGURATION 0
#define PHY_PACKET_ID_LINK_ON 1
#define PHY_PACKET_ID_SELF_ID 2
#define INITIAL_REGISTER_SPACE_HI 0xFFFF
#define INITIAL_REGISTER_SPACE_LO 0xF0000000
#define STATE_CLEAR_LOCATION 0x00
#define STATE_SET_LOCATION 0x04
#define NODE_IDS_LOCATION 0x08
#define RESET_START_LOCATION 0x0C
#define SPLIT_TIMEOUT_HI_LOCATION 0x18
#define SPLIT_TIMEOUT_LO_LOCATION 0x1C
#define INTERRUPT_TARGET_LOCATION 0x50
#define INTERRUPT_MASK_LOCATION 0x54
#define CYCLE_TIME_LOCATION 0x0200
#define BUS_TIME_LOCATION 0x0204
#define POWER_FAIL_IMMINENT_LOCATION 0x0208
#define POWER_SOURCE_LOCATION 0x020C
#define BUSY_TIMEOUT_LOCATION 0x0210
#define BUS_MANAGER_ID_LOCATION 0x021C
#define BANDWIDTH_AVAILABLE_LOCATION 0x0220
#define CHANNELS_AVAILABLE_LOCATION 0x0224
#define NETWORK_CHANNELS_LOCATION 0x0234
#define CONFIG_ROM_LOCATION 0x0400
#define TOPOLOGY_MAP_LOCATION 0x1000
#define SPEED_MAP_LOCATION 0x2000
#define CONFIG_ROM_KEY_MASK 0xFF
#define CONFIG_ROM_OFFSET_MASK 0xFFFFFF00
#define MODULE_VENDOR_ID_KEY_SIGNATURE 0x03
#define NODE_CAPABILITIES_KEY_SIGNATURE 0x0C
#define SPEC_ID_KEY_SIGNATURE 0x12
#define SOFTWARE_VERSION_KEY_SIGNATURE 0x13
#define MODEL_ID_KEY_SIGNATURE 0x17
#define COMMAND_BASE_KEY_SIGNATURE 0x40
#define VENDOR_KEY_SIGNATURE 0x81
#define TEXTUAL_LEAF_INDIRECT_KEY_SIGNATURE 0x81
#define MODEL_KEY_SIGNATURE 0x82
#define UNIT_DIRECTORY_KEY_SIGNATURE 0xD1
#define UNIT_DEP_DIR_KEY_SIGNATURE 0xD4
#define ASYNC_PAYLOAD_100_RATE 512
#define ASYNC_PAYLOAD_200_RATE 1024
#define ASYNC_PAYLOAD_400_RATE 2048
#define ISOCH_PAYLOAD_50_RATE 512
#define ISOCH_PAYLOAD_100_RATE 1024
#define ISOCH_PAYLOAD_200_RATE 2048
#define ISOCH_PAYLOAD_400_RATE 4096
#define ISOCH_PAYLOAD_800_RATE 8192
#define ISOCH_PAYLOAD_1600_RATE 16384
#define S100_BW_UNITS_PER_QUADLET 19
#define S200_BW_UNITS_PER_QUADLET 9
#define S400_BW_UNITS_PER_QUADLET 4
#define S800_BW_UNITS_PER_QUADLET 2
#define S1600_BW_UNITS_PER_QUADLET 1
#define INITIAL_BANDWIDTH_UNITS 4915
#define MAX_REC_100_RATE 0x08
#define MAX_REC_200_RATE 0x09
#define MAX_REC_400_RATE 0x0A
#define LOCAL_BUS 0x03FF
#define MAX_LOCAL_NODES 64
#define SELFID_PACKET_SIGNITURE 2
#define NOMINAL_CYCLE_TIME 125
#define NO_BUS_MANAGER 0x3F
#define SPEED_MAP_LENGTH 0x03F1
#define DEVICE_EXTENSION_TAG 0xDEADBEEF
#define VIRTUAL_DEVICE_EXTENSION_TAG 0xDEAFBEEF
#define PORT_EXTENSION_TAG 0xDEAFCAFE
#define BUS_EXTENSION_TAG 0xABACADAB
#define ISOCH_RESOURCE_TAG 0xBABEFACE
#define BANDWIDTH_ALLOCATE_TAG 0xFEEDBEAD
#define CONFIG_ROM_SIGNATURE 0x31333934
#define REQUEST_ASYNC_READ 0
#define REQUEST_ASYNC_WRITE 1
#define REQUEST_ASYNC_LOCK 2
#define REQUEST_ISOCH_ALLOCATE_BANDWIDTH 3
#define REQUEST_ISOCH_ALLOCATE_CHANNEL 4
#define REQUEST_ISOCH_ALLOCATE_RESOURCES 5
#define REQUEST_ISOCH_ATTACH_BUFFERS 6
#define REQUEST_ISOCH_DETACH_BUFFERS 7
#define REQUEST_ISOCH_FREE_BANDWIDTH 8
#define REQUEST_ISOCH_FREE_CHANNEL 9
#define REQUEST_ISOCH_FREE_RESOURCES 10
#define REQUEST_ISOCH_LISTEN 11
#define REQUEST_ISOCH_STOP 12
#define REQUEST_ISOCH_TALK 13
#define REQUEST_ISOCH_QUERY_CYCLE_TIME 14
#define REQUEST_ISOCH_QUERY_RESOURCES 15
#define REQUEST_ISOCH_SET_CHANNEL_BANDWIDTH 16
#define REQUEST_ALLOCATE_ADDRESS_RANGE 17
#define REQUEST_FREE_ADDRESS_RANGE 18
#define REQUEST_GET_LOCAL_HOST_INFO 19
#define REQUEST_GET_ADDR_FROM_DEVICE_OBJECT 20
#define REQUEST_CONTROL 21
#define REQUEST_GET_SPEED_BETWEEN_DEVICES 22
#define REQUEST_SET_DEVICE_XMIT_PROPERTIES 23
#define REQUEST_GET_CONFIGURATION_INFO 24
#define REQUEST_BUS_RESET 25
#define REQUEST_GET_GENERATION_COUNT 26
#define REQUEST_SEND_PHY_CONFIG_PACKET 27
#define REQUEST_GET_SPEED_TOPOLOGY_MAPS 28
#define REQUEST_BUS_RESET_NOTIFICATION 29
#define REQUEST_ASYNC_STREAM 30
#define REQUEST_SET_LOCAL_HOST_PROPERTIES 31
#if _WIN32_WINNT >= 0x0501
#define REQUEST_ISOCH_MODIFY_STREAM_PROPERTIES 32
#endif
#define IRB_BUS_RESERVED_SZ 8
#define IRB_PORT_RESERVED_SZ 8
#define IRB_FLAG_USE_PRE_CALCULATED_VALUE 1
#define IRB_FLAG_ALLOW_REMOTE_FREE 2
#define PORT_SUPPORTS_NOTHING 0
#define PORT_SUPPORTS_CSRS 1
#define PORT_SUPPORTS_LARGE_ASYNC 2
#define PORT_SUPPORTS_NATIVE_ENDIAN 4
#if _WIN32_WINNT >= 0x0501
#define PORT_SUPPORTS_WMI 8
#endif
#define SET_LOCAL_HOST_PROPERTIES_NO_CYCLE_STARTS 0x01
#if _WIN32_WINNT >= 0x0501
#define SET_LOCAL_HOST_PROPERTIES_CYCLE_START_CONTROL 0x01
#endif
#define SET_LOCAL_HOST_PROPERTIES_GAP_COUNT 0x02
#define SET_LOCAL_HOST_PROPERTIES_MODIFY_CROM 0x03
#if _WIN32_WINNT >= 0x0501
#define SET_LOCAL_HOST_PROPERTIES_MAX_PAYLOAD 0x04
#endif
#define SLHP_FLAG_ADD_CROM_DATA 0x01
#define SLHP_FLAG_REMOVE_CROM_DATA 0x02
#define ASYNC_FLAGS_NONINCREMENTING 0x01
#define ASYNC_FLAGS_PARTIAL_REQUEST 0x80000000
#define ASYNC_FLAGS_NO_STATUS 0x02
#define ASYNC_FLAGS_PING 0x04
#define ASYNC_FLAGS_BROADCAST 0x08
#define ACCESS_FLAGS_TYPE_READ 1
#define ACCESS_FLAGS_TYPE_WRITE 2
#define ACCESS_FLAGS_TYPE_LOCK 4
#define ACCESS_FLAGS_TYPE_BROADCAST 8
#define NOTIFY_FLAGS_NEVER 0
#define NOTIFY_FLAGS_AFTER_READ 1
#define NOTIFY_FLAGS_AFTER_WRITE 2
#define NOTIFY_FLAGS_AFTER_LOCK 4
#define SPEED_FLAGS_100 0x01
#define SPEED_FLAGS_200 0x02
#define SPEED_FLAGS_400 0x04
#define SPEED_FLAGS_800 0x08
#define SPEED_FLAGS_1600 0x10
#define SPEED_FLAGS_3200 0x20
#define SPEED_FLAGS_FASTEST 0x80000000
#define ISOCH_ANY_CHANNEL 0xFFFFFFFF
#define ISOCH_MAX_CHANNEL 63
#define BUS_RESET_FLAGS_PERFORM_RESET 1
#define BUS_RESET_FLAGS_FORCE_ROOT 2
#if _WIN32_WINNT >= 0x0501
#define BUS_RESET_BEGINNING 0x01
#else
#define BUS_RESET_BEGINNING 1
#endif
#if _WIN32_WINNT >= 0x0501
#define BUS_RESET_FINISHED 0x02
#else
#define BUS_RESET_FINISHED 2
#endif
#if _WIN32_WINNT >= 0x0501
#define BUS_RESET_LOCAL_NODE_IS_ROOT 0x04
#else
#define BUS_RESET_LOCAL_NODE_IS_ROOT 4
#endif
#if _WIN32_WINNT >= 0x0501
#define BUS_RESET_LOCAL_NODE_IS_ISOCH_MANAGER 0x08
#else
#define BUS_RESET_LOCAL_NODE_IS_ISOCH_MANAGER 8
#endif
#if _WIN32_WINNT >= 0x0501
#define BUS_RESET_LOCAL_NODE_IS_BUS_MANAGER 0x10
#else
#define BUS_RESET_LOCAL_NODE_IS_BUS_MANAGER 16
#endif
#if _WIN32_WINNT >= 0x0501
#define BUS_RESET_SELFID_ENUMERATION_ERROR 0x20
#else
#define BUS_RESET_SELFID_ENUMERATION_ERROR 32
#endif
#if _WIN32_WINNT >= 0x0501
#define BUS_RESET_STORM_ERROR 0x40
#define BUS_RESET_ABSENT_ON_POWER_UP 0x80
#define BUS_RESET_UNOPTIMIZED_TOPOLOGY 0x0100
#endif
#define LOCK_TRANSACTION_MASK_SWAP 1
#define LOCK_TRANSACTION_COMPARE_SWAP 2
#define LOCK_TRANSACTION_FETCH_ADD 3
#define LOCK_TRANSACTION_LITTLE_ADD 4
#define LOCK_TRANSACTION_BOUNDED_ADD 5
#define LOCK_TRANSACTION_WRAP_ADD 6
#define RESOURCE_USED_IN_LISTENING 0x01
#define RESOURCE_USED_IN_TALKING 0x02
#define RESOURCE_BUFFERS_CIRCULAR 0x04
#define RESOURCE_STRIP_ADDITIONAL_QUADLETS 0x08
#define RESOURCE_TIME_STAMP_ON_COMPLETION 0x10
#define RESOURCE_SYNCH_ON_TIME 0x20
#define RESOURCE_USE_PACKET_BASED 0x40
#define RESOURCE_VARIABLE_ISOCH_PAYLOAD 0x80
#if _WIN32_WINNT >= 0x0501
#define RESOURCE_USE_MULTICHANNEL 0x0100
#endif
#define DESCRIPTOR_SYNCH_ON_SY 0x01
#define DESCRIPTOR_SYNCH_ON_TAG 0x02
#define DESCRIPTOR_SYNCH_ON_TIME 0x04
#define DESCRIPTOR_USE_SY_TAG_IN_FIRST 0x08
#define DESCRIPTOR_TIME_STAMP_ON_COMPLETION 0x10
#define DESCRIPTOR_PRIORITY_TIME_DELIVERY 0x20
#define DESCRIPTOR_HEADER_SCATTER_GATHER 0x40
#define DESCRIPTOR_HEADER_RX_SCATTER_GATHER 0x80
#if _WIN32_WINNT >= 0x0501
#define DESCRIPTOR_SYNCH_ON_ALL_TAGS 0x80
#endif
#define SYNCH_ON_SY 0x01
#define SYNCH_ON_TAG 0x02
#define SYNCH_ON_TIME 0x04
#define GET_HOST_UNIQUE_ID 1
#define GET_HOST_CAPABILITIES 2
#define GET_POWER_SUPPLIED 3
#define GET_PHYS_ADDR_ROUTINE 4
#define GET_HOST_CONFIG_ROM 5
#define GET_HOST_CSR_CONTENTS 6
#define GET_HOST_DMA_CAPABILITIES 7
#define HOST_INFO_PACKET_BASED 0x01
#define HOST_INFO_STREAM_BASED 0x02
#define HOST_INFO_SUPPORTS_ISOCH_STRIPPING 0x04
#define HOST_INFO_SUPPORTS_START_ON_CYCLE 0x08
#define HOST_INFO_SUPPORTS_RETURNING_ISO_HDR 0x10
#define HOST_INFO_SUPPORTS_ISO_HDR_INSERTION 0x20
#define HOST_INFO_SUPPORTS_DV_CIP_STRIPPING 0x40
#if _WIN32_WINNT >= 0x0501
#define HOST_INFO_SUPPORTS_ISO_DUAL_BUFFER_RX 0x40
#define HOST_INFO_DMA_DOUBLE_BUFFERING_ENABLED 0x80
#endif
#define USE_LOCAL_NODE 1
#define BUS_RESPONSE_IS_RAW 1
#define REGISTER_NOTIFICATION_ROUTINE 1
#define DEREGISTER_NOTIFICATION_ROUTINE 2
#define BIG_ENDIAN_ADDRESS_RANGE 1
#if _WIN32_WINNT >= 0x0501
#define ALLOCATE_ADDRESS_FLAGS_USE_BIG_ENDIAN 1
#define ALLOCATE_ADDRESS_FLAGS_USE_COMMON_BUFFER 2
#endif
#pragma pack(push, 2)
struct _NODE_ADDRESS {
  USHORT NA_Node_Number : 6;
  USHORT NA_Bus_Number : 10;
};
#pragma pack(pop)
typedef struct _NODE_ADDRESS NODE_ADDRESS;
typedef struct _NODE_ADDRESS *PNODE_ADDRESS;
struct _ADDRESS_OFFSET {
  USHORT Off_High;
  ULONG Off_Low;
};
typedef struct _ADDRESS_OFFSET ADDRESS_OFFSET;
typedef struct _ADDRESS_OFFSET *PADDRESS_OFFSET;
struct _IO_ADDRESS {
  NODE_ADDRESS IA_Destination_ID;
  ADDRESS_OFFSET IA_Destination_Offset;
};
typedef struct _IO_ADDRESS IO_ADDRESS;
typedef struct _IO_ADDRESS *PIO_ADDRESS;
struct _ADDRESS_RANGE {
  USHORT AR_Off_High;
  USHORT AR_Length;
  ULONG AR_Off_Low;
};
typedef struct _ADDRESS_RANGE ADDRESS_RANGE;
typedef struct _ADDRESS_RANGE *PADDRESS_RANGE;
struct _SELF_ID {
  ULONG SID_Phys_ID : 6;
  ULONG SID_Packet_ID : 2;
  ULONG SID_Gap_Count : 6;
  ULONG SID_Link_Active : 1;
  ULONG SID_Zero : 1;
  ULONG SID_Power_Class : 3;
  ULONG SID_Contender : 1;
  ULONG SID_Delay : 2;
  ULONG SID_Speed : 2;
  ULONG SID_More_Packets : 1;
  ULONG SID_Initiated_Rst : 1;
  ULONG SID_Port3 : 2;
  ULONG SID_Port2 : 2;
  ULONG SID_Port1 : 2;
};
typedef struct _SELF_ID SELF_ID;
typedef struct _SELF_ID *PSELF_ID;
struct _SELF_ID_MORE {
  ULONG SID_Phys_ID : 6;
  ULONG SID_Packet_ID : 2;
  ULONG SID_PortA : 2;
  ULONG SID_Reserved2 : 2;
  ULONG SID_Sequence : 3;
  ULONG SID_One : 1;
  ULONG SID_PortE : 2;
  ULONG SID_PortD : 2;
  ULONG SID_PortC : 2;
  ULONG SID_PortB : 2;
  ULONG SID_More_Packets : 1;
  ULONG SID_Reserved3 : 1;
  ULONG SID_PortH : 2;
  ULONG SID_PortG : 2;
  ULONG SID_PortF : 2;
};
typedef struct _SELF_ID_MORE SELF_ID_MORE;
typedef struct _SELF_ID_MORE *PSELF_ID_MORE;
struct _PHY_CONFIGURATION_PACKET {
  ULONG PCP_Phys_ID : 6;
  ULONG PCP_Packet_ID : 2;
  ULONG PCP_Gap_Count : 6;
  ULONG PCP_Set_Gap_Count : 1;
  ULONG PCP_Force_Root : 1;
  ULONG PCP_Reserved1 : 8;
  ULONG PCP_Reserved2 : 8;
  ULONG PCP_Inverse;
};
typedef struct _PHY_CONFIGURATION_PACKET PHY_CONFIGURATION_PACKET;
typedef struct _PHY_CONFIGURATION_PACKET *PPHY_CONFIGURATION_PACKET;
struct _ASYNC_PACKET {
  USHORT AP_Priority : 4;
  USHORT AP_tCode : 4;
  USHORT AP_rt : 2;
  USHORT AP_tLabel : 6;
  NODE_ADDRESS AP_Destination_ID;
  union {
    struct {
      USHORT AP_Reserved : 12;
      USHORT AP_Rcode : 4;
    } Response;
    USHORT AP_Offset_High;
  } u;
  NODE_ADDRESS AP_Source_ID;
  ULONG AP_Offset_Low;
  union {
    struct {
      USHORT AP_Extended_tCode;
      USHORT AP_Data_Length;
    } Block;
    ULONG AP_Quadlet_Data;
  } u1;
};
typedef struct _ASYNC_PACKET ASYNC_PACKET;
typedef struct _ASYNC_PACKET *PASYNC_PACKET;
struct _ISOCH_HEADER {
  ULONG IH_Sy : 4;
  ULONG IH_tCode : 4;
  ULONG IH_Channel : 6;
  ULONG IH_Tag : 2;
  ULONG IH_Data_Length : 16;
};
typedef struct _ISOCH_HEADER ISOCH_HEADER;
typedef struct _ISOCH_HEADER *PISOCH_HEADER;
#if _WIN32_WINNT >= 0x0501
struct _TOPOLOGY_MAP {
  USHORT TOP_Length;
  USHORT TOP_CRC;
  ULONG TOP_Generation;
  USHORT TOP_Node_Count;
  USHORT TOP_Self_ID_Count;
  SELF_ID TOP_Self_ID_Array[1];
};
#else
struct _TOPOLOGY_MAP {
  USHORT TOP_Length;
  USHORT TOP_CRC;
  ULONG TOP_Generation;
  USHORT TOP_Node_Count;
  USHORT TOP_Self_ID_Count;
  SELF_ID TOP_Self_ID_Array[];
};
#endif
typedef struct _TOPOLOGY_MAP TOPOLOGY_MAP;
typedef struct _TOPOLOGY_MAP *PTOPOLOGY_MAP;
struct _SPEED_MAP {
  USHORT SPD_Length;
  USHORT SPD_CRC;
  ULONG SPD_Generation;
  UCHAR SPD_Speed_Code[4032];
};
typedef struct _SPEED_MAP SPEED_MAP;
typedef struct _SPEED_MAP *PSPEED_MAP;
struct _CONFIG_ROM {
  ULONG CR_Info;
  ULONG CR_Signiture;
  ULONG CR_BusInfoBlockCaps;
  ULONG CR_Node_UniqueID[2];
  ULONG CR_Root_Info;
};
typedef struct _CONFIG_ROM CONFIG_ROM;
typedef struct _CONFIG_ROM *PCONFIG_ROM;
struct _NETWORK_CHANNELS {
  ULONG NC_Channel : 6;
  ULONG NC_Reserved : 18;
  ULONG NC_Npm_ID : 6;
  ULONG NC_Valid : 1;
  ULONG NC_One : 1;
};
typedef struct _NETWORK_CHANNELS NETWORK_CHANNELSR;
typedef struct _NETWORK_CHANNELS *PNETWORK_CHANNELS;
struct _TEXTUAL_LEAF {
  USHORT TL_CRC;
  USHORT TL_Length;
  ULONG TL_Spec_Id;
  ULONG TL_Language_Id;
  UCHAR TL_Data;
};
typedef struct _TEXTUAL_LEAF TEXTUAL_LEAF;
typedef struct _TEXTUAL_LEAF *PTEXTUAL_LEAF;
struct _CYCLE_TIME {
  ULONG CL_CycleOffset : 12;
  ULONG CL_CycleCount : 13;
  ULONG CL_SecondCount : 7;
};
typedef struct _CYCLE_TIME CYCLE_TIME;
typedef struct _CYCLE_TIME *PCYCLE_TIME;
struct _ADDRESS_FIFO {
  SINGLE_LIST_ENTRY FifoList;
  PMDL FifoMdl;
};
typedef struct _ADDRESS_FIFO ADDRESS_FIFO;
typedef struct _ADDRESS_FIFO *PADDRESS_FIFO;
#if _WIN32_WINNT >= 0x0501
struct _NOTIFICATION_INFO {
  PMDL Mdl;
  ULONG ulOffset;
  ULONG nLength;
  ULONG fulNotificationOptions;
  PVOID Context;
  PADDRESS_FIFO Fifo;
  PVOID RequestPacket;
  PMDL ResponseMdl;
  PVOID *ResponsePacket;
  PULONG ResponseLength;
  PKEVENT *ResponseEvent;
  ULONG ResponseCode;
};
#else
struct _NOTIFICATION_INFO {
  PMDL Mdl;
  ULONG ulOffset;
  ULONG nLength;
  ULONG fulNotificationOptions;
  PVOID Context;
  PADDRESS_FIFO Fifo;
  PVOID RequestPacket;
  PMDL ResponseMdl;
  PVOID *ResponsePacket;
  PULONG ResponseLength;
  PKEVENT *ResponseEvent;
};
#endif
typedef struct _NOTIFICATION_INFO NOTIFICATION_INFO;
typedef struct _NOTIFICATION_INFO *PNOTIFICATION_INFO;
typedef void (*PBUS_NOTIFICATION_ROUTINE)(PNOTIFICATION_INFO) __attribute__((stdcall));
typedef void (*PBUS_ISOCH_DESCRIPTOR_ROUTINE)(PVOID, PVOID) __attribute__((stdcall));
typedef void (*PBUS_BUS_RESET_NOTIFICATION)(PVOID) __attribute__((stdcall));
struct _NODE_DEVICE_EXTENSION {
  ULONG Tag;
  BOOLEAN bConfigurationInformationValid;
  PCONFIG_ROM ConfigRom;
  ULONG UnitDirectoryLength;
  PVOID UnitDirectory;
  IO_ADDRESS UnitDirectoryLocation;
  ULONG UnitDependentDirectoryLength;
  PVOID UnitDependentDirectory;
  IO_ADDRESS UnitDependentDirectoryLocation;
  ULONG VendorLeafLength;
  PTEXTUAL_LEAF VendorLeaf;
  ULONG ModelLeafLength;
  PTEXTUAL_LEAF ModelLeaf;
  NODE_ADDRESS NodeAddress;
  UCHAR Speed;
  UCHAR Priority;
  PIRP Irp;
  PDEVICE_OBJECT DeviceObject;
  PDEVICE_OBJECT PortDeviceObject;
  PVOID DeviceInformation;
  PBUS_BUS_RESET_NOTIFICATION ResetRoutine;
  PVOID ResetContext;
};
typedef struct _NODE_DEVICE_EXTENSION NODE_DEVICE_EXTENSION;
typedef struct _NODE_DEVICE_EXTENSION *PNODE_DEVICE_EXTENSION;
struct _ISOCH_DESCRIPTOR {
  ULONG fulFlags;
  PMDL Mdl;
  ULONG ulLength;
  ULONG nMaxBytesPerFrame;
  ULONG ulSynch;
  ULONG ulTag;
  CYCLE_TIME CycleTime;
  PBUS_ISOCH_DESCRIPTOR_ROUTINE Callback;
  PVOID Context1;
  PVOID Context2;
  NTSTATUS status;
  ULONG_PTR DeviceReserved[8];
  ULONG_PTR BusReserved[8];
  ULONG_PTR PortReserved[16];
};
typedef struct _ISOCH_DESCRIPTOR ISOCH_DESCRIPTOR;
typedef struct _ISOCH_DESCRIPTOR *PISOCH_DESCRIPTOR;
#pragma pack(push, 2)
struct _IEEE1394_SCATTER_GATHER_HEADER {
  USHORT HeaderLength;
  USHORT DataLength;
  UCHAR HeaderData;
};
#pragma pack(pop)
typedef struct _IEEE1394_SCATTER_GATHER_HEADER IEEE1394_SCATTER_GATHER_HEADER;
typedef struct _IEEE1394_SCATTER_GATHER_HEADER *PIEEE1394_SCATTER_GATHER_HEADER;
struct _BANDWIDTH_ALLOCATION {
  LIST_ENTRY AllocationList;
  ULONG Tag;
  ULONG OwnedUnits;
  ULONG fulSpeed;
  BOOLEAN bRemoteAllocation;
  ULONG Generation;
  PNODE_DEVICE_EXTENSION DeviceExtension;
};
typedef struct _BANDWIDTH_ALLOCATION BANDWIDTH_ALLOCATION;
typedef struct _BANDWIDTH_ALLOCATION *PBANDWIDTH_ALLOCATION;
#if _WIN32_WINNT >= 0x0501
struct _IRB {
  ULONG FunctionNumber;
  ULONG Flags;
  ULONG_PTR BusReserved[8];
  ULONG_PTR PortReserved[8];
  union {
    struct {
      IO_ADDRESS DestinationAddress;
      ULONG nNumberOfBytesToRead;
      ULONG nBlockSize;
      ULONG fulFlags;
      PMDL Mdl;
      ULONG ulGeneration;
      UCHAR chPriority;
      UCHAR nSpeed;
      UCHAR tCode;
      UCHAR Reserved;
      ULONG ElapsedTime;
    } AsyncRead;
    struct {
      IO_ADDRESS DestinationAddress;
      ULONG nNumberOfBytesToWrite;
      ULONG nBlockSize;
      ULONG fulFlags;
      PMDL Mdl;
      ULONG ulGeneration;
      UCHAR chPriority;
      UCHAR nSpeed;
      UCHAR tCode;
      UCHAR Reserved;
      ULONG ElapsedTime;
    } AsyncWrite;
    struct {
      IO_ADDRESS DestinationAddress;
      ULONG nNumberOfArgBytes;
      ULONG nNumberOfDataBytes;
      ULONG fulTransactionType;
      ULONG fulFlags;
      ULONG Arguments[2];
      ULONG DataValues[2];
      PVOID pBuffer;
      ULONG ulGeneration;
      UCHAR chPriority;
      UCHAR nSpeed;
      UCHAR tCode;
      UCHAR Reserved;
    } AsyncLock;
    struct {
      ULONG nMaxBytesPerFrameRequested;
      ULONG fulSpeed;
      HANDLE hBandwidth;
      ULONG BytesPerFrameAvailable;
      ULONG SpeedSelected;
      ULONG nBandwidthUnitsRequired;
    } IsochAllocateBandwidth;
    struct {
      ULONG nRequestedChannel;
      ULONG Channel;
      LARGE_INTEGER ChannelsAvailable;
    } IsochAllocateChannel;
    struct {
      ULONG fulSpeed;
      ULONG fulFlags;
      ULONG nChannel;
      ULONG nMaxBytesPerFrame;
      ULONG nNumberOfBuffers;
      ULONG nMaxBufferSize;
      ULONG nQuadletsToStrip;
      HANDLE hResource;
      ULARGE_INTEGER ChannelMask;
    } IsochAllocateResources;
    struct {
      HANDLE hResource;
      ULONG nNumberOfDescriptors;
      PISOCH_DESCRIPTOR pIsochDescriptor;
    } IsochAttachBuffers;
    struct {
      HANDLE hResource;
      ULONG nNumberOfDescriptors;
      PISOCH_DESCRIPTOR pIsochDescriptor;
    } IsochDetachBuffers;
    struct {
      HANDLE hBandwidth;
      ULONG nMaxBytesPerFrameRequested;
      ULONG fulSpeed;
      ULONG BytesPerFrameAvailable;
      ULONG SpeedSelected;
      ULONG nBandwidthUnitsRequired;
    } IsochFreeBandwidth;
    struct {
      ULONG nChannel;
    } IsochFreeChannel;
    struct {
      HANDLE hResource;
    } IsochFreeResources;
    struct {
      HANDLE hResource;
      ULONG fulFlags;
      CYCLE_TIME StartTime;
    } IsochListen;
    struct {
      CYCLE_TIME CycleTime;
    } IsochQueryCurrentCycleTime;
    struct {
      ULONG fulSpeed;
      ULONG BytesPerFrameAvailable;
      LARGE_INTEGER ChannelsAvailable;
    } IsochQueryResources;
    struct {
      HANDLE hBandwidth;
      ULONG nMaxBytesPerFrame;
      ULONG nBandwidthUnitsRequired;
    } IsochSetChannelBandwidth;
    struct {
      HANDLE hResource;
      ULONG fulFlags;
    } IsochStop;
    struct {
      HANDLE hResource;
      ULONG fulFlags;
      CYCLE_TIME StartTime;
    } IsochTalk;
    struct {
      HANDLE hResource;
      ULARGE_INTEGER ChannelMask;
      ULONG fulSpeed;
    } IsochModifyStreamProperties;
    struct {
      PMDL Mdl;
      ULONG fulFlags;
      ULONG nLength;
      ULONG MaxSegmentSize;
      ULONG fulAccessType;
      ULONG fulNotificationOptions;
      PVOID Callback;
      PVOID Context;
      ADDRESS_OFFSET Required1394Offset;
      PSLIST_HEADER FifoSListHead;
      PKSPIN_LOCK FifoSpinLock;
      ULONG AddressesReturned;
      PADDRESS_RANGE p1394AddressRange;
      HANDLE hAddressRange;
      PVOID DeviceExtension;
    } AllocateAddressRange;
    struct {
      ULONG nAddressesToFree;
      PADDRESS_RANGE p1394AddressRange;
      PHANDLE pAddressRange;
      PVOID DeviceExtension;
    } FreeAddressRange;
    struct {
      ULONG nLevel;
      PVOID Information;
    } GetLocalHostInformation;
    struct {
      ULONG fulFlags;
      NODE_ADDRESS NodeAddress;
    } Get1394AddressFromDeviceObject;
    struct {
      ULONG ulIoControlCode;
      PMDL pInBuffer;
      ULONG ulInBufferLength;
      PMDL pOutBuffer;
      ULONG ulOutBufferLength;
      ULONG BytesReturned;
    } Control;
    struct {
      ULONG fulFlags;
      ULONG ulNumberOfDestinations;
      PDEVICE_OBJECT hDestinationDeviceObjects[64];
      ULONG fulSpeed;
    } GetMaxSpeedBetweenDevices;
    struct {
      ULONG fulSpeed;
      ULONG fulPriority;
    } SetDeviceXmitProperties;
    struct {
      ULONG nLevel;
      PVOID Information;
    } SetLocalHostProperties;
    struct {
      PCONFIG_ROM ConfigRom;
      ULONG UnitDirectoryBufferSize;
      PVOID UnitDirectory;
      IO_ADDRESS UnitDirectoryLocation;
      ULONG UnitDependentDirectoryBufferSize;
      PVOID UnitDependentDirectory;
      IO_ADDRESS UnitDependentDirectoryLocation;
      ULONG VendorLeafBufferSize;
      PTEXTUAL_LEAF VendorLeaf;
      ULONG ModelLeafBufferSize;
      PTEXTUAL_LEAF ModelLeaf;
    } GetConfigurationInformation;
    struct {
      ULONG fulFlags;
    } BusReset;
    struct {
      ULONG GenerationCount;
    } GetGenerationCount;
    struct {
      PHY_CONFIGURATION_PACKET PhyConfigurationPacket;
    } SendPhyConfigurationPacket;
    struct {
      PSPEED_MAP SpeedMap;
      PTOPOLOGY_MAP TopologyMap;
    } GetSpeedTopologyMaps;
    struct {
      ULONG fulFlags;
      PBUS_BUS_RESET_NOTIFICATION ResetRoutine;
      PVOID ResetContext;
    } BusResetNotification;
    struct {
      ULONG nNumberOfBytesToStream;
      ULONG fulFlags;
      PMDL Mdl;
      ULONG ulTag;
      ULONG nChannel;
      ULONG ulSynch;
      ULONG Reserved;
      UCHAR nSpeed;
    } AsyncStream;
  } u;
};
#else
struct _IRB {
  ULONG FunctionNumber;
  ULONG Flags;
  ULONG_PTR BusReserved[8];
  ULONG_PTR PortReserved[8];
  union {
    struct {
      IO_ADDRESS DestinationAddress;
      ULONG nNumberOfBytesToRead;
      ULONG nBlockSize;
      ULONG fulFlags;
      PMDL Mdl;
      ULONG ulGeneration;
      UCHAR chPriority;
      UCHAR nSpeed;
      UCHAR tCode;
      UCHAR Reserved;
      ULONG ElapsedTime;
    } AsyncRead;
    struct {
      IO_ADDRESS DestinationAddress;
      ULONG nNumberOfBytesToWrite;
      ULONG nBlockSize;
      ULONG fulFlags;
      PMDL Mdl;
      ULONG ulGeneration;
      UCHAR chPriority;
      UCHAR nSpeed;
      UCHAR tCode;
      UCHAR Reserved;
      ULONG ElapsedTime;
    } AsyncWrite;
    struct {
      IO_ADDRESS DestinationAddress;
      ULONG nNumberOfArgBytes;
      ULONG nNumberOfDataBytes;
      ULONG fulTransactionType;
      ULONG fulFlags;
      ULONG Arguments[2];
      ULONG DataValues[2];
      PVOID pBuffer;
      ULONG ulGeneration;
      UCHAR chPriority;
      UCHAR nSpeed;
      UCHAR tCode;
      UCHAR Reserved;
    } AsyncLock;
    struct {
      ULONG nMaxBytesPerFrameRequested;
      ULONG fulSpeed;
      HANDLE hBandwidth;
      ULONG BytesPerFrameAvailable;
      ULONG SpeedSelected;
      ULONG nBandwidthUnitsRequired;
    } IsochAllocateBandwidth;
    struct {
      ULONG nRequestedChannel;
      ULONG Channel;
      LARGE_INTEGER ChannelsAvailable;
    } IsochAllocateChannel;
    struct {
      ULONG fulSpeed;
      ULONG fulFlags;
      ULONG nChannel;
      ULONG nMaxBytesPerFrame;
      ULONG nNumberOfBuffers;
      ULONG nMaxBufferSize;
      ULONG nQuadletsToStrip;
      HANDLE hResource;
    } IsochAllocateResources;
    struct {
      HANDLE hResource;
      ULONG nNumberOfDescriptors;
      PISOCH_DESCRIPTOR pIsochDescriptor;
    } IsochAttachBuffers;
    struct {
      HANDLE hResource;
      ULONG nNumberOfDescriptors;
      PISOCH_DESCRIPTOR pIsochDescriptor;
    } IsochDetachBuffers;
    struct {
      HANDLE hBandwidth;
      ULONG nMaxBytesPerFrameRequested;
      ULONG fulSpeed;
      ULONG BytesPerFrameAvailable;
      ULONG SpeedSelected;
      ULONG nBandwidthUnitsRequired;
    } IsochFreeBandwidth;
    struct {
      ULONG nChannel;
    } IsochFreeChannel;
    struct {
      HANDLE hResource;
    } IsochFreeResources;
    struct {
      HANDLE hResource;
      ULONG fulFlags;
      CYCLE_TIME StartTime;
    } IsochListen;
    struct {
      CYCLE_TIME CycleTime;
    } IsochQueryCurrentCycleTime;
    struct {
      ULONG fulSpeed;
      ULONG BytesPerFrameAvailable;
      LARGE_INTEGER ChannelsAvailable;
    } IsochQueryResources;
    struct {
      HANDLE hBandwidth;
      ULONG nMaxBytesPerFrame;
      ULONG nBandwidthUnitsRequired;
    } IsochSetChannelBandwidth;
    struct {
      HANDLE hResource;
      ULONG fulFlags;
    } IsochStop;
    struct {
      HANDLE hResource;
      ULONG fulFlags;
      CYCLE_TIME StartTime;
    } IsochTalk;
    struct {
      PMDL Mdl;
      ULONG fulFlags;
      ULONG nLength;
      ULONG MaxSegmentSize;
      ULONG fulAccessType;
      ULONG fulNotificationOptions;
      PVOID Callback;
      PVOID Context;
      ADDRESS_OFFSET Required1394Offset;
      PSLIST_HEADER FifoSListHead;
      PKSPIN_LOCK FifoSpinLock;
      ULONG AddressesReturned;
      PADDRESS_RANGE p1394AddressRange;
      HANDLE hAddressRange;
      PVOID DeviceExtension;
    } AllocateAddressRange;
    struct {
      ULONG nAddressesToFree;
      PADDRESS_RANGE p1394AddressRange;
      PHANDLE pAddressRange;
      PVOID DeviceExtension;
    } FreeAddressRange;
    struct {
      ULONG nLevel;
      PVOID Information;
    } GetLocalHostInformation;
    struct {
      ULONG fulFlags;
      NODE_ADDRESS NodeAddress;
    } Get1394AddressFromDeviceObject;
    struct {
      ULONG ulIoControlCode;
      PMDL pInBuffer;
      ULONG ulInBufferLength;
      PMDL pOutBuffer;
      ULONG ulOutBufferLength;
      ULONG BytesReturned;
    } Control;
    struct {
      ULONG fulFlags;
      ULONG ulNumberOfDestinations;
      PDEVICE_OBJECT hDestinationDeviceObjects[64];
      ULONG fulSpeed;
    } GetMaxSpeedBetweenDevices;
    struct {
      ULONG fulSpeed;
      ULONG fulPriority;
    } SetDeviceXmitProperties;
    struct {
      ULONG nLevel;
      PVOID Information;
    } SetLocalHostProperties;
    struct {
      PCONFIG_ROM ConfigRom;
      ULONG UnitDirectoryBufferSize;
      PVOID UnitDirectory;
      IO_ADDRESS UnitDirectoryLocation;
      ULONG UnitDependentDirectoryBufferSize;
      PVOID UnitDependentDirectory;
      IO_ADDRESS UnitDependentDirectoryLocation;
      ULONG VendorLeafBufferSize;
      PTEXTUAL_LEAF VendorLeaf;
      ULONG ModelLeafBufferSize;
      PTEXTUAL_LEAF ModelLeaf;
    } GetConfigurationInformation;
    struct {
      ULONG fulFlags;
    } BusReset;
    struct {
      ULONG GenerationCount;
    } GetGenerationCount;
    struct {
      PHY_CONFIGURATION_PACKET PhyConfigurationPacket;
    } SendPhyConfigurationPacket;
    struct {
      PSPEED_MAP SpeedMap;
      PTOPOLOGY_MAP TopologyMap;
    } GetSpeedTopologyMaps;
    struct {
      ULONG fulFlags;
      PBUS_BUS_RESET_NOTIFICATION ResetRoutine;
      PVOID ResetContext;
    } BusResetNotification;
    struct {
      ULONG nNumberOfBytesToStream;
      ULONG fulFlags;
      PMDL Mdl;
      ULONG ulTag;
      ULONG nChannel;
      ULONG ulSynch;
      ULONG Reserved;
      UCHAR nSpeed;
    } AsyncStream;
  } u;
};
#endif
typedef struct _IRB IRB;
typedef struct _IRB *PIRB;
#if _WIN32_WINNT >= 0x0501
struct _SET_LOCAL_HOST_PROPS1 {
  ULONG fulFlags;
};
typedef struct _SET_LOCAL_HOST_PROPS1 SET_LOCAL_HOST_PROPS1;
typedef struct _SET_LOCAL_HOST_PROPS1 *PSET_LOCAL_HOST_PROPS1;
#endif
struct _SET_LOCAL_HOST_PROPS2 {
  ULONG GapCountLowerBound;
};
typedef struct _SET_LOCAL_HOST_PROPS2 SET_LOCAL_HOST_PROPS2;
typedef struct _SET_LOCAL_HOST_PROPS2 *PSET_LOCAL_HOST_PROPS2;
struct _SET_LOCAL_HOST_PROPS3 {
  ULONG fulFlags;
  HANDLE hCromData;
  ULONG nLength;
  PMDL Mdl;
};
typedef struct _SET_LOCAL_HOST_PROPS3 SET_LOCAL_HOST_PROPS3;
typedef struct _SET_LOCAL_HOST_PROPS3 *PSET_LOCAL_HOST_PROPS3;
#if _WIN32_WINNT >= 0x0501
struct _SET_LOCAL_HOST_PROPS4 {
  ULONG MaxAsyncPayloadRequested;
  ULONG MaxAsyncPayloadResult;
};
typedef struct _SET_LOCAL_HOST_PROPS4 SET_LOCAL_HOST_PROPS4;
typedef struct _SET_LOCAL_HOST_PROPS4 *PSET_LOCAL_HOST_PROPS4;
#endif
struct _GET_LOCAL_HOST_INFO1 {
  LARGE_INTEGER UniqueId;
};
typedef struct _GET_LOCAL_HOST_INFO1 GET_LOCAL_HOST_INFO1;
typedef struct _GET_LOCAL_HOST_INFO1 *PGET_LOCAL_HOST_INFO1;
struct _GET_LOCAL_HOST_INFO2 {
  ULONG HostCapabilities;
  ULONG MaxAsyncReadRequest;
  ULONG MaxAsyncWriteRequest;
};
typedef struct _GET_LOCAL_HOST_INFO2 GET_LOCAL_HOST_INFO2;
typedef struct _GET_LOCAL_HOST_INFO2 *PGET_LOCAL_HOST_INFO2;
struct _GET_LOCAL_HOST_INFO3 {
  ULONG deciWattsSupplied;
  ULONG Voltage;
};
typedef struct _GET_LOCAL_HOST_INFO3 GET_LOCAL_HOST_INFO3;
typedef struct _GET_LOCAL_HOST_INFO3 *PGET_LOCAL_HOST_INFO3;
typedef NTSTATUS (*PPORT_PHYS_ADDR_ROUTINE)(PVOID, PIRB) __attribute__((stdcall));
typedef void (*PPORT_ALLOC_COMPLETE_NOTIFICATION)(PVOID) __attribute__((stdcall));
struct _GET_LOCAL_HOST_INFO4 {
  PPORT_PHYS_ADDR_ROUTINE PhysAddrMappingRoutine;
  PVOID Context;
};
typedef struct _GET_LOCAL_HOST_INFO4 GET_LOCAL_HOST_INFO4;
typedef struct _GET_LOCAL_HOST_INFO4 *PGET_LOCAL_HOST_INFO4;
struct _GET_LOCAL_HOST_INFO5 {
  PVOID ConfigRom;
  ULONG ConfigRomLength;
};
typedef struct _GET_LOCAL_HOST_INFO5 GET_LOCAL_HOST_INFO5;
typedef struct _GET_LOCAL_HOST_INFO5 *PGET_LOCAL_HOST_INFO5;
struct _GET_LOCAL_HOST_INFO6 {
  ADDRESS_OFFSET CsrBaseAddress;
  ULONG CsrDataLength;
  PVOID CsrDataBuffer;
};
typedef struct _GET_LOCAL_HOST_INFO6 GET_LOCAL_HOST_INFO6;
typedef struct _GET_LOCAL_HOST_INFO6 *PGET_LOCAL_HOST_INFO6;
#if _WIN32_WINNT >= 0x0501
struct _GET_LOCAL_HOST_INFO7 {
  ULONG HostDmaCapabilities;
  ULARGE_INTEGER MaxDmaBufferSize;
};
#else
struct _GET_LOCAL_HOST_INFO7 {
  ULONG HostDmaCapabilities;
  ULARGE_INTEGER MaxDmaBufferSize;
  ULONG MaxOutstandingXmitRequests;
  ULONG MaxOutstandingXmitResponses;
};
#endif
typedef struct _GET_LOCAL_HOST_INFO7 GET_LOCAL_HOST_INFO7;
typedef struct _GET_LOCAL_HOST_INFO7 *PGET_LOCAL_HOST_INFO7;

#endif
