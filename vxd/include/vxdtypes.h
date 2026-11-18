/* vxdtypes.h: the base types of the DDKs (basedef.h, vmm.h ...) that the
   VxD subsystem headers use. Made by tools/gen-vxd-headers.py. */
#ifndef VXD_VXDTYPES_H
#define VXD_VXDTYPES_H

#include <vxd.h>

#pragma pack(push, 1)
typedef void *PVOID;
typedef char CHAR;
typedef CHAR *PCHAR;
typedef unsigned long ULONG;
typedef unsigned char UCHAR;
typedef unsigned short USHORT;
struct VxD_Desc_Block {
  ULONG DDB_Next;
  USHORT DDB_SDK_Version;
  USHORT DDB_Req_Device_Number;
  UCHAR DDB_Dev_Major_Version;
  UCHAR DDB_Dev_Minor_Version;
  USHORT DDB_Flags;
  UCHAR DDB_Name[8];
  ULONG DDB_Init_Order;
  ULONG DDB_Control_Proc;
  ULONG DDB_V86_API_Proc;
  ULONG DDB_PM_API_Proc;
  ULONG DDB_V86_API_CSIP;
  ULONG DDB_PM_API_CSIP;
  ULONG DDB_Reference_Data;
  ULONG DDB_Service_Table_Ptr;
  ULONG DDB_Service_Table_Size;
  ULONG DDB_Win32_Service_Table;
  ULONG DDB_Prev;
  ULONG DDB_Size;
  ULONG DDB_Reserved1;
  ULONG DDB_Reserved2;
  ULONG DDB_Reserved3;
};
typedef struct VxD_Desc_Block *PVMMDDB;
typedef PVMMDDB *PPVMMDDB;
typedef PVOID HANDLE;
typedef long LONG;
typedef int BOOL;
#if VXD_WINVER >= 0x040A
typedef long long LONGLONG;
struct _LARGE_INTERGER2 {
  ULONG LowPart;
  LONG HighPart;
};
typedef unsigned long long ULONGLONG;
struct _LARGE_INTERGER4 {
  ULONG LowPart;
  ULONG HighPart;
};
#endif
typedef int INT;
typedef unsigned int UINT;
typedef UCHAR *PUCHAR;
typedef UINT *PUINT;

#pragma pack(pop)

#endif
