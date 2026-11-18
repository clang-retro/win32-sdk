/* ivt.h: IOS's vector table. (In the DDKs'
   block/inc.)

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   VXD_WINVER whose DDK has it. */
#ifndef VXD_IVT_H
#define VXD_IVT_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
struct IVT {
  ULONG IVT_ios_mem_virtual;
  PVOID IVT_ios_mem_phys;
  ULONG IVT_first_dvt;
};
typedef struct IVT _IVT;

#pragma pack(pop)

#endif
