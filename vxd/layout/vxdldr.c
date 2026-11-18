/* Made by tools/gen-vxd-headers.py: the layouts of <vxdldr.h> as the 98 DDK's
   compiler laid them out. */
#define VXD_WINVER 0x040A
#include <vxdldr.h>
#include <stddef.h>
#ifdef __cplusplus
#define A(e) static_assert(e, #e);
#else
#define A(e) _Static_assert(e, #e);
#endif
A(sizeof(struct ObjectInfo) == 16)
A(offsetof(struct ObjectInfo, OI_LinearAddress) == 0)
A(offsetof(struct ObjectInfo, OI_Size) == 4)
A(offsetof(struct ObjectInfo, OI_ObjType) == 8)
A(offsetof(struct ObjectInfo, OI_Resident) == 12)
A(sizeof(struct DeviceInfo) == 36)
A(offsetof(struct DeviceInfo, DI_Next) == 0)
A(offsetof(struct DeviceInfo, DI_LoadCount) == 4)
A(offsetof(struct DeviceInfo, DI_DDB) == 5)
A(offsetof(struct DeviceInfo, DI_DeviceID) == 9)
A(offsetof(struct DeviceInfo, DI_ModuleName) == 11)
A(offsetof(struct DeviceInfo, DI_Signature) == 15)
A(offsetof(struct DeviceInfo, DI_ObjCount) == 19)
A(offsetof(struct DeviceInfo, DI_ObjInfo) == 23)
A(offsetof(struct DeviceInfo, DI_V86_API_CSIP) == 27)
A(offsetof(struct DeviceInfo, DI_PM_API_CSIP) == 31)
A(offsetof(struct DeviceInfo, DI_InitCount) == 35)
A(sizeof(struct _W32IoctlPkt) == 5)
A(offsetof(struct _W32IoctlPkt, W32IO_ErrorCode) == 0)
A(offsetof(struct _W32IoctlPkt, W32IO_DeviceID) == 2)
A(offsetof(struct _W32IoctlPkt, W32IO_ModuleName) == 4)
