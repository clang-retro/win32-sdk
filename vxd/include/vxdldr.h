/* vxdldr.h: VXDLDR, the loader of dynamic VxDs: its errors and
   the structures describing a loaded device.

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   VXD_WINVER whose DDK has it. */
#ifndef VXD_VXDLDR_H
#define VXD_VXDLDR_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
#define VXDLDR_INIT_DEVICE 0x01
#define VXDLDR_ERR_OUT_OF_MEMORY 1
#define VXDLDR_ERR_IN_DOS 2
#define VXDLDR_ERR_FILE_OPEN_ERROR 3
#define VXDLDR_ERR_FILE_READ 4
#define VXDLDR_ERR_DUPLICATE_DEVICE 5
#define VXDLDR_ERR_BAD_DEVICE_FILE 6
#define VXDLDR_ERR_DEVICE_REFUSED 7
#define VXDLDR_ERR_NO_SUCH_DEVICE 8
#define VXDLDR_ERR_DEVICE_UNLOADABLE 9
#define VXDLDR_ERR_ALLOC_V86_AREA 10
#define VXDLDR_ERR_BAD_API_FUNCTION 11
#define VXDLDR_ERR_MAX 11
#define VXDLDR_NOTIFY_OBJECTUNLOAD 0
#define VXDLDR_NOTIFY_OBJECTLOAD 1
#define VXDLDR_APIFUNC_GETVERSION 0
#define VXDLDR_APIFUNC_LOADDEVICE 1
#define VXDLDR_APIFUNC_UNLOADDEVICE 2
#define PELDR_ERR_NOERROR 0
#define PELDR_ERR_INVALIDHANDLE (-1)
#define PELDR_ERR_MEM_NOMEMORY (-2)
#define PELDR_ERR_FILE_UNABLETOOPEN (-3)
#define PELDR_ERR_FILE_READERROR (-4)
#define PELDR_ERR_IMAGE_INVALIDSIZE (-5)
#define PELDR_ERR_IMAGE_INVALIDFORMAT (-6)
#define PELDR_ERR_IMAGE_UNKNOWNFIXUP (-7)
#define PELDR_ERR_IMAGE_INVALIDFIXUP (-8)
#define PELDR_ERR_IMAGE_UNDEFINEDIMPORT (-9)
#define PELDR_ERR_EXPORT_DUPLICATE (-10)
#define PELDR_ERR_INIT_WASCOMPLETED (-11)
#if VXD_WINVER >= 0x040A
#define PELDR_ERR_IMAGE_UNDEFINEDDLL (-12)
#endif
#define PELDR_LOADFLAG_FREERESOURCES 0x01
#if VXD_WINVER >= 0x040A
#define PELDR_LOADFLAG_PREEMPTABLE 0x02
#define PELDR_LOADFLAG_KEEP_INIT 0x04
#endif
struct ObjectInfo {
  ULONG OI_LinearAddress;
  ULONG OI_Size;
  ULONG OI_ObjType;
  ULONG OI_Resident;
};
#if VXD_WINVER >= 0x040A
struct DeviceInfo {
  struct DeviceInfo *DI_Next;
  UCHAR DI_LoadCount;
  struct VxD_Desc_Block *DI_DDB;
  USHORT DI_DeviceID;
  CHAR *DI_ModuleName;
  ULONG DI_Signature;
  ULONG DI_ObjCount;
  struct ObjectInfo *DI_ObjInfo;
  ULONG DI_V86_API_CSIP;
  ULONG DI_PM_API_CSIP;
  UCHAR DI_InitCount;
};
#else
struct DeviceInfo {
  struct DeviceInfo *DI_Next;
  UCHAR DI_LoadCount;
  struct VxD_Desc_Block *DI_DDB;
  USHORT DI_DeviceID;
  CHAR *DI_ModuleName;
  ULONG DI_Signature;
  ULONG DI_ObjCount;
  struct ObjectInfo *DI_ObjInfo;
  ULONG DI_V86_API_CSIP;
  ULONG DI_PM_API_CSIP;
};
#endif
struct _W32IoctlPkt {
  USHORT W32IO_ErrorCode;
  USHORT W32IO_DeviceID;
  UCHAR W32IO_ModuleName[1];
};
typedef DWORD VXDLDRRET;
typedef char *PSTR;
typedef LONG LRESULT;
typedef DWORD HPEEXPORTTABLE;
typedef HPEEXPORTTABLE *PHPEEXPORTTABLE;
typedef DWORD HPEMODULE;
typedef HPEMODULE *PHPEMODULE;
typedef DWORD HLIST;
typedef HLIST *PHLIST;

#pragma pack(pop)

#endif
