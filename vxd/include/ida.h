/* ida.h: The IOS data
   area (IDA) of IOS's global data. (In the DDKs' block/inc.)

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   VXD_WINVER whose DDK has it. */
#ifndef VXD_IDA_H
#define VXD_IDA_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
#define IDA_250_M_SECOND 1
#define IDA_HALF_SECOND 2
#define IDA_ONE_SECOND 0x04
#define IDA_TWO_SECONDS 0x08
#define IDA_TEN_SECONDS 0x28
#define IDA_40_SECONDS 0xA0
#define IDA_INIT_COUNTER 2
#define IDAF_BOOT_COMPLETE 0x01
#define IDAF_REAL_MODE_ONLY 0x02
#define IDAF_FORCE_RM_IO 0x04
#define IDAF_REAL_MODE_LOGICAL 0x08
#define IDAF_FORCE_PM_IO 0x10
#define IDAF_V86_AREA_TAKEN 0x20
#define IDAF_BIG_MEMORY 0x40
#define IDAF_MAPPER_BUSY 0x80
#define IDAF_NEED_4BHOOK 0x0100
#define IDAF_INT13_DUE_TO_RMM 0x0200
#define IDAF_MAPPER_INIT 0x0400
#define IDAF_PUNT_CDS 0x0800
#define IDAF_MSCDEX_PRESENT 0x1000
#define IDAF_DOS_PAGER 0x2000
#define IDAF_SHUTDOWN 0x4000
#define IDAF_INT13_FCN_8_BIT 15
#define IDAF_INT13_FCN_8 0x8000
#define IDAF_STAC_PRESENT_BIT 16
#define IDAF_STAC_PRESENT 0x00010000
#define IDAF_START_SUCCESS_BIT 17
#define IDAF_START_SUCCESS 0x00020000
#define IDAF_CANT_SHARE_IRQ_BIT 18
#define IDAF_CANT_SHARE_IRQ 0x00040000
#define IDAF_NO_BOOT_CONFIG_BIT 19
#define IDAF_NO_BOOT_CONFIG 0x00080000
#define IDAF_512_BYTE_EMUL_POSSIBLE_BIT 20
#define IDAF_512_BYTE_EMUL_POSSIBLE 0x00100000
#define IDAF_INT13_IGNORE_COPY_PROT_BIT 21
#define IDAF_INT13_IGNORE_COPY_PROT 0x00200000
#define IDAF_EZ_DRIVE_BIT 22
#define IDAF_EZ_DRIVE 0x00400000
#define IDAF_NO_SPARROW_BIT 23
#define IDAF_NO_SPARROW 0x00800000
#if VXD_WINVER >= 0x040A
#define IDAF_I13_FLOPPY_BIOS_BIT 24
#define IDAF_I13_FLOPPY_BIOS 0x01000000
#endif
#define IS_ISA 0x01
#define IS_EISA 0x02
#define IS_MCA 0x04
#define IS_FAMILY3 0x08
struct IDA {
  ULONG IDA_first_ice;
  ULONG IDA_current_ice;
  ULONG IDA_ios_mem_phys;
  ULONG IDA_current_ice_len;
  ULONG IDA_ios_timer;
  ULONG IDA_ios_counter;
  ULONG IDA_start_eci;
  ULONG IDA_start_cct;
  CHAR IDA_esdi_p_in_use;
  CHAR IDA_esdi_s_in_use;
  CHAR IDA_Num_Floppies;
  CHAR PAD0;
  USHORT IDA_platform;
  USHORT PAD1;
  ULONG IDA_flags;
  PVOID IDA_platform_config_data;
  PVOID IDA_first_dcb;
  PVOID IDA_logical_dev_map;
  PVOID IDA_physical_dev_map;
  PVOID IDA_rmm_phys_dcb;
  PVOID IDA_first_vrp;
  ULONG IDA_drive_letter_map;
  ULONG IDA_cdrom_letter_map;
  ULONG IDA_rmd_ptr;
  ULONG IDA_aspi_cam_rmds;
  ULONG IDA_big_mem_buf_virt;
  ULONG IDA_big_mem_buf_phys;
  ULONG IDA_big_mem_buf_size;
  ULONG IDA_ios_mem_pool_size;
  ULONG IDA_dbl_vol_map;
  ULONG IDA_ontrack_data;
  USHORT IDA_rm_irq;
};
typedef struct IDA IDA;
typedef struct IDA *pIDA;

#pragma pack(pop)

#endif
