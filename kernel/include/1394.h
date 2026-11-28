/* 1394.h: the IEEE 1394 (FireWire) bus interface for client drivers, for
 * Windows 98, 98 SE, ME (1394BUS.SYS), 2000 and later.
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) clang-retro contributors.
 *
 * Written for clang-retro from Microsoft's published documentation of
 * the 1394 bus driver interface (the "1394.h" reference pages of the
 * Windows driver documentation, whose code samples are under the MIT
 * license; see NOTICE.md) and from the IEEE 1394 standard. No text of a
 * Microsoft header is used.
 *
 * A client driver puts an I/O request block (IRB) in
 * Parameters.Others.Argument1 of an IOCTL_1394_CLASS request, and sends
 * the request to the bus driver below it. IRB.FunctionNumber is one of
 * the REQUEST_* codes. The member of IRB.u that agrees with the code
 * holds the parameters. 1394BUS.SYS exports no functions for a client.
 *
 * Some structures have one layout for Windows 2000 (the _W2K names) and
 * a longer one for Windows XP and later (the _WXP names). The names
 * without a suffix select the layout with _WIN32_WINNT. */
#ifndef WDM_1394_H
#define WDM_1394_H

#include <ntddk.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Select the Windows XP layouts when the target is XP or later. */
#if defined(_WIN32_WINNT) && _WIN32_WINNT >= 0x0501
#define WDM_1394_WXP 1
#else
#define WDM_1394_WXP 0
#endif

/* The internal device control request that carries an IRB. */
#ifndef IOCTL_1394_CLASS
#define IOCTL_1394_CLASS \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x87, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#endif

/* IRB.FunctionNumber: the request types. */
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
#if WDM_1394_WXP
#define REQUEST_ISOCH_MODIFY_STREAM_PROPERTIES 32
#endif

/* IRB.Flags. */
#define IRB_FLAG_ALLOW_REMOTE_FREE 0x00000002

/* The number of reserved ULONG_PTR slots in an IRB. */
#define IRB_BUS_RESERVED_SZ 8
#define IRB_PORT_RESERVED_SZ 8

/* fulFlags of the asynchronous read, write and lock requests. */
#define ASYNC_FLAGS_NONINCREMENTING 0x00000001
#define ASYNC_FLAGS_PING 0x00000004
#define ASYNC_FLAGS_NO_STATUS 0x00000002

/* fulTransactionType of REQUEST_ASYNC_LOCK: the extended transaction
 * codes of the IEEE 1394 standard. */
#define LOCK_TRANSACTION_MASK_SWAP 1
#define LOCK_TRANSACTION_COMPARE_SWAP 2
#define LOCK_TRANSACTION_FETCH_ADD 3
#define LOCK_TRANSACTION_LITTLE_ADD 4
#define LOCK_TRANSACTION_BOUNDED_ADD 5
#define LOCK_TRANSACTION_WRAP_ADD 6

/* fulAccessType of REQUEST_ALLOCATE_ADDRESS_RANGE. */
#define ACCESS_FLAGS_TYPE_READ 0x00000001
#define ACCESS_FLAGS_TYPE_WRITE 0x00000002
#define ACCESS_FLAGS_TYPE_LOCK 0x00000004
#define ACCESS_FLAGS_TYPE_BROADCAST 0x00000008

/* fulNotificationOptions of REQUEST_ALLOCATE_ADDRESS_RANGE. */
#define NOTIFY_FLAGS_NEVER 0x00000000
#define NOTIFY_FLAGS_AFTER_READ 0x00000001
#define NOTIFY_FLAGS_AFTER_WRITE 0x00000002
#define NOTIFY_FLAGS_AFTER_LOCK 0x00000004

/* fulFlags of REQUEST_ALLOCATE_ADDRESS_RANGE. */
#define BIG_ENDIAN_ADDRESS_RANGE 0x00000001

/* The base of the initial register space (the CSR space) of a node. */
#define INITIAL_REGISTER_SPACE_HI 0xFFFF
#define INITIAL_REGISTER_SPACE_LO 0xF0000000

/* fulFlags of REQUEST_BUS_RESET. */
#define BUS_RESET_FLAGS_PERFORM_RESET 1
#define BUS_RESET_FLAGS_FORCE_ROOT 2

/* fulFlags of REQUEST_BUS_RESET_NOTIFICATION. */
#define REGISTER_NOTIFICATION_ROUTINE 1
#define DEREGISTER_NOTIFICATION_ROUTINE 2

/* fulFlags of REQUEST_GET_ADDR_FROM_DEVICE_OBJECT. */
#define USE_LOCAL_NODE 1

