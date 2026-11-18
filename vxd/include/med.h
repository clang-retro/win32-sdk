/* med.h: IOS memory element descriptors.

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   VXD_WINVER whose DDK has it. */
#ifndef VXD_MED_H
#define VXD_MED_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
#define MED_FREE_POOL 0x01
#define MED_FREE_FAST 0x02
#define MED_ALLOC 0x04
#define MED_PANIC 0x08
#define MED_EXTERNAL 0x10
#define MED_SMART 0x20
#define MED_FREE_TYPE 0
#define MED_IOP 1
#define MED_ILB 2
#define MED_AEP 3
#define MED_DCB 4
#define MED_DDB 5
#define MED_DRP 6
#define MED_DVT 7
#define MED_IRB 8
#define MED_MISC 9
#define MED_ILR 10
#define MED_IOC 11
#define MED_NEW_IOP 12
#define MED_VRP 13
#define MED_PCD 14
#define MED_MAX_TYPE 14
#define MED_GRAN_DELTA 16
#define MED_FAST_GRANS 64
#define MED_SHIFT_CNT 0x02
struct MED {
  ULONG MED_Physical;
  ULONG MED_Next;
  ULONG MED_Adjacent;
  USHORT MED_Type;
  USHORT MED_Size;
  USHORT MED_Flags;
  USHORT MED_type_spec;
};
typedef struct MED MED;
typedef struct MED *PMED;

#pragma pack(pop)

#endif
