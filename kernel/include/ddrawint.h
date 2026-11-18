/* ddrawint.h: the DirectDraw driver types that <winddi.h> names, for
   display drivers that use GDI's DDI (DrvEnableDriver, the Eng*
   functions in win32k.sys) without a DirectDraw HAL. mingw-w64's
   <winddi.h> includes this header but mingw-w64 has none. <winddi.h>
   uses these types only through pointers (DrvGetDirectDrawInfo,
   DrvEnableDirectDraw, EngLockDirectDrawSurface ...), so they are
   declared and not defined: a driver that implements DirectDraw needs
   their members, from the DDK. */
#ifndef WDM_DDRAWINT_H
#define WDM_DDRAWINT_H

typedef struct _DD_CALLBACKS DD_CALLBACKS, *PDD_CALLBACKS;
typedef struct _DD_SURFACECALLBACKS DD_SURFACECALLBACKS,
    *PDD_SURFACECALLBACKS;
typedef struct _DD_PALETTECALLBACKS DD_PALETTECALLBACKS,
    *PDD_PALETTECALLBACKS;
typedef struct _DD_HALINFO DD_HALINFO, *PDD_HALINFO;
typedef struct _DD_DIRECTDRAW_GLOBAL DD_DIRECTDRAW_GLOBAL,
    *PDD_DIRECTDRAW_GLOBAL;
typedef struct _DD_DIRECTDRAW_LOCAL DD_DIRECTDRAW_LOCAL,
    *PDD_DIRECTDRAW_LOCAL;
typedef struct _DD_SURFACE_GLOBAL DD_SURFACE_GLOBAL, *PDD_SURFACE_GLOBAL;
typedef struct _DD_SURFACE_LOCAL DD_SURFACE_LOCAL, *PDD_SURFACE_LOCAL;
typedef struct _VIDEOMEMORY VIDEOMEMORY, *LPVIDEOMEMORY;

#endif
