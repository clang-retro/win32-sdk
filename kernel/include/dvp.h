/* dvp.h: DirectDraw video ports
   (VPE): the video port descriptions, connections and capabilities that
   <ddrawint.h>'s video port callbacks take.

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 2000 and XP DDKs; each declaration is under the first
   _WIN32_WINNT whose DDK has it. */
#ifndef WDM_DVP_H
#define WDM_DVP_H

#include <windef.h>
#include <wingdi.h>
#include <ddraw.h>
#include <guiddef.h>
DEFINE_GUID(IID_IDDVideoPortContainer, 0x6c142760, 0xa733, 0x11ce, 0xa5, 0x21, 0x00, 0x20, 0xaf, 0x0b, 0xe5, 0x60);
DEFINE_GUID(IID_IDirectDrawVideoPort, 0xb36d93e0, 0x2b43, 0x11cf, 0xa2, 0xde, 0x00, 0xaa, 0x00, 0xb9, 0x33, 0x56);
DEFINE_GUID(DDVPTYPE_E_HREFH_VREFH, 0x54f39980, 0xda60, 0x11cf, 0x9b, 0x06, 0x00, 0xa0, 0xc9, 0x03, 0xa3, 0xb8);
DEFINE_GUID(DDVPTYPE_E_HREFH_VREFL, 0x92783220, 0xda60, 0x11cf, 0x9b, 0x06, 0x00, 0xa0, 0xc9, 0x03, 0xa3, 0xb8);
DEFINE_GUID(DDVPTYPE_E_HREFL_VREFH, 0xa07a02e0, 0xda60, 0x11cf, 0x9b, 0x06, 0x00, 0xa0, 0xc9, 0x03, 0xa3, 0xb8);
DEFINE_GUID(DDVPTYPE_E_HREFL_VREFL, 0xe09c77e0, 0xda60, 0x11cf, 0x9b, 0x06, 0x00, 0xa0, 0xc9, 0x03, 0xa3, 0xb8);
DEFINE_GUID(DDVPTYPE_CCIR656, 0xfca326a0, 0xda60, 0x11cf, 0x9b, 0x06, 0x00, 0xa0, 0xc9, 0x03, 0xa3, 0xb8);
DEFINE_GUID(DDVPTYPE_BROOKTREE, 0x1352a560, 0xda61, 0x11cf, 0x9b, 0x06, 0x00, 0xa0, 0xc9, 0x03, 0xa3, 0xb8);
DEFINE_GUID(DDVPTYPE_PHILIPS, 0x332cf160, 0xda61, 0x11cf, 0x9b, 0x06, 0x00, 0xa0, 0xc9, 0x03, 0xa3, 0xb8);
#if _WIN32_WINNT >= 0x0501
DEFINE_GUID(IID_IDirectDrawVideoPortNotify, 0xa655fb94, 0x0589, 0x4e57, 0xb3, 0x33, 0x56, 0x7a, 0x89, 0x46, 0x8c, 0x88);
#endif
#define DDVPD_WIDTH 0x01
#define DDVPD_HEIGHT 0x02
#define DDVPD_ID 0x04
#define DDVPD_CAPS 0x08
#define DDVPD_FX 0x10
#define DDVPD_AUTOFLIP 0x20
#define DDVPD_ALIGN 0x40
#define DDVPD_PREFERREDAUTOFLIP 0x80
#define DDVPD_FILTERQUALITY 0x0100
#define DDVPCONNECT_DOUBLECLOCK 0x01
#define DDVPCONNECT_VACT 0x02
#define DDVPCONNECT_INVERTPOLARITY 0x04
#define DDVPCONNECT_DISCARDSVREFDATA 0x08
#define DDVPCONNECT_HALFLINE 0x10
#define DDVPCONNECT_INTERLACED 0x20
#define DDVPCONNECT_SHAREEVEN 0x40
#define DDVPCONNECT_SHAREODD 0x80
#define DDVPCAPS_AUTOFLIP 0x01
#define DDVPCAPS_INTERLACED 0x02
#define DDVPCAPS_NONINTERLACED 0x04
#define DDVPCAPS_READBACKFIELD 0x08
#define DDVPCAPS_READBACKLINE 0x10
#define DDVPCAPS_SHAREABLE 0x20
#define DDVPCAPS_SKIPEVENFIELDS 0x40
#define DDVPCAPS_SKIPODDFIELDS 0x80
#define DDVPCAPS_SYNCMASTER 0x0100
#define DDVPCAPS_VBISURFACE 0x0200
#define DDVPCAPS_COLORCONTROL 0x0400
#define DDVPCAPS_OVERSAMPLEDVBI 0x0800
#define DDVPCAPS_SYSTEMMEMORY 0x1000
#define DDVPCAPS_VBIANDVIDEOINDEPENDENT 0x2000
#define DDVPCAPS_HARDWAREDEINTERLACE 0x4000
#define DDVPFX_CROPTOPDATA 0x01
#define DDVPFX_CROPX 0x02
#define DDVPFX_CROPY 0x04
#define DDVPFX_INTERLEAVE 0x08
#define DDVPFX_MIRRORLEFTRIGHT 0x10
#define DDVPFX_MIRRORUPDOWN 0x20
#define DDVPFX_PRESHRINKX 0x40
#define DDVPFX_PRESHRINKY 0x80
#define DDVPFX_PRESHRINKXB 0x0100
#define DDVPFX_PRESHRINKYB 0x0200
#define DDVPFX_PRESHRINKXS 0x0400
#define DDVPFX_PRESHRINKYS 0x0800
#define DDVPFX_PRESTRETCHX 0x1000
#define DDVPFX_PRESTRETCHY 0x2000
#define DDVPFX_PRESTRETCHXN 0x4000
#define DDVPFX_PRESTRETCHYN 0x8000
#define DDVPFX_VBICONVERT 0x00010000
#define DDVPFX_VBINOSCALE 0x00020000
#define DDVPFX_IGNOREVBIXCROP 0x00040000
#define DDVPFX_VBINOINTERLEAVE 0x00080000
#define DDVP_AUTOFLIP 0x01
#define DDVP_CONVERT 0x02
#define DDVP_CROP 0x04
#define DDVP_INTERLEAVE 0x08
#define DDVP_MIRRORLEFTRIGHT 0x10
#define DDVP_MIRRORUPDOWN 0x20
#define DDVP_PRESCALE 0x40
#define DDVP_SKIPEVENFIELDS 0x80
#define DDVP_SKIPODDFIELDS 0x0100
#define DDVP_SYNCMASTER 0x0200
#define DDVP_VBICONVERT 0x0400
#define DDVP_VBINOSCALE 0x0800
#define DDVP_OVERRIDEBOBWEAVE 0x1000
#define DDVP_IGNOREVBIXCROP 0x2000
#define DDVP_VBINOINTERLEAVE 0x4000
#define DDVP_HARDWAREDEINTERLACE 0x8000
#define DDVPFORMAT_VIDEO 0x01
#define DDVPFORMAT_VBI 0x02
#define DDVPTARGET_VIDEO 0x01
#define DDVPTARGET_VBI 0x02
#define DDVPWAIT_BEGIN 0x01
#define DDVPWAIT_END 0x02
#define DDVPWAIT_LINE 0x03
#define DDVPFLIP_VIDEO 0x01
#define DDVPFLIP_VBI 0x02
#define DDVPSQ_NOSIGNAL 0x01
#define DDVPSQ_SIGNALOK 0x02
#define DDVPB_VIDEOPORT 0x01
#define DDVPB_OVERLAY 0x02
#define DDVPB_TYPE 0x04
#define DDVPBCAPS_SOURCE 0x01
#define DDVPBCAPS_DESTINATION 0x02
#define DDVPCREATE_VBIONLY 0x01
#define DDVPCREATE_VIDEOONLY 0x02
#define DDVPSTATUS_VBIONLY 0x01
#define DDVPSTATUS_VIDEOONLY 0x02
struct IDirectDraw;
struct IDirectDrawSurface;
struct IDirectDrawPalette;
struct IDirectDrawClipper;
struct IDDVideoPortContainer;
typedef struct IDDVideoPortContainer *LPDDVIDEOPORTCONTAINER;
struct IDirectDrawVideoPort;
typedef struct IDirectDrawVideoPort *LPDIRECTDRAWVIDEOPORT;
#if _WIN32_WINNT >= 0x0501
struct IDirectDrawVideoPortNotify;
typedef struct IDirectDrawVideoPortNotify *LPDIRECTDRAWVIDEOPORTNOTIFY;
#endif
struct _DDVIDEOPORTCONNECT;
typedef struct _DDVIDEOPORTCONNECT *LPDDVIDEOPORTCONNECT;
struct _DDVIDEOPORTCAPS;
typedef struct _DDVIDEOPORTCAPS *LPDDVIDEOPORTCAPS;
struct _DDVIDEOPORTDESC;
typedef struct _DDVIDEOPORTDESC *LPDDVIDEOPORTDESC;
struct _DDVIDEOPORTINFO;
typedef struct _DDVIDEOPORTINFO *LPDDVIDEOPORTINFO;
struct _DDVIDEOPORTBANDWIDTH;
typedef struct _DDVIDEOPORTBANDWIDTH *LPDDVIDEOPORTBANDWIDTH;
struct _DDVIDEOPORTSTATUS;
typedef struct _DDVIDEOPORTSTATUS *LPDDVIDEOPORTSTATUS;
#if _WIN32_WINNT >= 0x0501
struct _DDVIDEOPORTNOTIFY;
typedef struct _DDVIDEOPORTNOTIFY *LPDDVIDEOPORTNOTIFY;
#endif
struct IDDVideoPortContainerVtbl;
typedef struct IDDVideoPortContainerVtbl DDVIDEOPORTCONTAINERCALLBACKS;
struct IDirectDrawVideoPortVtbl;
typedef struct IDirectDrawVideoPortVtbl DIRECTDRAWVIDEOPORTCALLBACKS;
#if _WIN32_WINNT >= 0x0501
struct IDirectDrawVideoPortNotifyVtbl;
typedef struct IDirectDrawVideoPortNotifyVtbl DIRECTDRAWVIDEOPORTNOTIFYCALLBACKS;
#endif
typedef HRESULT (*LPDDENUMVIDEOCALLBACK)(LPDDVIDEOPORTCAPS, LPVOID) __attribute__((stdcall));
struct IDDVideoPortContainer {
  struct IDDVideoPortContainerVtbl *lpVtbl;
};
typedef struct IDDVideoPortContainer IDDVideoPortContainer;
typedef struct IDDVideoPortContainerVtbl IDDVideoPortContainerVtbl;
struct IDDVideoPortContainerVtbl {
  HRESULT (*QueryInterface)(IDDVideoPortContainer *, const IID *const, LPVOID *) __attribute__((stdcall));
  ULONG (*AddRef)(IDDVideoPortContainer *) __attribute__((stdcall));
  ULONG (*Release)(IDDVideoPortContainer *) __attribute__((stdcall));
  HRESULT (*CreateVideoPort)(IDDVideoPortContainer *, DWORD, LPDDVIDEOPORTDESC, LPDIRECTDRAWVIDEOPORT *, IUnknown *) __attribute__((stdcall));
  HRESULT (*EnumVideoPorts)(IDDVideoPortContainer *, DWORD, LPDDVIDEOPORTCAPS, LPVOID, LPDDENUMVIDEOCALLBACK) __attribute__((stdcall));
  HRESULT (*GetVideoPortConnectInfo)(IDDVideoPortContainer *, DWORD, LPDWORD, LPDDVIDEOPORTCONNECT) __attribute__((stdcall));
  HRESULT (*QueryVideoPortStatus)(IDDVideoPortContainer *, DWORD, LPDDVIDEOPORTSTATUS) __attribute__((stdcall));
};
struct IDirectDrawVideoPort {
  struct IDirectDrawVideoPortVtbl *lpVtbl;
};
typedef struct IDirectDrawVideoPort IDirectDrawVideoPort;
typedef struct IDirectDrawVideoPortVtbl IDirectDrawVideoPortVtbl;
struct IDirectDrawVideoPortVtbl {
  HRESULT (*QueryInterface)(IDirectDrawVideoPort *, const IID *const, LPVOID *) __attribute__((stdcall));
  ULONG (*AddRef)(IDirectDrawVideoPort *) __attribute__((stdcall));
  ULONG (*Release)(IDirectDrawVideoPort *) __attribute__((stdcall));
  HRESULT (*Flip)(IDirectDrawVideoPort *, LPDIRECTDRAWSURFACE, DWORD) __attribute__((stdcall));
  HRESULT (*GetBandwidthInfo)(IDirectDrawVideoPort *, LPDDPIXELFORMAT, DWORD, DWORD, DWORD, LPDDVIDEOPORTBANDWIDTH) __attribute__((stdcall));
  HRESULT (*GetColorControls)(IDirectDrawVideoPort *, LPDDCOLORCONTROL) __attribute__((stdcall));
  HRESULT (*GetInputFormats)(IDirectDrawVideoPort *, LPDWORD, LPDDPIXELFORMAT, DWORD) __attribute__((stdcall));
  HRESULT (*GetOutputFormats)(IDirectDrawVideoPort *, LPDDPIXELFORMAT, LPDWORD, LPDDPIXELFORMAT, DWORD) __attribute__((stdcall));
  HRESULT (*GetFieldPolarity)(IDirectDrawVideoPort *, LPBOOL) __attribute__((stdcall));
  HRESULT (*GetVideoLine)(IDirectDrawVideoPort *, LPDWORD) __attribute__((stdcall));
  HRESULT (*GetVideoSignalStatus)(IDirectDrawVideoPort *, LPDWORD) __attribute__((stdcall));
  HRESULT (*SetColorControls)(IDirectDrawVideoPort *, LPDDCOLORCONTROL) __attribute__((stdcall));
  HRESULT (*SetTargetSurface)(IDirectDrawVideoPort *, LPDIRECTDRAWSURFACE, DWORD) __attribute__((stdcall));
  HRESULT (*StartVideo)(IDirectDrawVideoPort *, LPDDVIDEOPORTINFO) __attribute__((stdcall));
  HRESULT (*StopVideo)(IDirectDrawVideoPort *) __attribute__((stdcall));
  HRESULT (*UpdateVideo)(IDirectDrawVideoPort *, LPDDVIDEOPORTINFO) __attribute__((stdcall));
  HRESULT (*WaitForSync)(IDirectDrawVideoPort *, DWORD, DWORD, DWORD) __attribute__((stdcall));
};
#if _WIN32_WINNT >= 0x0501
struct IDirectDrawVideoPortNotify {
  struct IDirectDrawVideoPortNotifyVtbl *lpVtbl;
};
typedef struct IDirectDrawVideoPortNotify IDirectDrawVideoPortNotify;
typedef struct IDirectDrawVideoPortNotifyVtbl IDirectDrawVideoPortNotifyVtbl;
struct IDirectDrawVideoPortNotifyVtbl {
  HRESULT (*QueryInterface)(IDirectDrawVideoPortNotify *, const IID *const, LPVOID *) __attribute__((stdcall));
  ULONG (*AddRef)(IDirectDrawVideoPortNotify *) __attribute__((stdcall));
  ULONG (*Release)(IDirectDrawVideoPortNotify *) __attribute__((stdcall));
  HRESULT (*AcquireNotification)(IDirectDrawVideoPortNotify *, HANDLE *, LPDDVIDEOPORTNOTIFY) __attribute__((stdcall));
  HRESULT (*ReleaseNotification)(IDirectDrawVideoPortNotify *, HANDLE) __attribute__((stdcall));
};
#endif
struct _DDVIDEOPORTCONNECT {
  DWORD dwSize;
  DWORD dwPortWidth;
  GUID guidTypeID;
  DWORD dwFlags;
  ULONG_PTR dwReserved1;
};
typedef struct _DDVIDEOPORTCONNECT DDVIDEOPORTCONNECT;
struct _DDVIDEOPORTCAPS {
  DWORD dwSize;
  DWORD dwFlags;
  DWORD dwMaxWidth;
  DWORD dwMaxVBIWidth;
  DWORD dwMaxHeight;
  DWORD dwVideoPortID;
  DWORD dwCaps;
  DWORD dwFX;
  DWORD dwNumAutoFlipSurfaces;
  DWORD dwAlignVideoPortBoundary;
  DWORD dwAlignVideoPortPrescaleWidth;
  DWORD dwAlignVideoPortCropBoundary;
  DWORD dwAlignVideoPortCropWidth;
  DWORD dwPreshrinkXStep;
  DWORD dwPreshrinkYStep;
  DWORD dwNumVBIAutoFlipSurfaces;
  DWORD dwNumPreferredAutoflip;
  WORD wNumFilterTapsX;
  WORD wNumFilterTapsY;
};
typedef struct _DDVIDEOPORTCAPS DDVIDEOPORTCAPS;
struct _DDVIDEOPORTDESC {
  DWORD dwSize;
  DWORD dwFieldWidth;
  DWORD dwVBIWidth;
  DWORD dwFieldHeight;
  DWORD dwMicrosecondsPerField;
  DWORD dwMaxPixelsPerSecond;
  DWORD dwVideoPortID;
  DWORD dwReserved1;
  DDVIDEOPORTCONNECT VideoPortType;
  ULONG_PTR dwReserved2;
  ULONG_PTR dwReserved3;
};
typedef struct _DDVIDEOPORTDESC DDVIDEOPORTDESC;
struct _DDVIDEOPORTINFO {
  DWORD dwSize;
  DWORD dwOriginX;
  DWORD dwOriginY;
  DWORD dwVPFlags;
  RECT rCrop;
  DWORD dwPrescaleWidth;
  DWORD dwPrescaleHeight;
  LPDDPIXELFORMAT lpddpfInputFormat;
  LPDDPIXELFORMAT lpddpfVBIInputFormat;
  LPDDPIXELFORMAT lpddpfVBIOutputFormat;
  DWORD dwVBIHeight;
  ULONG_PTR dwReserved1;
  ULONG_PTR dwReserved2;
};
typedef struct _DDVIDEOPORTINFO DDVIDEOPORTINFO;
struct _DDVIDEOPORTBANDWIDTH {
  DWORD dwSize;
  DWORD dwCaps;
  DWORD dwOverlay;
  DWORD dwColorkey;
  DWORD dwYInterpolate;
  DWORD dwYInterpAndColorkey;
  ULONG_PTR dwReserved1;
  ULONG_PTR dwReserved2;
};
typedef struct _DDVIDEOPORTBANDWIDTH DDVIDEOPORTBANDWIDTH;
struct _DDVIDEOPORTSTATUS {
  DWORD dwSize;
  BOOL bInUse;
  DWORD dwFlags;
  DWORD dwReserved1;
  DDVIDEOPORTCONNECT VideoPortType;
  ULONG_PTR dwReserved2;
  ULONG_PTR dwReserved3;
};
typedef struct _DDVIDEOPORTSTATUS DDVIDEOPORTSTATUS;
#if _WIN32_WINNT >= 0x0501
struct _DDVIDEOPORTNOTIFY {
  LARGE_INTEGER ApproximateTimeStamp;
  LONG lField;
  UINT dwSurfaceIndex;
  LONG lDone;
};
typedef struct _DDVIDEOPORTNOTIFY DDVIDEOPORTNOTIFY;
#endif

#endif
