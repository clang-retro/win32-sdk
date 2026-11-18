/* iop.h: The I/O packet
   (IOP) around each IOR in IOS's calldown chain. (In the DDKs'
   block/inc.)

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   VXD_WINVER whose DDK has it. */
#ifndef VXD_IOP_H
#define VXD_IOP_H

#include <vxd.h>
#include <vxdtypes.h>
#include <ior.h>

#pragma pack(push, 1)
#define IOP_CALLBACK_TABLE_DEPTH 6
struct IOP_callback_entry {
  ULONG IOP_CB_address;
  ULONG IOP_CB_ref_data;
};
typedef struct IOP_callback_entry IOP_callback_entry;
struct _IOP {
  ULONG IOP_physical;
  ULONG IOP_physical_dcb;
  ULONG IOP_original_dcb;
  USHORT IOP_timer;
  USHORT IOP_timer_orig;
  ULONG IOP_calldown_ptr;
  ULONG IOP_callback_ptr;
  ULONG IOP_voltrk_private;
  ULONG IOP_Thread_Handle;
  ULONG IOP_srb;
  ULONG IOP_reserved[2];
  IOP_callback_entry IOP_callback_table[6];
  BYTE IOP_format_head;
  BYTE IOP_format_xfer_rate;
  USHORT IOP_format_track;
  ULONG IOP_format_num_sectors;
  IOR IOP_ior;
};
typedef struct _IOP IOP;
typedef struct _IOP *pIOP;

#pragma pack(pop)

#endif