/* The speed flags (fulSpeed). */
#define SPEED_FLAGS_100 0x00000001
#define SPEED_FLAGS_200 0x00000002
#define SPEED_FLAGS_400 0x00000004
#define SPEED_FLAGS_800 0x00000008
#define SPEED_FLAGS_1600 0x00000010
#define SPEED_FLAGS_3200 0x00000020
#define SPEED_FLAGS_FASTEST 0x80000000

/* The speed codes of the IEEE 1394 standard (nSpeed, SPEED_MAP). */
#define SCODE_100_RATE 0
#define SCODE_200_RATE 1
#define SCODE_400_RATE 2
#define SCODE_800_RATE 3
#define SCODE_1600_RATE 4
#define SCODE_3200_RATE 5

/* nRequestedChannel of REQUEST_ISOCH_ALLOCATE_CHANNEL: any free one. */
#define ISOCH_ANY_CHANNEL 0xFFFFFFFF

/* fulFlags of REQUEST_ISOCH_ALLOCATE_RESOURCES. */
#define RESOURCE_USED_IN_LISTENING 0x00000001
#define RESOURCE_USED_IN_TALKING 0x00000002
#define RESOURCE_BUFFERS_CIRCULAR 0x00000004
#define RESOURCE_STRIP_ADDITIONAL_QUADLETS 0x00000008
#define RESOURCE_TIME_STAMP_ON_COMPLETION 0x00000010
#define RESOURCE_SYNCH_ON_TIME 0x00000020
#define RESOURCE_USE_PACKET_BASED 0x00000040
#define RESOURCE_VARIABLE_ISOCH_PAYLOAD 0x00000080
#if WDM_1394_WXP
#define RESOURCE_USE_MULTICHANNEL 0x00000100
#endif

/* ISOCH_DESCRIPTOR.fulFlags. */
#define DESCRIPTOR_SYNCH_ON_SY 0x00000001
#define DESCRIPTOR_SYNCH_ON_TAG 0x00000002
#define DESCRIPTOR_SYNCH_ON_TIME 0x00000004
#define DESCRIPTOR_USE_SY_TAG_IN_FIRST 0x00000008
#define DESCRIPTOR_TIME_STAMP_ON_COMPLETION 0x00000010
#define DESCRIPTOR_PRIORITY_TIME_DELIVERY 0x00000020
#define DESCRIPTOR_HEADER_SCATTER_GATHER 0x00000040

/* nLevel of REQUEST_GET_LOCAL_HOST_INFO: which GET_LOCAL_HOST_INFOn
 * structure Information points to. */
#define GET_HOST_UNIQUE_ID 1
#define GET_HOST_CAPABILITIES 2
#define GET_POWER_SUPPLIED 3
#define GET_PHYS_ADDR_ROUTINE 4
#define GET_HOST_CONFIG_ROM 5
#define GET_HOST_CSR_CONTENTS 6
#define GET_HOST_DMA_CAPABILITIES 7

/* GET_LOCAL_HOST_INFO2.HostCapabilities. */
#define HOST_INFO_PACKET_BASED 0x00000001
#define HOST_INFO_STREAM_BASED 0x00000002
#define HOST_INFO_SUPPORTS_ISOCH_STRIPPING 0x00000004
#define HOST_INFO_SUPPORTS_START_ON_CYCLE 0x00000008
#define HOST_INFO_SUPPORTS_RETURNING_ISO_HDR 0x00000010
#define HOST_INFO_SUPPORTS_ISO_HDR_INSERTION 0x00000020
#if WDM_1394_WXP
#define HOST_INFO_SUPPORTS_ISO_DUAL_BUFFER_RX 0x00000040
#define HOST_INFO_DMA_DOUBLE_BUFFERING_ENABLED 0x00000080
#endif

/* nLevel of REQUEST_SET_LOCAL_HOST_PROPERTIES. */
#define SET_LOCAL_HOST_PROPERTIES_NO_CYCLE_STARTS 1
#define SET_LOCAL_HOST_PROPERTIES_GAP_COUNT 2
#define SET_LOCAL_HOST_PROPERTIES_MODIFY_CROM 3

/* SET_LOCAL_HOST_PROPS3.fulFlags. */
#define SLHP_FLAG_ADD_CROM_DATA 0x01
#define SLHP_FLAG_REMOVE_CROM_DATA 0x02

/* The packet identifiers of PHY packets (IEEE 1394). */
#define PHY_PACKET_ID_CONFIGURATION 0
#define PHY_PACKET_ID_SELF_ID 2

