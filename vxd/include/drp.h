/* drp.h: The driver registration packet (DRP)
   a layered block device driver gives IOS.

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   VXD_WINVER whose DDK has it. */
#ifndef VXD_DRP_H
#define VXD_DRP_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
#define EyeCatcher 'XXXXXXXX'
#define DRP_FC_SCAN_DOWN 0x04
#define DRP_FC_IO_FOR_INQ_AEP 0x40
#define DRP_FC_HALF_SEC 0x2000
#define DRP_FC_1_SEC 0x2000
#define DRP_FC_2_SECS 0x4000
#define DRP_FC_4_SECS 0x8000
#define DRP_FC_DYNALOAD 0x00010000
#define DRP_FC_NEED_PRELOAD 0x00020000
#define DRP_FC_NEED_PRE_POST_LOAD 0x00040000
#if VXD_WINVER >= 0x045A
#define DRP_FC_SUPPORT_ESDI_REENUMERATION 0x00080000
#define DRP_FC_NTMAP 0x00100000
#endif
#define DRP_IF_ISA 0x01
#define DRP_IF_EISA 0x02
#define DRP_IF_MCA 0x04
#define DRP_IF_STD 0xFF
#define DRP_BT_ESDI 0x00
#define DRP_BT_SCSI 0x01
#define DRP_BT_FLOPPY 0x02
#define DRP_BT_SMART 0x03
#define DRP_BT_ABIOS 0x04
#define DRP_IFS_BIT 0x00
#define DRP_RESRVD1_BIT 0x01
#define DRP_FSD_BIT 0x02
#define DRP_FSD_EXT_1_BIT 0x03
#define DRP_FSD_EXT_2_BIT 0x04
#define DRP_VOLTRK_BIT 0x05
#define DRP_CLASS_DRV_BIT 0x06
#define DRP_TSD_BIT 0x07
#define DRP_VSD_1_BIT 0x08
#define DRP_VSD_2_BIT 0x09
#define DRP_VSD_3_BIT 0x0A
#define DRP_SCSI_LAYER_BIT 0x0B
#define DRP_VSD_4_BIT 0x0C
#define DRP_VSD_5_BIT 0x0D
#define DRP_VSD_6_BIT 0x0E
#define DRP_VSD_7_BIT 0x0F
#define DRP_VSD_8_BIT 0x10
#define DRP_VSD_9_BIT 0x11
#define DRP_RESRVD18_BIT 0x12
#define DRP_MISC_PD_BIT 0x13
#define DRP_NT_MPD_BIT 0x14
#define DRP_NT_PD_BIT 0x15
#define DRP_ESDI_PD_BIT 0x16
#define DRP_ESDIEMUL_PD_BIT 0x17
#define DRP_RESRVD23_BIT 0x18
#define DRP_ABIOS_PD_BIT 0x19
#define DRP_ABIOS_PREMPT_PD_BIT 0x1A
#define DRP_NEC_FLOPPY_BIT 0x1B
#define DRP_RESRVD28_BIT 0x1C
#define DRP_SOC_SER_DRV_BIT 0x1D
#define DRP_SOC_DRV_BIT 0x1E
#define DRP_IOS_REG_BIT 0x1F
#define DRP_LGN_MAX 0x1F
#define DRP_IFS 0x01
#define DRP_RESRVD1 0x02
#define DRP_FSD 0x04
#define DRP_FSD_EXT_1 0x08
#define DRP_FSD_EXT_2 0x10
#define DRP_VOLTRK 0x20
#define DRP_CLASS_DRV 0x40
#define DRP_TSD 0x80
#define DRP_VSD_1 0x0100
#define DRP_VSD_2 0x0200
#define DRP_VSD_3 0x0400
#define DRP_SCSI_LAYER 0x0800
#define DRP_VSD_4 0x1000
#define DRP_VSD_5 0x2000
#define DRP_VSD_6 0x4000
#define DRP_VSD_7 0x8000
#define DRP_VSD_8 0x00010000
#define DRP_VSD_9 0x00020000
#define DRP_RESRVD18 0x00040000
#define DRP_MISC_PD 0x00080000
#define DRP_NT_MPD 0x00100000
#define DRP_NT_PD 0x00200000
#define DRP_ESDI_PD 0x00400000
#define DRP_ESDIEMUL_PD 0x00800000
#define DRP_RESRVD23 0x01000000
#define DRP_ABIOS_PD 0x02000000
#define DRP_ABIOS_PREMPT_PD 0x04000000
#define DRP_NEC_FLOPPY 0x08000000
#define DRP_RESRVD28 0x10000000
#define DRP_SOC_SER_DRV 0x20000000
#define DRP_SOC_DRV 0x40000000
#define DRP_IOS_REG (-2147483648)
#define DRP_REMAIN_RESIDENT 1
#define DRP_MINIMIZE 2
#define DRP_ABORT 3
#define DRP_INVALID_LAYER 4
struct DRP {
  CHAR DRP_eyecatch_str[8];
  ULONG DRP_LGN;
  PVOID DRP_aer;
  PVOID DRP_ilb;
  CHAR DRP_ascii_name[16];
  BYTE DRP_revision;
  ULONG DRP_feature_code;
  USHORT DRP_if_requirements;
  UCHAR DRP_bus_type;
  USHORT DRP_reg_result;
  ULONG DRP_reference_data;
  UCHAR DRP_reserved1[2];
  ULONG DRP_reserved2[1];
};
typedef struct DRP DRP;
typedef struct DRP *PDRP;

#pragma pack(pop)

#endif
