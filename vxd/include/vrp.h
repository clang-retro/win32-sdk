/* vrp.h: The volume request parameters (VRP)
   IOS keeps for each volume.

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   VXD_WINVER whose DDK has it. */
#ifndef VXD_VRP_H
#define VXD_VRP_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
#define VRP_dmd_small_memory 0x10
#define VRP_dmd_word_align 0x0200
#define VRP_dmd_dword_align 0x0400
#define VRP_dmd_phys_sgd 0x0800
#define VRP_dmd_phys_sgd_ptr 0x1000
#define VRP_dmd_do_a_b_toggling 0x2000
#define VRP_dmd_removable_supp 0x4000
#define VRP_dmd_lock_unlock_media 0x00010000
#define VRP_dmd_load_eject_media 0x00020000
#define VRP_dmd_clear_media_chg 0x00040000
#define VRP_dmd_no_xcl_required 0x00200000
#define VRP_dmd_pageability 0x00400000
#if VXD_WINVER >= 0x040A
#define VRP_dmd_async_media_change 0x00800000
#endif
#define VRP_ef_media_changed 0x01
#define VRP_ef_media_uncertain 0x02
#define VRP_ef_prompting 0x04
#define VRP_ef_input_share 0x08
#define VRP_ef_output_share 0x10
#define VRP_ef_user_canceled 0x20
#define VRP_ef_write_protected 0x40
#define VRP_ef_real_mode_mapped 0x80
#define VRP_ef_ios_locked 0x8000
struct _VRP {
  ULONG VRP_demand_flags;
  ULONG VRP_event_flags;
  USHORT VRP_max_sgd;
  USHORT VRP_max_req_size;
  ULONG VRP_delta_to_ior;
  ULONG VRP_block_size;
  ULONG VRP_fsd_hvol;
  ULONG VRP_fsd_entry;
  ULONG VRP_device_handle;
  ULONG VRP_partition_offset;
  ULONG VRP_next_vrp;
  ULONG VRP_logical_handle;
  ULONG VRP_reserved;
};
typedef struct _VRP VRP;
typedef struct _VRP *PVRP;

#pragma pack(pop)

#endif
