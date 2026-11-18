/* aep.h: The asynchronous
   event packets (AEP) IOS sends each driver's async event routine, and
   the AEP_* functions.

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   VXD_WINVER whose DDK has it. */
#ifndef VXD_AEP_H
#define VXD_AEP_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
#define AEP_PrivateLen 32
#define AEP_INITIALIZE 0
#define AEP_SYSTEM_CRIT_SHUTDOWN 1
#define AEP_BOOT_COMPLETE 2
#define AEP_CONFIG_DCB 3
#define AEP_UNCONFIG_DCB 4
#define AEP_IOP_TIMEOUT 5
#define AEP_DEVICE_INQUIRY 6
#define AEP_HALF_SEC 7
#define AEP_1_SEC 8
#define AEP_2_SECS 9
#define AEP_4_SECS 10
#define AEP_DBG_DOT_CMD 11
#define AEP_ASSOCIATE_DCB 12
#define AEP_REAL_MODE_HANDOFF 13
#define AEP_SYSTEM_SHUTDOWN 14
#define AEP_UNINITIALIZE 15
#define AEP_DCB_LOCK 16
#define AEP_MOUNT_NOTIFY 17
#define AEP_CREATE_VRP 18
#define AEP_DESTROY_VRP 19
#define AEP_REFRESH_DRIVE 20
#define AEP_PEND_UNCONFIG_DCB 21
#define AEP_1E_VEC_UPDATE 22
#define AEP_CHANGE_RPM 23
#if VXD_WINVER >= 0x040A
#define AEP_QUERY_RPM 24
#define AEP_CONFIG_3MODE 25
#define AEP_SYSTEM_REBOOT 26
#define AEP_QUERY_CD_SPINDOWN 27
#define AEP_FUNC_POWER_MESSAGE 28
#define AEP_FUNC_ACPI_REQUEST 29
#endif
#if VXD_WINVER >= 0x040A
#define AEP_MAX_FUNC 29
#else
#define AEP_MAX_FUNC 23
#endif
#define AEP_SUCCESS 0
#if VXD_WINVER >= 0x040A
#define AEP_DEREGISTER 1
#endif
#define AEP_FAILURE (-1)
#define AEP_BI_FL_SCSI_SCAN_DOWN 0x01
#define AEP_BI_FL_SEND_CONFIG_AGAIN 0x02
#define AEP_NO_INQ_DATA 1
#define AEP_NO_MORE_DEVICES 2
#define AEP_D_C_NUM_1_PRES 0x01
#define AEP_D_C_NUM_2_PRES 0x02
#define AEP_D_C_NUM_3_PRES 0x04
#define MAX_DOT_CMD_KEYWORD_SIZE 16
#define AEP_M_N_SUCCESS 0x00
#define AEP_RC_360_RPM 0x01
#define AEP_RC_300_RPM 0x02
#if VXD_WINVER >= 0x040A
#define AEP_SUBF_ACPI_SET_TASK_FILES 0x01
#endif
struct AEPHDR {
  USHORT AEP_func;
  USHORT AEP_result;
  ULONG AEP_ddb;
  UCHAR AEP_lgn;
  UCHAR AEP_align[3];
};
typedef struct AEPHDR AEP;
typedef struct AEPHDR *PAEP;
struct AEP_bi_init {
  struct AEPHDR AEP_bi_i_hdr;
  ULONG AEP_bi_reference_data;
  UCHAR AEP_bi_flags;
  CHAR AEP_bi_i_max_target;
  CHAR AEP_bi_i_max_lun;
  ULONG AEP_bi_i_dcb;
  PVOID AEP_bi_i_hdevnode;
  PVOID AEP_bi_i_regkey;
  UCHAR AEP_bi_i_align[1];
};
typedef struct AEP_bi_init AEP_bi_init;
typedef struct AEP_bi_init *PAEP_bi_init;
struct AEP_inquiry_device {
  struct AEPHDR AEP_i_d_hdr;
  ULONG AEP_i_d_dcb;
};
typedef struct AEP_inquiry_device AEP_inquiry_device;
typedef struct AEP_inquiry_device *PAEP_inquiry_device;
struct AEP_bi_uninit {
  struct AEPHDR AEP_bi_u_hdr;
};
typedef struct AEP_bi_uninit AEP_bi_uninit;
typedef struct AEP_bi_uninit *PAEP_bi_uninit;
struct AEP_boot_done {
  struct AEPHDR AEP_b_d_hdr;
};
typedef struct AEP_boot_done AEP_boot_done;
typedef struct AEP_boot_done *PAEP_boot_done;
struct AEP_dcb_config {
  struct AEPHDR AEP_d_c_hdr;
  ULONG AEP_d_c_dcb;
};
typedef struct AEP_dcb_config AEP_dcb_config;
typedef struct AEP_dcb_config *PAEP_dcb_config;
struct AEP_dcb_unconfig {
  struct AEPHDR AEP_d_u_hdr;
  ULONG AEP_d_u_dcb;
};
typedef struct AEP_dcb_unconfig AEP_dcb_unconfig;
typedef struct AEP_dcb_unconfig *PAEP_dcb_unconfig;
struct AEP_dcb_unconfig_pend {
  struct AEPHDR AEP_d_u_p_hdr;
  ULONG AEP_d_u_p_dcb;
};
typedef struct AEP_dcb_unconfig_pend AEP_dcb_unconfig_pend;
typedef struct AEP_dcb_unconfig_pend *PAEP_dcb_unconfig_pend;
struct AEP_iop_timeout_occurred {
  struct AEPHDR AEP_i_t_o_hdr;
  ULONG AEP_i_t_o_iop;
};
typedef struct AEP_iop_timeout_occurred AEP_iop_timeout_occurred;
typedef struct AEP_iop_timeout_occurred *PAEP_iop_timeout_occurred;
#if VXD_WINVER >= 0x045A
struct AEP_dot_command {
  struct AEPHDR AEP_i_t_o_hdr;
  UCHAR AEP_d_c_flags;
  PVOID AEP_d_c_pkeyword;
  UCHAR AEP_d_c_key_len;
  ULONG AEP_d_c_num_1;
  ULONG AEP_d_c_num_2;
  ULONG AEP_d_c_num_3;
  USHORT AEP_d_c_reserved;
};
#else
struct AEP_dot_command {
  struct AEPHDR AEP_i_t_o_hdr;
  UCHAR AEP_d_c_flags;
  PVOID AEP_d_c_pkeyword;
  UCHAR AEP_d_c_key_len;
  ULONG AEP_d_c_num_1;
  ULONG AEP_d_c_num_2;
  ULONG AEP_d_c_num_3;
};
#endif
typedef struct AEP_dot_command AEP_dot_command;
typedef struct AEP_dot_command *PAEP_dot_command;
#if VXD_WINVER >= 0x040A
struct AEP_sys_reboot {
  struct AEPHDR AEP_s_r_hdr;
};
typedef struct AEP_sys_reboot AEP_sys_reboot;
typedef struct AEP_sys_reboot *PAEP_sys_reboot;
#endif
struct AEP_sys_shutdown {
  struct AEPHDR AEP_s_s_hdr;
};
typedef struct AEP_sys_shutdown AEP_sys_shutdown;
typedef struct AEP_sys_shutdown *PAEP_sys_shutdown;
struct AEP_sys_crit_shutdown {
  struct AEPHDR AEP_s_c_s_hdr;
};
typedef struct AEP_sys_crit_shutdown AEP_sys_crit_shutdown;
typedef struct AEP_sys_crit_shutdown *PAEP_sys_crit_shutdown;
struct AEP_assoc_dcb {
  struct AEPHDR AEP_a_d_hdr;
  PVOID AEP_a_d_pdcb;
  ULONG AEP_a_d_drives;
};
typedef struct AEP_assoc_dcb AEP_assoc_dcb;
typedef struct AEP_assoc_dcb *PAEP_assoc_dcb;
struct AEP_rm_handoff {
  struct AEPHDR AEP_r_m_h_hdr;
};
typedef struct AEP_rm_handoff AEP_rm_handoff;
typedef struct AEP_rm_handoff *PAEP_rm_handoff;
struct AEP_lock_dcb {
  struct AEPHDR AEP_d_l_hdr;
  PVOID AEP_d_l_pdcb;
  ULONG AEP_d_l_drives;
  UCHAR AEP_d_l_designtr;
  UCHAR AEP_d_l_align[3];
};
typedef struct AEP_lock_dcb AEP_lock_dcb;
typedef struct AEP_lock_dcb *PAEP_lock_dcb;
struct AEP_mnt_notify {
  struct AEPHDR AEP_m_n_hdr;
  PVOID AEP_m_n_pvrp;
  ULONG AEP_m_n_drivemap;
  ULONG AEP_m_n_drive;
  ULONG AEP_m_n_effective_drive;
  ULONG AEP_m_n_actual_drive;
};
typedef struct AEP_mnt_notify AEP_mnt_notify;
typedef struct AEP_mnt_notify *PAEP_mnt_notify;
struct AEP_vrp_create_destroy {
  struct AEPHDR AEP_v_cd_hdr;
  PVOID AEP_v_cd_pvrp;
  ULONG AEP_v_cd_drive;
};
typedef struct AEP_vrp_create_destroy AEP_vrp_create_destroy;
typedef struct AEP_vrp_create_destroy *PAEP_vrp_create_destroy;
struct AEP_drive_refresh {
  struct AEPHDR AEP_d_r_hdr;
  ULONG AEP_d_r_drive;
};
typedef struct AEP_drive_refresh AEP_drive_refresh;
typedef struct AEP_drive_refresh *PAEP_drive_refresh;
struct AEP_update_1e_vec {
  struct AEPHDR AEP_u_1e_hdr;
};
typedef struct AEP_update_1e_vec AEP_update_1e_vec;
typedef struct AEP_update_1e_vec *PAEP_update_1e_vec;
#if VXD_WINVER >= 0x040A
struct AEP_rpm_change {
  struct AEPHDR AEP_rc_hdr;
  USHORT AEP_rc_speed;
  ULONG AEP_rc_dcb;
};
#else
struct AEP_rpm_change {
  struct AEPHDR AEP_rc_hdr;
  USHORT AEP_rc_speed;
};
#endif
typedef struct AEP_rpm_change AEP_rpm_change;
#if VXD_WINVER >= 0x045A
typedef struct AEP_rpm_change *PAEP_rpm_change;
#endif
#if VXD_WINVER >= 0x040A
struct AEP_rpm_inquiry {
  struct AEPHDR AEP_ri_hdr;
  USHORT AEP_ri_speed;
  ULONG AEP_ri_dcb;
};
typedef struct AEP_rpm_inquiry AEP_rpm_inquiry;
#endif
#if VXD_WINVER >= 0x045A
typedef struct AEP_rpm_inquiry *PAEP_rpm_inquiry;
#endif
#if VXD_WINVER >= 0x040A
struct AEP_3mode_config {
  struct AEPHDR AEP_3c_hdr;
  USHORT AEP_3c_flag;
  USHORT AEP_3c_wait;
  ULONG AEP_3c_dcb;
};
typedef struct AEP_3mode_config AEP_3mode_config;
#endif
#if VXD_WINVER >= 0x045A
typedef struct AEP_3mode_config *PAEP_3mode_config;
#endif
#if VXD_WINVER >= 0x040A
struct AEP_QUERY_CD_SPIN {
  struct AEPHDR AEP_qcds_hdr;
  ULONG AEP_qcds_result;
};
typedef struct AEP_QUERY_CD_SPIN AEP_QUERY_CD_SPIN;
typedef struct AEP_QUERY_CD_SPIN *PAEP_QUERY_CD_SPIN;
struct _AEP_POWER_MESSAGE {
  struct AEPHDR AEP_hdr;
  ULONG ulSubFunction;
  ULONG devNode;
};
typedef struct _AEP_POWER_MESSAGE AEP_POWER_MESSAGE;
typedef struct _AEP_POWER_MESSAGE *PAEP_POWER_MESSAGE;
struct _AEP_ACPI_REQUEST {
  struct AEPHDR AEP_hdr;
  ULONG aep_acpi_ulSubFunction;
  ULONG aep_acpi_pDdb;
  ULONG aep_acpi_pDcb;
  ULONG aep_acpi_ulInputLength;
  BYTE *aep_acpi_pbInputBuffer;
  ULONG aep_acpi_ulOutputLength;
  BYTE *aep_acpi_pbOutputBuffer;
};
typedef struct _AEP_ACPI_REQUEST AEP_ACPI_REQUEST;
#endif

#pragma pack(pop)

#endif