/* SELF_ID.SID_Power_Class (IEEE 1394). */
#define POWER_CLASS_NOT_NEED_NOT_REPEAT 0
#define POWER_CLASS_SELF_POWER_PROVIDE_15W 1
#define POWER_CLASS_SELF_POWER_PROVIDE_30W 2
#define POWER_CLASS_SELF_POWER_PROVIDE_45W 3
#define POWER_CLASS_MAYBE_POWERED_UPTO_1W 4
#define POWER_CLASS_IS_POWERED_UPTO_1W_NEEDS_2W 5
#define POWER_CLASS_IS_POWERED_UPTO_1W_NEEDS_5W 6
#define POWER_CLASS_IS_POWERED_UPTO_1W_NEEDS_9W 7

/* The response codes of the IEEE 1394 standard (NOTIFICATION_INFO). */
#define RCODE_RESPONSE_COMPLETE 0
#define RCODE_RESERVED1 1
#define RCODE_RESERVED2 2
#define RCODE_RESERVED3 3
#define RCODE_CONFLICT_ERROR 4
#define RCODE_DATA_ERROR 5
#define RCODE_TYPE_ERROR 6
#define RCODE_ADDRESS_ERROR 7
#define RCODE_TIMED_OUT 15

/* A response code. */
typedef ULONG RCODE, *PRCODE;

/* The 16-bit address of a node: the node number and the bus number. */
typedef struct _NODE_ADDRESS {
  USHORT NA_Node_Number : 6;
  USHORT NA_Bus_Number : 10;
} NODE_ADDRESS, *PNODE_ADDRESS;

/* The 48-bit offset in the address space of a node. */
typedef struct _ADDRESS_OFFSET {
  USHORT Off_High;
  ULONG Off_Low;
} ADDRESS_OFFSET, *PADDRESS_OFFSET;

/* A full 1394 address: the node and the offset in it. */
typedef struct _IO_ADDRESS {
  NODE_ADDRESS IA_Destination_ID;
  ADDRESS_OFFSET IA_Destination_Offset;
} IO_ADDRESS, *PIO_ADDRESS;

/* One range of 1394 addresses that the bus driver gave the client. */
typedef struct _ADDRESS_RANGE {
  USHORT AR_Off_High;
  USHORT AR_Length;
  ULONG AR_Off_Low;
} ADDRESS_RANGE, *PADDRESS_RANGE;

/* A self-ID packet (packet 0 of a node). */
typedef struct _SELF_ID {
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
} SELF_ID, *PSELF_ID;

/* A self-ID packet that follows packet 0 (ports 4 and higher). */
typedef struct _SELF_ID_MORE {
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
} SELF_ID_MORE, *PSELF_ID_MORE;

/* A PHY configuration packet. */
typedef struct _PHY_CONFIGURATION_PACKET {
  ULONG PCP_Phys_ID : 6;
  ULONG PCP_Packet_ID : 2;
  ULONG PCP_Gap_Count : 6;
  ULONG PCP_Set_Gap_Count : 1;
  ULONG PCP_Force_Root : 1;
  ULONG PCP_Reserved1 : 8;
  ULONG PCP_Reserved2 : 8;
  ULONG PCP_Inverse;
} PHY_CONFIGURATION_PACKET, *PPHY_CONFIGURATION_PACKET;

/* The topology map of the bus: one SELF_ID for each self-ID packet. */
typedef struct _TOPOLOGY_MAP {
  USHORT TOP_Length;
  USHORT TOP_CRC;
  ULONG TOP_Generation;
  USHORT TOP_Node_Count;
  USHORT TOP_Self_ID_Count;
  SELF_ID TOP_Self_ID_Array[1];
} TOPOLOGY_MAP, *PTOPOLOGY_MAP;

/* The speed map of the bus. */
typedef struct _SPEED_MAP {
  USHORT SPD_Length;
  USHORT SPD_CRC;
  ULONG SPD_Generation;
  UCHAR SPD_Speed_Code[4032];
} SPEED_MAP, *PSPEED_MAP;

/* The bus information block at the start of the configuration ROM. */
typedef struct _CONFIG_ROM {
  ULONG CR_Info;
  ULONG CR_Signiture; /* the spelling of the documentation */
  ULONG CR_BusInfoBlockCaps;
  ULONG CR_Node_UniqueID[2];
  ULONG CR_Root_Info;
} CONFIG_ROM, *PCONFIG_ROM;

/* A textual leaf of the configuration ROM. The text starts at TL_Data. */
typedef struct _TEXTUAL_LEAF {
  USHORT TL_CRC;
  USHORT TL_Length;
  ULONG TL_Spec_Id;
  ULONG TL_Language_Id;
  UCHAR TL_Data;
} TEXTUAL_LEAF, *PTEXTUAL_LEAF;

/* An isochronous cycle time. */
typedef struct _CYCLE_TIME {
  ULONG CL_CycleOffset : 12;
  ULONG CL_CycleCount : 13;
  ULONG CL_SecondCount : 7;
} CYCLE_TIME, *PCYCLE_TIME;

