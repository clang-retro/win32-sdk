/* isp.h: The IOS service packets (ISP)
   drivers send to IOS.

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   VXD_WINVER whose DDK has it. */
#ifndef VXD_ISP_H
#define VXD_ISP_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
#define ISP_CREATE_DDB 0
#define ISP_CREATE_DCB 1
#define ISP_CREATE_IOP 2
#define ISP_ALLOC_MEM 3
#define ISP_DEALLOC_MEM 4
#define ISP_INSERT_CALLDOWN 5
#define ISP_ASSOCIATE_DCB 6
#define ISP_GET_DCB 7
#define ISP_GET_FIRST_NEXT_DCB 8
#define ISP_DEALLOC_DDB 9
#define ISP_DESTROY_DCB 10
#define ISP_QUERY_MATCHING_DCBS 11
#define ISP_QUERY_REMOVE_DCB 12
#define ISP_DEVICE_REMOVED 13
#define ISP_DEVICE_ARRIVED 14
#define ISP_DISASSOCIATE_DCB 15
#define ISP_DRIVE_LETTER_PICK 16
#define ISP_REGISTRY_READ 17
#define ISP_FIND_LDM_ENTRY 18
#define ISP_DELETE_LDM_ENTRY 19
#define ISP_BROADCAST_AEP 20
#define ISP_DDB_FLAG_NEED_PHYS_CONTIG 0x01
#define ISP_M_FL_INTERRUPT_TIME 0x01
#define ISP_M_FL_MUST_SUCCEED 0x02
#define ISP_M_FL_EXTERNAL_IOP 0x04
#define ISP_M_FL_SMART_ALLOC 0x08
#define ISP_M_FL_PERSISTENT_IOP 0x10
#define ISP_M_FL_USE_SYSTEM_HEAP 0x20
#if VXD_WINVER >= 0x040A
#define ISP_M_FL_LOW_PRIORITY 0x40
#endif
#define ISP_D_A_FL_NOSHELLMSG 0x01
#define ISP_Q_REMOVE_FL_MEDIA_ONLY 1
#define ISP_DEV_REMOVED_FL_MEDIA_ONLY 1
#define ISP_DEV_ARR_FL_MEDIA_ONLY 1
#define ISP_PDL_FL_USER_ONLY 0x01
#define ISP_PDL_FL_OK_INVALID_RM 0x02
#define ISP_PDL_FL_OK_RM_CD 0x04
#define ISP_PDL_FL_USE_RANGE 0x08
#define ISP_PDL_FL_RELEASE 0x10
#if VXD_WINVER >= 0x040A
#define ISP_PDL_FL_IS_FLOPPY 0x20
#endif
struct ISP {
  USHORT ISP_func;
  USHORT ISP_result;
};
typedef struct ISP ISP;
typedef struct ISP *PISP;
struct ISP_ddb_create {
  ISP ISP_ddb_hdr;
  USHORT ISP_ddb_size;
  ULONG ISP_ddb_ptr;
  UCHAR ISP_ddb_flags;
  UCHAR ISP_pad0[1];
};
typedef struct ISP_ddb_create ISP_ddb_create;
typedef struct ISP_ddb_create *PISP_ddb_create;
struct ISP_dcb_create {
  ISP ISP_dcb_hdr;
  USHORT ISP_dcb_size;
  ULONG ISP_dcb_ptr;
  UCHAR ISP_pad1[2];
};
typedef struct ISP_dcb_create ISP_dcb_create;
typedef struct ISP_dcb_create *PISP_dcb_create;
struct ISP_IOP_create {
  ISP ISP_i_c_hdr;
  USHORT ISP_IOP_size;
  ULONG ISP_delta_to_ior;
  ULONG ISP_IOP_ptr;
  UCHAR ISP_i_c_flags;
  UCHAR ISP_pad2[1];
};
typedef struct ISP_IOP_create ISP_IOP_alloc;
typedef struct ISP_IOP_create *PISP_IOP_alloc;
struct ISP_mem_alloc {
  ISP ISP_mem_a_hdr;
  USHORT ISP_mem_size;
  ULONG ISP_mem_ptr;
  USHORT ISP_mem_type;
  UCHAR ISP_mem_flags;
  UCHAR ISP_pad3[3];
};
typedef struct ISP_mem_alloc ISP_mem_alloc;
typedef struct ISP_mem_alloc *PISP_mem_alloc;
struct ISP_mem_dealloc {
  ISP ISP_mem_da_hdr;
  ULONG ISP_mem_ptr_da;
};
typedef struct ISP_mem_dealloc ISP_mem_dealloc;
typedef struct ISP_mem_dealloc *PISP_mem_dealloc;
struct ISP_calldown_insert {
  ISP ISP_i_cd_hdr;
  ULONG ISP_i_cd_dcb;
  PVOID ISP_i_cd_req;
  ULONG ISP_i_cd_ddb;
  USHORT ISP_i_cd_expan_len;
  ULONG ISP_i_cd_flags;
  UCHAR ISP_i_cd_lgn;
  UCHAR ISP_pad4[1];
};
typedef struct ISP_calldown_insert ISP_calldown_insert;
typedef struct ISP_calldown_insert *PISP_calldown_insert;
struct ISP_dcb_associate {
  ISP ISP_d_a_hdr;
  ULONG ISP_d_a_dcb;
  UCHAR ISP_d_a_drive;
  UCHAR ISP_d_a_flags;
  UCHAR ISP_pad5[2];
};
typedef struct ISP_dcb_associate ISP_dcb_associate;
typedef struct ISP_dcb_associate *PISP_dcb_associate;
struct ISP_dcb_get {
  ISP ISP_g_d_hdr;
  ULONG ISP_g_d_dcb;
  ULONG ISP_g_d_drive;
};
typedef struct ISP_dcb_get ISP_dcb_get;
typedef struct ISP_dcb_get *PISP_get_dcb;
struct ISP_GET_FRST_NXT_DCB {
  ISP ISP_gfnd_hdr;
  ULONG ISP_gfnd_dcb_offset;
  ULONG ISP_gfnd_found_dcb;
  BYTE ISP_gfnd_dcb_type;
  UCHAR ISP_pad7[3];
};
typedef struct ISP_GET_FRST_NXT_DCB ISP_GET_FRST_NXT_DCB;
typedef struct ISP_GET_FRST_NXT_DCB *PISP_GET_FRST_NXT_DCB;
struct ISP_DDB_DEALLOC {
  ISP ISP_ddb_d_hdr;
  ULONG ISP_ddb_d_ddb;
  ULONG ISP_ddb_d_dvt;
};
typedef struct ISP_DDB_DEALLOC ISP_DDB_DEALLOC;
typedef struct ISP_DDB_DEALLOC *PISP_DDB_DEALLOC;
struct ISP_DCB_DESTROY {
  ISP ISP_ddb_d_hdr;
  ULONG ISP_dcb_d_dcb;
};
typedef struct ISP_DCB_DESTROY ISP_DCB_DESTROY;
typedef struct ISP_DCB_DESTROY *PISP_DCB_DESTROY;
struct ISP_QUERY_MATCH {
  ISP ISP_q_match_hdr;
  PVOID ISP_q_match_pdcb;
  ULONG ISP_q_match_drives;
};
typedef struct ISP_QUERY_MATCH ISP_QUERY_MATCH;
typedef struct ISP_QUERY_MATCH *PISP_QUERY_MATCH;
struct ISP_QUERY_REMOVE {
  ISP ISP_q_remove_hdr;
  PVOID ISP_q_remove_pdcb;
  ULONG ISP_q_remove_flags;
};
typedef struct ISP_QUERY_REMOVE ISP_QUERY_REMOVE;
typedef struct ISP_QUERY_REMOVE *PISP_QUERY_REMOVE;
struct ISP_DEV_REMOVED {
  ISP ISP_d_removed_hdr;
  PVOID ISP_dev_removed_pdcb;
  ULONG ISP_dev_removed_flags;
};
typedef struct ISP_DEV_REMOVED ISP_DEV_REMOVED;
typedef struct ISP_DEV_REMOVED *PISP_DEV_REMOVED;
struct ISP_DEV_ARRIVED {
  ISP ISP_d_arrived_hdr;
  PVOID ISP_dev_arrived_pdcb;
  ULONG ISP_dev_arrived_flags;
};
typedef struct ISP_DEV_ARRIVED ISP_DEV_ARRIVED;
typedef struct ISP_DEV_ARRIVED *PISP_DEV_ARRIVED;
struct ISP_DISASSOC_DCB {
  ISP ISP_dis_dcb_hdr;
  ULONG ISP_dis_dcb_drive;
};
typedef struct ISP_DISASSOC_DCB ISP_DISASSOC_DCB;
typedef struct ISP_DISASSOC_DCB *PISP_DISASSOC_DCB;
struct ISP_pick_drive_letter {
  ISP ISP_p_d_l_hdr;
  PVOID ISP_p_d_l_pdcb;
  UCHAR ISP_p_d_l_letter[2];
  UCHAR ISP_p_d_l_flags;
  UCHAR ISP_p_d_l_pad[1];
};
typedef struct ISP_pick_drive_letter ISP_pick_drive_letter;
typedef struct ISP_pick_drive_letter *PISP_pick_drive_letter;
struct ISP_read_registry {
  ISP ISP_r_r_hdr;
  PVOID ISP_r_r_dcb;
  PVOID ISP_r_r_string;
  PVOID ISP_r_r_data_type;
  PVOID ISP_r_r_data_size;
  PVOID ISP_r_r_buffer;
};
typedef struct ISP_read_registry ISP_read_registry;
typedef struct ISP_read_registry *PISP_read_registry;
struct ISP_FIND_LDM {
  ISP ISP_fldm_hdr;
  ULONG ISP_fldm_drive;
  PVOID ISP_fldm_pldm;
};
typedef struct ISP_FIND_LDM ISP_FIND_LDM;
typedef struct ISP_FIND_LDM *PISP_FIND_LDM;
struct ISP_AEP_BROADCAST {
  ISP ISP_baep_hdr;
  PVOID ISP_baep_paep;
};
typedef struct ISP_AEP_BROADCAST ISP_AEP_BROADCAST;
typedef struct ISP_AEP_BROADCAST *PISP_AEP_BROADCAST;

#pragma pack(pop)

#endif
