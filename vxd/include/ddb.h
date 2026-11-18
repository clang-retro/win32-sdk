/* ddb.h: IOS's driver data block. (In the DDKs'
   block/inc.)

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   VXD_WINVER whose DDK has it. */
#ifndef VXD_DDB_H
#define VXD_DDB_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
#define DDB_IOS_FL_PENDING_REMOVAL 1
#define DDB_IOS_FL_PAGE_ALLOCED 2
#define DDB_IOS_FL_FIRST_ENUM_DONE_BIT 2
#define DDB_IOS_FL_PLEASE_DESTROY 4
#if VXD_WINVER >= 0x040A
struct DDB {
  ULONG DDB_phys_addr;
  ULONG DDB_Next_DDB;
  ULONG DDB_Next_DDB_init;
  ULONG DDB_dcb_ptr;
  UCHAR DDB_number_buses;
  UCHAR DDB_ios_flags;
  USHORT DDB_sig;
  PVOID DDB_dvt;
  PVOID DDB_devnode_ptr;
  PVOID DDB_pAcpiBlock;
};
#else
struct DDB {
  ULONG DDB_phys_addr;
  ULONG DDB_Next_DDB;
  ULONG DDB_Next_DDB_init;
  ULONG DDB_dcb_ptr;
  UCHAR DDB_number_buses;
  UCHAR DDB_ios_flags;
  USHORT DDB_sig;
  PVOID DDB_dvt;
  PVOID DDB_devnode_ptr;
  PVOID DDB_reserved;
};
#endif
typedef struct DDB DDB;
typedef struct DDB *pDDB;

#pragma pack(pop)

#endif