/* One buffer in the FIFO of an address range that the bus driver fills
 * with the data of incoming write requests. */
typedef struct _ADDRESS_FIFO {
  SLIST_ENTRY FifoList;
  PMDL FifoMdl;
} ADDRESS_FIFO, *PADDRESS_FIFO;

/* What the bus driver gives the notification routine of an address
 * range when a request for the range arrives (Windows 2000 layout). */
typedef struct _NOTIFICATION_INFO_W2K {
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
} NOTIFICATION_INFO_W2K, *PNOTIFICATION_INFO_W2K;

/* The Windows XP layout of the notification information: it adds the
 * response code. */
typedef struct _NOTIFICATION_INFO_WXP {
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
  RCODE ResponseCode;
} NOTIFICATION_INFO_WXP, *PNOTIFICATION_INFO_WXP;

/* The notification information for the target version. */
typedef struct _NOTIFICATION_INFO {
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
#if WDM_1394_WXP
  RCODE ResponseCode;
#endif
} NOTIFICATION_INFO, *PNOTIFICATION_INFO;

/* The routine that the bus driver calls when the attached buffers of
 * an isochronous descriptor are ready to be detached. */
typedef void (*PBUS_ISOCH_DESCRIPTOR_ROUTINE)(PVOID Context1,
                                              PVOID Context2);

/* The routine that the bus driver calls after a bus reset. */
typedef void (*PBUS_BUS_RESET_NOTIFICATION)(PVOID Context);

/* The routine that the bus driver calls when a PHY packet arrives. */
typedef void (*PBUS_PHY_PACKET_NOTIFICATION)(PVOID Context,
                                             ULONG GenerationCount,
                                             ULARGE_INTEGER PhyPacket);

/* One buffer for isochronous transfers. */
typedef struct _ISOCH_DESCRIPTOR {
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
} ISOCH_DESCRIPTOR, *PISOCH_DESCRIPTOR;

/* What the extended bus reset notification gives in ResetContext. */
typedef struct _BUS_RESET_DATA {
  PVOID ResetContext;
  ULONG GenerationCount;
  NODE_ADDRESS DeviceNodeId;
  NODE_ADDRESS LocalNodeId;
  UCHAR SpeedToNode;
} BUS_RESET_DATA, *PBUS_RESET_DATA;

/* The configuration ROM object of REQUEST_GET_CONFIG_ROM. Its contents
 * are private to the bus driver. */
typedef struct _CROM_DATA *PCROM;

/* GET_HOST_UNIQUE_ID: the 64-bit unique ID of the host controller. */
typedef struct _GET_LOCAL_HOST_INFO1 {
  LARGE_INTEGER UniqueId;
} GET_LOCAL_HOST_INFO1, *PGET_LOCAL_HOST_INFO1;

/* GET_HOST_CAPABILITIES: the capabilities of the host controller. */
typedef struct _GET_LOCAL_HOST_INFO2 {
  ULONG HostCapabilities;
  ULONG MaxAsyncReadRequest;
  ULONG MaxAsyncWriteRequest;
} GET_LOCAL_HOST_INFO2, *PGET_LOCAL_HOST_INFO2;

/* GET_POWER_SUPPLIED: the power that the host gives to the bus. */
typedef struct _GET_LOCAL_HOST_INFO3 {
  ULONG deciWattsSupplied;
  ULONG Voltage;
} GET_LOCAL_HOST_INFO3, *PGET_LOCAL_HOST_INFO3;

struct _IRB;

/* The physical address mapping routine of the port driver. */
typedef NTSTATUS (*PPORT_PHYS_ADDR_ROUTINE)(PVOID Context,
                                            struct _IRB *Irb);

/* GET_PHYS_ADDR_ROUTINE: the physical address mapping routine. */
typedef struct _GET_LOCAL_HOST_INFO4 {
  PPORT_PHYS_ADDR_ROUTINE PhysAddrMappingRoutine;
  PVOID Context;
} GET_LOCAL_HOST_INFO4, *PGET_LOCAL_HOST_INFO4;

/* GET_HOST_CONFIG_ROM: a copy of the configuration ROM of the host. */
typedef struct _GET_LOCAL_HOST_INFO5 {
  PVOID ConfigRom;
  ULONG ConfigRomLength;
} GET_LOCAL_HOST_INFO5, *PGET_LOCAL_HOST_INFO5;

/* GET_HOST_CSR_CONTENTS: the contents of a CSR of the host. */
typedef struct _GET_LOCAL_HOST_INFO6 {
  ADDRESS_OFFSET CsrBaseAddress;
  ULONG CsrDataLength;
  PVOID CsrDataBuffer;
} GET_LOCAL_HOST_INFO6, *PGET_LOCAL_HOST_INFO6;

