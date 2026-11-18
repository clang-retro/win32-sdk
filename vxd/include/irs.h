/* irs.h: The IOS requestor services
   (IRS) packets that VxDs above IOS send it.

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   VXD_WINVER whose DDK has it. */
#ifndef VXD_IRS_H
#define VXD_IRS_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
#define IRS_GET_VRP 0
#define IRS_REPLACE_VRP 1
#define IRS_GET_DRVINFO 2
#define IRS_MEDIA_CHECK_RESET 3
#define IRS_SET_CUR_FLP_UNIT 4
#define IRS_REAL_MODE_HANDOFF 5
#define IRS_QUERY_VOLUME_LOCK 6
#define IRS_GET_LOGICAL_DRVS 7
#define IRS_MOUNT_NOTIFY 8
#define IRS_FIRSTNXT_CHAR_DEV 9
#define IRS_IS_DRVCOMPRESSED 10
#define IRS_SUCCESS 0x00
#define IRS_V_G_CHAR 0x01
#define IRS_V_G_SUCCESS 0x00
#define IRS_V_G_NO_DEVICE 0xA0
#define IRS_V_G_UNCERTAIN_MEDIA 0xA2
#define IRS_V_R_CHAR 0x01
#define IRS_V_R_DEF 0x02
#define IRS_DRV_SINGLE_FLOPPY 0x00800000
#define IRS_DRV_PAGEABLE 0x00400000
#define IRS_DRV_RMM 0x2000
#define IRS_DRV_REMOTE 0x08
#define IRS_DRV_REMOVABLE 0x04
#define IRS_DRV_INT13_DRIVE 0x01
#define IRS_M_C_R_SUCCESS 0x00
#define IRS_M_C_R_UNCERTAIN_MEDIA 0xA2
#define IRS_M_N_SUCCESS 0x00
#define IRS_M_N_REMOUNT 0x01
#define IRS_I_D_C_COMP 0x01
#define IRS_I_D_C_PM_COMP 0x02
struct IRS_header {
  UCHAR IRS_func;
  UCHAR IRS_result;
};
typedef struct IRS_header IRS;
typedef struct IRS_header *PIRS;
struct IRS_vrp_get {
  UCHAR filler1;
  UCHAR filler2;
  UCHAR IRS_v_g_dev_type;
  ULONG IRS_v_g_designtr;
  ULONG IRS_v_g_handle;
  ULONG IRS_v_g_vrp;
  UCHAR IRS_alignchar1;
};
typedef struct IRS_vrp_get IRS_vrp_get;
typedef struct IRS_vrp_get *PIRS_vrp_get;
struct IRS_vrp_replace {
  UCHAR filler3;
  UCHAR filler4;
  UCHAR IRS_v_r_designtr;
  UCHAR IRS_v_r_flags;
  ULONG IRS_v_r_old_vrp;
  ULONG IRS_v_r_new_vrp;
};
typedef struct IRS_vrp_replace IRS_vrp_replace;
typedef struct IRS_vrp_replace *PIRS_vrp_replace;
struct IRS_drv_get {
  UCHAR IRS_filler_5;
  UCHAR IRS_filler_6;
  UCHAR IRS_DrvLetter;
  ULONG IRS_DrvFlags;
  UCHAR IRS_alignchar2;
};
typedef struct IRS_drv_get IRS_drv;
typedef struct IRS_drv_get *PIRS_drv;
struct IRS_media_chk_reset {
  UCHAR filler7;
  UCHAR filler8;
  UCHAR IRS_m_c_r_designtr;
  ULONG IRS_m_c_r_vrp;
  UCHAR IRS_alignchar3;
};
typedef struct IRS_media_chk_reset IRS_media_chk_reset;
typedef struct IRS_media_chk_reset *PIRS_media_chk_reset;
struct IRS_cur_flp_unit_set {
  UCHAR filler9;
  UCHAR filler10;
  UCHAR IRS_s_c_f_unit;
  UCHAR IRS_alignchar4;
};
typedef struct IRS_cur_flp_unit_set IRS_cur_flp_unit_set;
typedef struct IRS_cur_flp_unit_set *PIRS_cur_flp_unit_set;
struct IRS_rm_handoff {
  struct IRS_header IRS_r_m_h_hdr;
  UCHAR IRS_r_m_h_pad[2];
  UCHAR IRS_alignchar5;
};
typedef struct IRS_rm_handoff IRS_rm_handoff;
typedef struct IRS_rm_handoff *PIRS_rm_handoff;
struct IRS_query_vollock {
  struct IRS_header IRS_q_v_l_hdr;
  UCHAR IRS_q_v_l_designtr;
  ULONG IRS_q_v_l_drivemap;
  UCHAR IRS_q_v_l_pad;
};
typedef struct IRS_query_vollock IRS_query_vollock;
typedef struct IRS_query_vollock *PIRS_query_vollock;
struct IRS_get_logical_drives {
  struct IRS_header IRS_g_l_d_hdr;
  UCHAR IRS_g_l_d_designtr;
  ULONG IRS_g_l_d_drivemap;
  UCHAR IRS_g_l_d_pad;
};
typedef struct IRS_get_logical_drives IRS_get_logical_drives;
typedef struct IRS_get_logical_drives *PIRS_get_logical_drives;
struct IRS_mnt_notify {
  struct IRS_header IRS_m_n_hdr;
  USHORT IRS_m_n_drive;
  void *IRS_m_n_pvrp;
  ULONG IRS_m_n_drivemap;
  ULONG IRS_m_n_effective_drive;
  ULONG IRS_m_n_actual_drive;
};
typedef struct IRS_mnt_notify IRS_mnt_notify;
typedef struct IRS_mnt_notify *PIRS_mnt_notify;
struct IRS_frstnxt_char_dev {
  struct IRS_header IRS_g_f_n_hdr;
  ULONG IRS_g_f_n_hndl;
  ULONG IRS_g_f_n_ptr;
  USHORT IRS_alignshort1;
};
typedef struct IRS_frstnxt_char_dev IRS_frstnxt_char_dev;
typedef struct IRS_frstnxt_char_dev *PIRS_frstnxt_char_dev;
struct IRS_drv_comp {
  struct IRS_header IRS_i_d_c_hdr;
  UCHAR IRS_i_d_c_drive;
  UCHAR IRS_i_d_c_flags;
};
typedef struct IRS_drv_comp IRS_drv_comp;
typedef struct IRS_drv_comp *PIRS_drv_comp;

#pragma pack(pop)

#endif
