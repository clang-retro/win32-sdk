/* ilb.h: The IOS linkage block (ILB) IOS gives
   each driver.

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   VXD_WINVER whose DDK has it. */
#ifndef VXD_ILB_H
#define VXD_ILB_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
struct ISP;
typedef void (*PFNISP)(struct ISP *);
struct ILB {
  PFNISP ILB_service_rtn;
  PVOID ILB_dprintf_rtn;
  PVOID ILB_Wait_10th_Sec;
  PVOID ILB_internal_request;
  PVOID ILB_io_criteria_rtn;
  PVOID ILB_int_io_criteria_rtn;
  ULONG ILB_dvt;
  ULONG ILB_ios_mem_virt;
  ULONG ILB_enqueue_iop;
  ULONG ILB_dequeue_iop;
  ULONG ILB_reserved_1;
  ULONG ILB_reserved_2;
  USHORT ILB_flags;
  CHAR ILB_driver_numb;
  CHAR ILB_reserved_3;
};
typedef struct ILB ILB;
typedef struct ILB *PILB;

#pragma pack(pop)

#endif