/* GET_HOST_DMA_CAPABILITIES, Windows 2000 layout. */
typedef struct _GET_LOCAL_HOST_INFO7_W2K {
  ULONG HostDmaCapabilities;
  ULARGE_INTEGER MaxDmaBufferSize;
  ULONG MaxOutstandingXmitRequests;
  ULONG MaxOutstandingXmitResponses;
} GET_LOCAL_HOST_INFO7_W2K, *PGET_LOCAL_HOST_INFO7_W2K;

/* GET_HOST_DMA_CAPABILITIES, Windows XP layout. */
typedef struct _GET_LOCAL_HOST_INFO7_WXP {
  ULONG HostDmaCapabilities;
  ULARGE_INTEGER MaxDmaBufferSize;
} GET_LOCAL_HOST_INFO7_WXP, *PGET_LOCAL_HOST_INFO7_WXP;

/* GET_HOST_DMA_CAPABILITIES for the target version. */
typedef struct _GET_LOCAL_HOST_INFO7 {
  ULONG HostDmaCapabilities;
  ULARGE_INTEGER MaxDmaBufferSize;
#if !WDM_1394_WXP
  ULONG MaxOutstandingXmitRequests;
  ULONG MaxOutstandingXmitResponses;
#endif
} GET_LOCAL_HOST_INFO7, *PGET_LOCAL_HOST_INFO7;

/* GET_HOST_DDI_VERSION: the version of the bus driver interface. */
typedef struct _GET_LOCAL_HOST_INFO8 {
  USHORT MajorVersion;
  USHORT MinorVersion;
} GET_LOCAL_HOST_INFO8, *PGET_LOCAL_HOST_INFO8;

/* SET_LOCAL_HOST_PROPERTIES_GAP_COUNT: the lowest gap count to use. */
typedef struct _SET_LOCAL_HOST_PROPS2 {
  ULONG GapCountLowerBound;
} SET_LOCAL_HOST_PROPS2, *PSET_LOCAL_HOST_PROPS2;

/* SET_LOCAL_HOST_PROPERTIES_MODIFY_CROM: add or remove a unit
 * directory in the configuration ROM of the host. */
typedef struct _SET_LOCAL_HOST_PROPS3 {
  ULONG fulFlags;
  HANDLE hCromData;
  ULONG nLength;
  PMDL Mdl;
} SET_LOCAL_HOST_PROPS3, *PSET_LOCAL_HOST_PROPS3;

/* REQUEST_ASYNC_READ. */
typedef struct _IRB_REQ_ASYNC_READ {
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
} IRB_REQ_ASYNC_READ;

/* REQUEST_ASYNC_WRITE. */
typedef struct _IRB_REQ_ASYNC_WRITE {
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
} IRB_REQ_ASYNC_WRITE;

/* REQUEST_ASYNC_LOCK. */
typedef struct _IRB_REQ_ASYNC_LOCK {
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
} IRB_REQ_ASYNC_LOCK;

/* REQUEST_ASYNC_STREAM. */
typedef struct _IRB_REQ_ASYNC_STREAM {
  ULONG nNumberOfBytesToStream;
  ULONG fulFlags;
  PMDL Mdl;
  ULONG ulTag;
  ULONG nChannel;
  ULONG ulSynch;
  ULONG Reserved;
  UCHAR nSpeed;
} IRB_REQ_ASYNC_STREAM;

/* REQUEST_ISOCH_ALLOCATE_BANDWIDTH. */
typedef struct _IRB_REQ_ISOCH_ALLOCATE_BANDWIDTH {
  ULONG nMaxBytesPerFrameRequested;
  ULONG fulSpeed;
  HANDLE hBandwidth;
  ULONG BytesPerFrameAvailable;
  ULONG SpeedSelected;
  ULONG nBandwidthUnitsRequired;
} IRB_REQ_ISOCH_ALLOCATE_BANDWIDTH;

/* REQUEST_ISOCH_ALLOCATE_CHANNEL. */
typedef struct _IRB_REQ_ISOCH_ALLOCATE_CHANNEL {
  ULONG nRequestedChannel;
  ULONG Channel;
  LARGE_INTEGER ChannelsAvailable;
} IRB_REQ_ISOCH_ALLOCATE_CHANNEL;

/* REQUEST_ISOCH_ALLOCATE_RESOURCES, Windows 2000 layout. */
typedef struct _IRB_REQ_ISOCH_ALLOCATE_RESOURCES_W2K {
  ULONG fulSpeed;
  ULONG fulFlags;
  ULONG nChannel;
  ULONG nMaxBytesPerFrame;
  ULONG nNumberOfBuffers;
  ULONG nMaxBufferSize;
  ULONG nQuadletsToStrip;
  HANDLE hResource;
} IRB_REQ_ISOCH_ALLOCATE_RESOURCES_W2K;

