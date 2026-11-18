/* netvxd.h: Network VxDs: their initialization order and
   the structures shared by network transports and drivers.

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   VXD_WINVER whose DDK has it. */
#ifndef VXD_NETVXD_H
#define VXD_NETVXD_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
#define VXDNET_INC 1
#define VNetSup_Device_ID 0x0480
#if VXD_WINVER >= 0x040A
#define VNETSUP_Device_ID 0x0480
#endif
#define VRedir_Device_ID 0x0481
#define VBrowse_Device_ID 0x0482
#define VSHARE_Device_ID 0x0483
#define SERVER_Device_ID 0x32
#define MEMPROBE_Device_ID 0x0485
#define VFAT_Device_ID 0x0486
#define NWLINK_Device_ID 0x0487
#define VNWLINK_Device_ID 0x0487
#define NWSUP_Device_ID 0x0487
#define VTDI_Device_ID 0x0488
#define VIP_Device_ID 0x0489
#define VTCP_Device_ID 0x048A
#define VUDP_Device_ID 0x048C
#define VAsync_Device_ID 0x048D
#define NWREDIR_Device_ID 0x048E
#define FILESEC_Device_ID 0x0491
#define NWSERVER_Device_ID 0x0492
#define SECPROV_DEVICE_ID 0x0493
#if VXD_WINVER >= 0x040A
#define SECPROV_Device_ID 0x0493
#endif
#define NSCL_Device_ID 0x0494
#define WSTCP_DEVICE_ID 0x0495
#define NDIS2SUP_Device_ID 0x0496
#define MSODISUP_Device_ID 0x0497
#define Splitter_Device_ID 0x0498
#define PPP_Device_ID 0x0499
#define VDHCP_Device_ID 0x049A
#define VNBT_Device_ID 0x049B
#define LOGGER_DEVICE_ID 0x049D
#define EFILTER_Device_ID 0x049E
#define FFILTER_Device_ID 0x049F
#define TFILTER_Device_ID 0x04A0
#define AFILTER_Device_ID 0x04A1
#define IRLAMP_Device_ID 0x04A2
#if VXD_WINVER >= 0x040A
#define NDISWAN_Device_ID 0x04A4
#define NDIS40_DEVICE_ID 0x04A3
#define IRDA_TINYTP_DEVICE_ID 0x04A5
#define IRDA_IRDALAN_DEVICE_ID 0x04A6
#define DFS_Device_ID 0x04A7
#define CSC_Device_ID 0x04A8
#endif
#define MAC_Device_ID 0x00
#define NDIS_DEVICE_ID 0x28
#define VNB_Device_ID 0x31
#define WSOCK_DEVICE_ID 0x3E
#if VXD_WINVER >= 0x040A
#define WSOCK2_DEVICE_ID 0x3B0A
#endif
#define WSIPX_DEVICE_ID 0x3F

#pragma pack(pop)

#endif
