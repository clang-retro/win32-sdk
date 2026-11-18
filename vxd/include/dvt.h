/* dvt.h: The driver
   vector table (DVT) IOS keeps for each driver. (In the DDKs'
   block/inc.)

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   VXD_WINVER whose DDK has it. */
#ifndef VXD_DVT_H
#define VXD_DVT_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
#define DVT_NAME_LEN 16
#define DVT_DATE_LEN 8
#define DVT_TIME_LEN 8
#define DVT_FILE_LEN 8
#define DVT_REV_LEN 4
#define DVT_FC_LEN 4
#define DVT_FNAME_LEN 12
#define DVT_BT_ESDI 0x00
#define DVT_BT_SCSI 0x01
#define DVT_BT_FLOPPY 0x02
#define DVT_BT_SMART 0x03
#define DVT_BT_ABIOS 0x04
#define DVT_FC_SCAN_DOWN 0x04
#define DVT_FC_IO_FOR_INQ_AEP 0x40
#define DVT_FC_HALF_SEC 0x2000
#define DVT_FC_1_SEC 0x2000
#define DVT_FC_2_SECS 0x4000
#define DVT_FC_4_SECS 0x8000
#define DVT_FC_DYNALOAD 0x00010000
#define DVT_FC_NEED_PRELOAD 0x00020000
#define DVT_FC_NEED_PRE_POST_LOAD 0x00040000
struct DVT {
  USHORT DVT_reserved_1;
  ULONG DVT_next_dvt;
  USHORT DVT_device_cnt;
  PVOID DVT_aer;
  ULONG DVT_ddb;
  ULONG DVT_ddb_init;
  CHAR DVT_ascii_name[16];
  CHAR DVT_create_date[8];
  CHAR DVT_create_time[8];
  CHAR DVT_rev_level[4];
  ULONG DVT_feature_code;
  USHORT DVT_if_requirements;
  BYTE DVT_bus_type;
  ULONG DVT_reference_data;
  CHAR DVT_first_drive;
  CHAR DVT_current_lgn;
  ULONG DVT_LoadHandle;
  CHAR DVT_scsi_max_target;
  CHAR DVT_scsi_max_lun;
  PVOID DVT_entry_point;
  UCHAR DVT_init_count;
  PVOID DVT_reserved[2];
};
typedef struct DVT _DVT;

#pragma pack(pop)

#endif