/* REQUEST_ISOCH_ALLOCATE_RESOURCES, Windows XP layout: it adds the
 * channel mask for multichannel listening. */
typedef struct _IRB_REQ_ISOCH_ALLOCATE_RESOURCES_WXP {
  ULONG fulSpeed;
  ULONG fulFlags;
  ULONG nChannel;
  ULONG nMaxBytesPerFrame;
  ULONG nNumberOfBuffers;
  ULONG nMaxBufferSize;
  ULONG nQuadletsToStrip;
  HANDLE hResource;
  ULARGE_INTEGER ChannelMask;
} IRB_REQ_ISOCH_ALLOCATE_RESOURCES_WXP;

/* REQUEST_ISOCH_ALLOCATE_RESOURCES for the target version. */
#if WDM_1394_WXP
typedef IRB_REQ_ISOCH_ALLOCATE_RESOURCES_WXP IRB_REQ_ISOCH_ALLOCATE_RESOURCES;
#else
typedef IRB_REQ_ISOCH_ALLOCATE_RESOURCES_W2K IRB_REQ_ISOCH_ALLOCATE_RESOURCES;
#endif

/* REQUEST_ISOCH_ATTACH_BUFFERS. */
typedef struct _IRB_REQ_ISOCH_ATTACH_BUFFERS {
  HANDLE hResource;
  ULONG nNumberOfDescriptors;
  PISOCH_DESCRIPTOR pIsochDescriptor;
} IRB_REQ_ISOCH_ATTACH_BUFFERS;

/* REQUEST_ISOCH_DETACH_BUFFERS. */
typedef struct _IRB_REQ_ISOCH_DETACH_BUFFERS {
  HANDLE hResource;
  ULONG nNumberOfDescriptors;
  PISOCH_DESCRIPTOR pIsochDescriptor;
} IRB_REQ_ISOCH_DETACH_BUFFERS;

/* REQUEST_ISOCH_FREE_BANDWIDTH. */
typedef struct _IRB_REQ_ISOCH_FREE_BANDWIDTH {
  HANDLE hBandwidth;
  ULONG nMaxBytesPerFrameRequested;
  ULONG fulSpeed;
  ULONG BytesPerFrameAvailable;
  ULONG SpeedSelected;
  ULONG nBandwidthUnitsRequired;
} IRB_REQ_ISOCH_FREE_BANDWIDTH;

/* REQUEST_ISOCH_FREE_CHANNEL. */
typedef struct _IRB_REQ_ISOCH_FREE_CHANNEL {
  ULONG nChannel;
} IRB_REQ_ISOCH_FREE_CHANNEL;

/* REQUEST_ISOCH_FREE_RESOURCES. */
typedef struct _IRB_REQ_ISOCH_FREE_RESOURCES {
  HANDLE hResource;
} IRB_REQ_ISOCH_FREE_RESOURCES;

/* REQUEST_ISOCH_LISTEN. */
typedef struct _IRB_REQ_ISOCH_LISTEN {
  HANDLE hResource;
  ULONG fulFlags;
  CYCLE_TIME StartTime;
} IRB_REQ_ISOCH_LISTEN;

/* REQUEST_ISOCH_QUERY_CYCLE_TIME. */
typedef struct _IRB_REQ_ISOCH_QUERY_CURRENT_CYCLE_TIME {
  CYCLE_TIME CycleTime;
} IRB_REQ_ISOCH_QUERY_CURRENT_CYCLE_TIME;

/* REQUEST_ISOCH_QUERY_RESOURCES. */
typedef struct _IRB_REQ_ISOCH_QUERY_RESOURCES {
  ULONG fulSpeed;
  ULONG BytesPerFrameAvailable;
  LARGE_INTEGER ChannelsAvailable;
} IRB_REQ_ISOCH_QUERY_RESOURCES;

/* REQUEST_ISOCH_SET_CHANNEL_BANDWIDTH. */
typedef struct _IRB_REQ_ISOCH_SET_CHANNEL_BANDWIDTH {
  HANDLE hBandwidth;
  ULONG nMaxBytesPerFrame;
  ULONG nBandwidthUnitsRequired;
} IRB_REQ_ISOCH_SET_CHANNEL_BANDWIDTH;

/* REQUEST_ISOCH_STOP. */
typedef struct _IRB_REQ_ISOCH_STOP {
  HANDLE hResource;
  ULONG fulFlags;
} IRB_REQ_ISOCH_STOP;

/* REQUEST_ISOCH_TALK. */
typedef struct _IRB_REQ_ISOCH_TALK {
  HANDLE hResource;
  ULONG fulFlags;
  CYCLE_TIME StartTime;
} IRB_REQ_ISOCH_TALK;

