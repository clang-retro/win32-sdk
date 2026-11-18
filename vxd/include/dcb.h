/* dcb.h: The device control
   block (DCB) a port driver makes for each device it finds, and the
   DCB_* types and flags. (In the DDKs' block/inc.)

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   VXD_WINVER whose DDK has it. */
#ifndef VXD_DCB_H
#define VXD_DCB_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
#define DCB_cd_table_depth 10
#define DCB_IS_3MODE_DRIVE 0x01
#define DCB_SET_RPM_ON_ACCESS 0x02
#define DCBF_DISK_BPB_USEFAKE 0x01
#define CDROM_MAX_TOC 804
#define CDROM_MAX_MODE_BUF 20
#define DCB_cdaudio_playing 0x01
#define DCB_cdaudio_paused 0x02
#define DCB_cdaudio_vol_set 0x04
#define DCB_cd_supports_audio_play 0x01
#define DCB_cd_supports_data_only 0x00
#define DCB_cd_multi_session_valid 0x02
#define DCB_CD_CHANGEABLE_BLOCK 0x04
#define DCB_CD_FAKE_SCSI2 0x08
#define DCB_CD_CDPLUS_DISC 0x10
#define DCB_CD_FAKE_SCSI1 0x20
#define DCB_CD_USE_ATAPI_READCD 0x40
#if VXD_WINVER >= 0x040A
#define DCB_CD_CHANGER_DEVICE 0x80
#define DCB_CD_ISDVD 0x0100
#define DCB_CD_ISMMC 0x0200
#define DCB_CD_ISAUDIO 0x0400
#define DCB_CD_ISDVD_MEDIA 0x0800
#define DCB_CD_ISDA 0x1000
#define DCB_CD_ISMESN 0x2000
#define DCB_CD_ISMESN_BIT 13
#endif
#define DCB_q_fi_fo 0x00
#define DCB_q_sort 0x01
#define DCB_type_disk 0x00
#define DCB_type_tape 0x01
#define DCB_type_printer 0x02
#define DCB_type_processor 0x03
#define DCB_type_worm 0x04
#define DCB_type_cdrom 0x05
#define DCB_type_scanner 0x06
#define DCB_type_optical_memory 0x07
#define DCB_type_changer 0x08
#define DCB_type_comm 0x09
#define DCB_type_floppy 0x0A
#define DCB_BUS_ESDI 0x00
#define DCB_BUS_SCSI 0x01
#define DCB_BUS_NEC 0x02
#define DCB_BUS_SMART 0x03
#define DCB_BUS_ABIOS 0x04
#define DCB_VF_INHIBIT_LOCKING 0x01
#define DCB_VF_INHIBIT_IO 0x02
#define DCB_VF_INHIBIT_GEOM_RECOMPUTE 0x04
#define DCB_VF_INHIBIT_GEOM_RECOMPUTE_BIT 2
#define DCB_VF_UNLOCK_SCHED_BIT 4
#define DCB_VF_UNLOCK_SCHED 0x10
#define DCB_VF_NEED_PHYS_RECOMP_BIT 5
#define DCB_VF_NEED_PHYS_RECOMP 0x20
#define DCB_VF_PROT_NEC_DRIVE_BIT 6
#define DCB_VF_PROT_NEC_DRIVE 0x40
#define DCB_TSD_INVALID_PARTITION 1
#define DCB_TSD_BAD_MBR 1
#define DCB_TSD_USER_LETTERS_CHECKED_BIT 1
#define DCB_TSD_USER_LETTERS_CHECKED 0x02
#define DCB_TSD_FIRST_USER_CHECK_BIT 2
#define DCB_TSD_FIRST_USER_CHECK 0x04
#define DCB_TSD_BID_SET_GEOM 0x08
#define DCB_TSD_USER_SET_BIT 4
#define DCB_TSD_USER_SET 0x10
#define DCB_TSD_NO_USER_INT13 0x20
#define DCB_TSD_PROTMODE_INT_13_BIT 6
#define DCB_TSD_PROTMODE_INT_13 0x40
#define DCB_TSD_NONEXIST_PARTITION_BIT 7
#define DCB_TSD_NONEXIST_PARTITION 0x80
#define DCB_TSD_MBPB_PBR 0x0100
#define DCB_TSD_RBPB_INIT_BIT 0x09
#define DCB_TSD_RBPB_INIT 0x0200
#define DCB_TSD_MBPB_INIT 0x0400
#define DCB_TSD_APPARENT_PBR 0x0800
#define DCB_TSD_APPARENT_ROM 0x1000
#define DCB_TSD_ACTUAL_SET 0x2000
#define DCB_TSD_ACTUAL_PRE_SET 0x4000
#define DCB_TSD_APPARENT_PRE_SET 0x8000
#define DCB_DEV_SPINDOWN_SUPPORTED_BIT 31
#define DCB_DEV_SPINDOWN_SUPPORTED (-2147483648)
#define DCB_DEV_SPUN_DOWN_BIT 30
#define DCB_DEV_SPUN_DOWN 0x40000000
#define DCB_DEV_IO_ACTIVE_BIT 29
#define DCB_DEV_IO_ACTIVE 0x20000000
#define DCB_DEV_INT13_CONFIGURED_BIT 28
#define DCB_DEV_INT13_CONFIGURED 0x10000000
#define DCB_DEV_NO_ASPICAM_BIT 27
#define DCB_DEV_NO_ASPICAM 0x08000000
#define DCB_DEV_REMOVAL_PENDING_BIT 26
#define DCB_DEV_REMOVAL_PENDING 0x04000000
#define DCB_DEV_RMM_INT13_BIT 25
#define DCB_DEV_RMM_INT13 0x02000000
#define DCB_DEV_TSD_PROCESSED_BIT 24
#define DCB_DEV_TSD_PROCESSED 0x01000000
#define DCB_DEV_SINGLE_FLOPPY 0x00800000
#define DCB_DEV_REAL_MODE 0x00400000
#define DCB_DEV_QUE_DESCEND 0x00200000
#define DCB_DEV_MEDIA_CHANGED 0x00100000
#define DCB_DEV_INVALID_GEOM 0x00080000
#define DCB_DEV_INHIBIT_SW_TIMED_SUPPORT 0x00040000
#define DCB_DEV_ASYNC_MED_CHG_SUPPORT 0x00020000
#define DCB_DEV_SYNC_MED_CHG_SUPPORT 0x00010000
#define DCB_DEV_PHYSICAL 0x8000
#define DCB_DEV_LOGICAL 0x4000
#define DCB_DEV_LOGICAL_BIT 14
#define DCB_DEV_RMM 0x2000
#define DCB_DEV_RMM_BIT 13
#define DCB_DEV_UNCERTAIN_MEDIA 0x1000
#define DCB_DEV_ROM_CFG_REQ 0x0800
#define DCB_DEV_MUST_CONFIGURE 0x0400
#define DCB_DEV_SSTOR_VOL 0x0200
#define DCB_DEV_PAGING_DEVICE 0x0100
#define DCB_CHAR_DEVICE 0x80
#define DCB_DEV_I13_IN_PROGRESS 0x40
#define DCB_DEV_SERIAL_CMD 0x10
#define DCB_DEV_REMOTE 0x08
#define DCB_DEV_REMOVABLE 0x04
#define DCB_DEV_REMOVABLE_BIT 2
#define DCB_DEV_WRITEABLE 0x02
#define DCB_DEV_INT13_DRIVE 0x01
#define DCB_DEV2_THREAD_LOCK 0x01
#define DCB_DEV2_THREAD_LOCK_BIT 0
#define DCB_DEV2_ATAPI_DEVICE 0x02
#define DCB_DEV2_ATAPI_DEVICE_BIT 1
#define DCB_DEV2_DMF_DISK 0x04
#define DCB_DEV2_DMF_DISK_BIT 2
#define DCB_DEV2_I13_COPY_PROT 0x08
#define DCB_DEV2_I13_COPY_PROT_BIT 3
#define DCB_DEV2_FAIL_FORMAT 0x10
#define DCB_DEV2_FAIL_FORMAT_BIT 4
#define DCB_DEV2_FLOPTICAL 0x20
#define DCB_DEV2_FLOPTICAL_BIT 5
#if VXD_WINVER >= 0x040A
#define DCB_DEV2_IDE_FLOPTICAL 0x40
#define DCB_DEV2_IDE_FLOPTICAL_BIT 6
#endif
#define DCB_dmd_srb_cdb 0x01
#define DCB_dmd_rsrv_1 0x02
#define DCB_dmd_logical 0x04
#define DCB_dmd_physical 0x08
#define DCB_dmd_small_memory 0x10
#define DCB_dmd_rsrv_2 0x20
#define DCB_dmd_rsrv_3 0x40
#define DCB_dmd_rsrv_4 0x80
#define DCB_dmd_not_512 0x0100
#define DCB_dmd_word_align 0x0200
#define DCB_dmd_dword_align 0x0400
#define DCB_dmd_phys_sgd 0x0800
#define DCB_dmd_phys_sgd_ptr 0x1000
#define DCB_dmd_do_a_b_toggling 0x2000
#define DCB_dmd_query_remov 0x4000
#define DCB_dmd_request_sns 0x8000
#define DCB_dmd_lock_unlock_media 0x00010000
#define DCB_dmd_load_eject_media 0x00020000
#define DCB_dmd_clear_media_chg 0x00040000
#define DCB_dmd_serialize_bit 19
#define DCB_dmd_serialize 0x00080000
#define DCB_dmd_prot_mode 0x00100000
#define DCB_dmd_no_xclusive 0x00200000
#define DCB_dmd_pageability 0x00400000
#if VXD_WINVER >= 0x040A
#define DCB_dmd_preload 0x00800000
#endif
#define DCB_dmd_rsrv 0xE2
struct _DCB_cd_entry {
  PVOID DCB_cd_io_address;
  ULONG DCB_cd_flags;
  ULONG DCB_cd_ddb;
  ULONG DCB_cd_next;
  USHORT DCB_cd_expan_off;
  UCHAR DCB_cd_layer_flags;
  UCHAR DCB_cd_lgn;
};
typedef struct _DCB_cd_entry DCB_cd_entry;
typedef struct _DCB_cd_entry *pDCB_cd_entry;
#if VXD_WINVER >= 0x040A
struct _DCB_COMMON {
  ULONG DCB_physical_dcb;
  ULONG DCB_expansion_length;
  PVOID DCB_ptr_cd;
  ULONG DCB_next_dcb;
  ULONG DCB_next_logical_dcb;
  BYTE DCB_drive_lttr_equiv;
  BYTE DCB_unit_number;
  USHORT DCB_TSD_Flags;
  ULONG DCB_vrp_ptr;
  ULONG DCB_dmd_flags;
  ULONG DCB_device_flags;
  ULONG DCB_device_flags2;
  ULONG DCB_Partition_Start;
  ULONG DCB_track_table_ptr;
  ULONG DCB_bds_ptr;
  ULONG DCB_Reserved1;
  ULONG DCB_pEid;
  BYTE DCB_apparent_blk_shift;
  BYTE DCB_partition_type;
  USHORT DCB_sig;
  BYTE DCB_device_type;
  ULONG DCB_Exclusive_VM;
  UCHAR DCB_disk_bpb_flags;
  UCHAR DCB_cAssoc;
  UCHAR DCB_Sstor_Host;
  USHORT DCB_user_drvlet;
  USHORT DCB_Reserved3;
  BYTE DCB_fACPI;
  BYTE DCB_fSpinDownIssued;
  BYTE DCB_bPowerState;
  BYTE DCB_bEidLength;
};
#else
struct _DCB_COMMON {
  ULONG DCB_physical_dcb;
  ULONG DCB_expansion_length;
  PVOID DCB_ptr_cd;
  ULONG DCB_next_dcb;
  ULONG DCB_next_logical_dcb;
  BYTE DCB_drive_lttr_equiv;
  BYTE DCB_unit_number;
  USHORT DCB_TSD_Flags;
  ULONG DCB_vrp_ptr;
  ULONG DCB_dmd_flags;
  ULONG DCB_device_flags;
  ULONG DCB_device_flags2;
  ULONG DCB_Partition_Start;
  ULONG DCB_track_table_ptr;
  ULONG DCB_bds_ptr;
  ULONG DCB_Reserved1;
  ULONG DCB_Reserved2;
  BYTE DCB_apparent_blk_shift;
  BYTE DCB_partition_type;
  USHORT DCB_sig;
  BYTE DCB_device_type;
  ULONG DCB_Exclusive_VM;
  UCHAR DCB_disk_bpb_flags;
  UCHAR DCB_cAssoc;
  UCHAR DCB_Sstor_Host;
  USHORT DCB_user_drvlet;
  USHORT DCB_Reserved3;
  ULONG DCB_Reserved4;
};
#endif
typedef struct _DCB_COMMON DCB_COMMON;
typedef struct _DCB_COMMON *PDCB_COMMON;
struct _DCB_BLOCKDEV {
  ULONG DCB_BDD_Next;
  BYTE DCB_BDD_BD_Major_Version;
  BYTE DCB_BDD_BD_Minor_Version;
  BYTE DCB_BDD_Device_SubType;
  BYTE DCB_BDD_Int_13h_Number;
  ULONG DCB_BDD_flags;
  ULONG DCB_BDD_Name_Ptr;
  ULONG DCB_apparent_sector_cnt[2];
  ULONG DCB_apparent_blk_size;
  ULONG DCB_apparent_head_cnt;
  ULONG DCB_apparent_cyl_cnt;
  ULONG DCB_apparent_spt;
  ULONG DCB_BDD_Sync_Cmd_Proc;
  ULONG DCB_BDD_Command_Proc;
  ULONG DCB_BDD_Hw_Int_Proc;
  ULONG DCB_BDP_Cmd_Queue_Ascending;
  ULONG DCB_BDP_Cmd_Queue_Descending;
  ULONG DCB_BDP_Current_Flags;
  ULONG DCB_BDP_Int13_Param_Ptr;
  ULONG DCB_BDP_Current_Command;
  ULONG DCB_BDP_Current_Position[2];
  ULONG DCB_BDP_Reserved[5];
  ULONG DCB_fastdisk_bdd;
};
typedef struct _DCB_BLOCKDEV DCB_BLOCKDEV;
typedef struct _DCB_BLOCKDEV *PDCB_BLOCKDEV;
struct _DCB {
  DCB_COMMON DCB_cmn;
  ULONG DCB_max_xfer_len;
  ULONG DCB_actual_sector_cnt[2];
  ULONG DCB_actual_blk_size;
  ULONG DCB_actual_head_cnt;
  ULONG DCB_actual_cyl_cnt;
  ULONG DCB_actual_spt;
  PVOID DCB_next_ddb_dcb;
  PVOID DCB_dev_node;
  BYTE DCB_bus_type;
  BYTE DCB_bus_number;
  UCHAR DCB_queue_freeze;
  UCHAR DCB_max_sg_elements;
  UCHAR DCB_io_pend_count;
  UCHAR DCB_lock_count;
  USHORT DCB_SCSI_VSD_FLAGS;
  BYTE DCB_scsi_target_id;
  BYTE DCB_scsi_lun;
  BYTE DCB_scsi_hba;
  BYTE DCB_max_sense_data_len;
  USHORT DCB_srb_ext_size;
  BYTE DCB_inquiry_flags[8];
  BYTE DCB_vendor_id[8];
  BYTE DCB_product_id[16];
  BYTE DCB_rev_level[4];
  BYTE DCB_port_name[8];
  UCHAR DCB_current_unit;
  ULONG DCB_blocked_iop;
  ULONG DCB_vol_unlock_timer;
  UCHAR DCB_access_timer;
  UCHAR DCB_Vol_Flags;
  BYTE DCB_q_algo;
  BYTE DCB_unit_on_ctl;
  ULONG DCB_Port_Specific;
  ULONG DCB_spindown_timer;
  DCB_BLOCKDEV DCB_bdd;
};
typedef struct _DCB DCB;
typedef struct _DCB *PDCB;
struct _LOG_DCB {
  DCB_COMMON DCB_cmn;
};
typedef struct _LOG_DCB LOG_DCB;
typedef struct _LOG_DCB *PLOG_DCB;
struct _DCB_disk {
  DCB DCB_filler_1;
  USHORT DCB_write_precomp;
  ULONG DCB_disk_tsd_private;
};
typedef struct _DCB_disk DCB_disk;
typedef struct _DCB_disk *pDCB_disk;
struct _DCB_floppy {
  DCB_disk DCB_filler_3;
  USHORT DCB_3mode_Flags;
};
typedef struct _DCB_floppy DCB_floppy;
typedef struct _DCB_floppy *pDCB_floppy;
struct _DCB_disk_geometry {
  ULONG sector_cnt[2];
  ULONG blk_size;
  ULONG head_cnt;
  ULONG cyl_cnt;
  ULONG spt;
};
typedef struct _DCB_disk_geometry DCB_disk_geometry;
typedef struct _DCB_disk_geometry *pDCB_disk_geometry;
#if VXD_WINVER >= 0x040A
struct _DCB_cdrom {
  DCB DCB_filler_2;
  ULONG DCB_cdrom_Partition_Start;
  ULONG DCB_cdrom_Partition_End;
  UCHAR DCB_cd_ls_ft;
  ULONG DCB_TOC[202];
  ULONG DCB_cd_mode_sense_buf[6];
  UCHAR DCB_cd_first_session;
  UCHAR DCB_cd_last_session;
  ULONG DCB_play_resume_start;
  ULONG DCB_play_resume_stop;
  ULONG DCB_play_status;
  ULONG DCB_cd_device_flags;
  UCHAR DCB_cd_fs_lt;
  UCHAR DCB_cd_bobbit_pt;
  ULONG DCB_cd_fs_lo;
  ULONG DCB_cd_last_session_start;
  ULONG DCB_cd_current_block_size;
  UCHAR DCB_cd_vol_map[8];
  ULONG DCB_cd_current_command;
  ULONG DCB_cd_queue_head;
  UCHAR DCB_cd_grp1_time[2];
  UCHAR DCB_cd_grp2_time[2];
  ULONG DCB_cd_dvd_copy_flags;
};
#else
struct _DCB_cdrom {
  DCB DCB_filler_2;
  ULONG DCB_cdrom_Partition_Start;
  ULONG DCB_cdrom_Partition_End;
  UCHAR DCB_cd_ls_ft;
  ULONG DCB_TOC[202];
  ULONG DCB_cd_mode_sense_buf[6];
  UCHAR DCB_cd_first_session;
  UCHAR DCB_cd_last_session;
  ULONG DCB_play_resume_start;
  ULONG DCB_play_resume_stop;
  ULONG DCB_play_status;
  ULONG DCB_cd_device_flags;
  UCHAR DCB_cd_fs_lt;
  UCHAR DCB_cd_bobbit_pt;
  ULONG DCB_cd_fs_lo;
  ULONG DCB_cd_last_session_start;
  ULONG DCB_cd_current_block_size;
  UCHAR DCB_cd_vol_map[8];
  ULONG DCB_cd_current_command;
  ULONG DCB_cd_queue_head;
  ULONG DCB_cd_reserved;
};
#endif
typedef struct _DCB_cdrom DCB_cdrom;
typedef struct _DCB_cdrom *PDCB_cdrom;

#pragma pack(pop)

#endif
