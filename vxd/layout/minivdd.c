/* Made by tools/gen-vxd-headers.py: the layouts of <minivdd.h> as the 98 DDK's
   compiler laid them out. */
#define VXD_WINVER 0x040A
#include <minivdd.h>
#include <stddef.h>
#ifdef __cplusplus
#define A(e) static_assert(e, #e);
#else
#define A(e) _Static_assert(e, #e);
#endif
A(sizeof(struct tagVDD_IOCTL_SET_NOTIFY_INPUT) == 16)
A(offsetof(struct tagVDD_IOCTL_SET_NOTIFY_INPUT, NotifyMask) == 0)
A(offsetof(struct tagVDD_IOCTL_SET_NOTIFY_INPUT, NotifyType) == 4)
A(offsetof(struct tagVDD_IOCTL_SET_NOTIFY_INPUT, NotifyProc) == 8)
A(offsetof(struct tagVDD_IOCTL_SET_NOTIFY_INPUT, NotifyData) == 12)
A(sizeof(struct DISPLAYINFO) == 74)
A(offsetof(struct DISPLAYINFO, diHdrSize) == 0)
A(offsetof(struct DISPLAYINFO, diInfoFlags) == 2)
A(offsetof(struct DISPLAYINFO, diDevNodeHandle) == 4)
A(offsetof(struct DISPLAYINFO, diDriverName) == 8)
A(offsetof(struct DISPLAYINFO, diXRes) == 24)
A(offsetof(struct DISPLAYINFO, diYRes) == 26)
A(offsetof(struct DISPLAYINFO, diDPI) == 28)
A(offsetof(struct DISPLAYINFO, diPlanes) == 30)
A(offsetof(struct DISPLAYINFO, diBpp) == 31)
A(offsetof(struct DISPLAYINFO, diRefreshRateMax) == 32)
A(offsetof(struct DISPLAYINFO, diRefreshRateMin) == 34)
A(offsetof(struct DISPLAYINFO, diLowHorz) == 36)
A(offsetof(struct DISPLAYINFO, diHighHorz) == 38)
A(offsetof(struct DISPLAYINFO, diLowVert) == 40)
A(offsetof(struct DISPLAYINFO, diHighVert) == 42)
A(offsetof(struct DISPLAYINFO, diMonitorDevNodeHandle) == 44)
A(offsetof(struct DISPLAYINFO, diHorzSyncPolarity) == 48)
A(offsetof(struct DISPLAYINFO, diVertSyncPolarity) == 49)
A(offsetof(struct DISPLAYINFO, diUnitNumber) == 50)
A(offsetof(struct DISPLAYINFO, diDisplayFlags) == 54)
A(offsetof(struct DISPLAYINFO, diXDesktopPos) == 58)
A(offsetof(struct DISPLAYINFO, diYDesktopPos) == 62)
A(offsetof(struct DISPLAYINFO, diXDesktopSize) == 66)
A(offsetof(struct DISPLAYINFO, diYDesktopSize) == 70)