/* REQUEST_ISOCH_MODIFY_STREAM_PROPERTIES (Windows XP and later). */
typedef struct _IRB_REQ_ISOCH_MODIFY_STREAM_PROPERTIES {
  HANDLE hResource;
  ULARGE_INTEGER ChannelMask;
  ULONG fulSpeed;
} IRB_REQ_ISOCH_MODIFY_STREAM_PROPERTIES;

/* REQUEST_ALLOCATE_ADDRESS_RANGE. */
typedef struct _IRB_REQ_ALLOCATE_ADDRESS_RANGE {
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
} IRB_REQ_ALLOCATE_ADDRESS_RANGE;

/* REQUEST_FREE_ADDRESS_RANGE. */
typedef struct _IRB_REQ_FREE_ADDRESS_RANGE {
  ULONG nAddressesToFree;
  PADDRESS_RANGE p1394AddressRange;
  PHANDLE pAddressRange;
  PVOID DeviceExtension;
} IRB_REQ_FREE_ADDRESS_RANGE;

/* REQUEST_GET_LOCAL_HOST_INFO. */
typedef struct _IRB_REQ_GET_LOCAL_HOST_INFORMATION {
  ULONG nLevel;
  PVOID Information;
} IRB_REQ_GET_LOCAL_HOST_INFORMATION;

/* REQUEST_GET_ADDR_FROM_DEVICE_OBJECT. */
typedef struct _IRB_REQ_GET_1394_ADDRESS_FROM_DEVICE_OBJECT {
  ULONG fulFlags;
  NODE_ADDRESS NodeAddress;
} IRB_REQ_GET_1394_ADDRESS_FROM_DEVICE_OBJECT;

/* REQUEST_CONTROL. */
typedef struct _IRB_REQ_CONTROL {
  ULONG ulIoControlCode;
  PMDL pInBuffer;
  ULONG ulInBufferLength;
  PMDL pOutBuffer;
  ULONG ulOutBufferLength;
  ULONG BytesReturned;
} IRB_REQ_CONTROL;

/* REQUEST_GET_SPEED_BETWEEN_DEVICES. */
typedef struct _IRB_REQ_GET_MAX_SPEED_BETWEEN_DEVICES {
  ULONG fulFlags;
  ULONG ulNumberOfDestinations;
  PDEVICE_OBJECT hDestinationDeviceObjects[64];
  ULONG fulSpeed;
} IRB_REQ_GET_MAX_SPEED_BETWEEN_DEVICES;

/* REQUEST_SET_DEVICE_XMIT_PROPERTIES. */
typedef struct _IRB_REQ_SET_DEVICE_XMIT_PROPERTIES {
  ULONG fulSpeed;
  ULONG fulPriority;
} IRB_REQ_SET_DEVICE_XMIT_PROPERTIES;

/* REQUEST_SET_LOCAL_HOST_PROPERTIES. */
typedef struct _IRB_REQ_SET_LOCAL_HOST_PROPERTIES {
  ULONG nLevel;
  PVOID Information;
} IRB_REQ_SET_LOCAL_HOST_PROPERTIES;

/* REQUEST_GET_CONFIGURATION_INFO. */
typedef struct _IRB_REQ_GET_CONFIGURATION_INFORMATION {
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
} IRB_REQ_GET_CONFIGURATION_INFORMATION;

/* The request to get the configuration ROM object of a node. */
typedef struct _IRB_REQ_GET_CONFIG_ROM {
  ULONG GenerationCount;
  PCROM ConfigRom;
  ULONG UnitDirectoryIndex;
  ULONG UnitDependentDirectoryIndex;
  ULONG VendorLeafIndex;
  ULONG ModelLeafIndex;
} IRB_REQ_GET_CONFIG_ROM;

/* REQUEST_BUS_RESET. */
typedef struct _IRB_REQ_BUS_RESET {
  ULONG fulFlags;
} IRB_REQ_BUS_RESET;

/* REQUEST_GET_GENERATION_COUNT. */
typedef struct _IRB_REQ_GET_GENERATION_COUNT {
  ULONG GenerationCount;
} IRB_REQ_GET_GENERATION_COUNT;

/* REQUEST_SEND_PHY_CONFIG_PACKET. */
typedef struct _IRB_REQ_SEND_PHY_CONFIGURATION_PACKET {
  PHY_CONFIGURATION_PACKET PhyConfigurationPacket;
} IRB_REQ_SEND_PHY_CONFIGURATION_PACKET;

/* The request to send a PHY packet. */
typedef struct _IRB_REQ_SEND_PHY_PACKET {
  ULONG Flags;
  ULONG GenerationCount;
  ULARGE_INTEGER PhyPacket;
} IRB_REQ_SEND_PHY_PACKET;

/* The request to register a routine for incoming PHY packets. */
typedef struct _IRB_RECEIVE_PHY_PACKETS {
  ULONG Flags;
  PBUS_PHY_PACKET_NOTIFICATION PhyPacketRoutine;
  PVOID PhyPacketContext;
} IRB_REQ_RECEIVE_PHY_PACKETS;

/* REQUEST_GET_SPEED_TOPOLOGY_MAPS. */
typedef struct _IRB_REQ_GET_SPEED_TOPOLOGY_MAPS {
  PSPEED_MAP SpeedMap;
  PTOPOLOGY_MAP TopologyMap;
} IRB_REQ_GET_SPEED_TOPOLOGY_MAPS;

/* REQUEST_BUS_RESET_NOTIFICATION. */
typedef struct _IRB_REQ_BUS_RESET_NOTIFICATION {
  ULONG fulFlags;
  PBUS_BUS_RESET_NOTIFICATION ResetRoutine;
  PVOID ResetContext;
} IRB_REQ_BUS_RESET_NOTIFICATION;

/* The I/O request block of IOCTL_1394_CLASS. */
typedef struct _IRB {
  ULONG FunctionNumber;
  ULONG Flags;
  ULONG_PTR BusReserved[IRB_BUS_RESERVED_SZ];
  ULONG_PTR PortReserved[IRB_PORT_RESERVED_SZ];
  union {
    IRB_REQ_ASYNC_READ AsyncRead;
    IRB_REQ_ASYNC_WRITE AsyncWrite;
    IRB_REQ_ASYNC_LOCK AsyncLock;
    IRB_REQ_ISOCH_ALLOCATE_BANDWIDTH IsochAllocateBandwidth;
    IRB_REQ_ISOCH_ALLOCATE_CHANNEL IsochAllocateChannel;
    IRB_REQ_ISOCH_ALLOCATE_RESOURCES IsochAllocateResources;
    IRB_REQ_ISOCH_ATTACH_BUFFERS IsochAttachBuffers;
    IRB_REQ_ISOCH_DETACH_BUFFERS IsochDetachBuffers;
    IRB_REQ_ISOCH_FREE_BANDWIDTH IsochFreeBandwidth;
    IRB_REQ_ISOCH_FREE_CHANNEL IsochFreeChannel;
    IRB_REQ_ISOCH_FREE_RESOURCES IsochFreeResources;
    IRB_REQ_ISOCH_LISTEN IsochListen;
    IRB_REQ_ISOCH_QUERY_CURRENT_CYCLE_TIME IsochQueryCurrentCycleTime;
    IRB_REQ_ISOCH_QUERY_RESOURCES IsochQueryResources;
    IRB_REQ_ISOCH_SET_CHANNEL_BANDWIDTH IsochSetChannelBandwidth;
    IRB_REQ_ISOCH_STOP IsochStop;
    IRB_REQ_ISOCH_TALK IsochTalk;
#if WDM_1394_WXP
    IRB_REQ_ISOCH_MODIFY_STREAM_PROPERTIES IsochModifyStreamProperties;
#endif
    IRB_REQ_ALLOCATE_ADDRESS_RANGE AllocateAddressRange;
    IRB_REQ_FREE_ADDRESS_RANGE FreeAddressRange;
    IRB_REQ_GET_LOCAL_HOST_INFORMATION GetLocalHostInformation;
    IRB_REQ_GET_1394_ADDRESS_FROM_DEVICE_OBJECT Get1394AddressFromDeviceObject;
    IRB_REQ_CONTROL Control;
    IRB_REQ_GET_MAX_SPEED_BETWEEN_DEVICES GetMaxSpeedBetweenDevices;
    IRB_REQ_SET_DEVICE_XMIT_PROPERTIES SetDeviceXmitProperties;
    IRB_REQ_SET_LOCAL_HOST_PROPERTIES SetLocalHostProperties;
    IRB_REQ_GET_CONFIGURATION_INFORMATION GetConfigurationInformation;
    IRB_REQ_BUS_RESET BusReset;
    IRB_REQ_GET_GENERATION_COUNT GetGenerationCount;
    IRB_REQ_SEND_PHY_CONFIGURATION_PACKET SendPhyConfigurationPacket;
    IRB_REQ_GET_SPEED_TOPOLOGY_MAPS GetSpeedTopologyMaps;
    IRB_REQ_BUS_RESET_NOTIFICATION BusResetNotification;
    IRB_REQ_ASYNC_STREAM AsyncStream;
  } u;
} IRB, *PIRB;

#ifdef __cplusplus
}
#endif

#endif
