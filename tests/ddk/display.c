/* A display driver's entry (GDI's DDI, <winddi.h>) for Windows NT 4.0 and
   later: DrvEnableDriver hands GDI the functions it implements; the one
   it has, DrvBitBlt, passes the call on to GDI's own EngBitBlt.
   run-tests.sh builds it with nt4-ddk.cfg as a native DLL that imports
   win32k.sys and checks with check-imports.py --nt 4.0 that NT 4.0's
   win32k.sys has its imports; with -DNT5_ONLY it also calls EngAlphaBlend,
   which 2000 added, and the check for 4.0 must fail.

   It also has a DirectDraw and Direct3D HAL (<ddrawint.h>,
   <d3dnthal.h>): DrvGetDirectDrawInfo describes it in DD_HALINFO, with
   Direct3D's global data, and DrvEnableDirectDraw hands DirectDraw its
   one callback, WaitForVerticalBlank. */
#include <windef.h>
#include <wingdi.h>
#include <winddi.h>

static BOOL APIENTRY MyBitBlt(SURFOBJ *dst, SURFOBJ *src, SURFOBJ *mask,
                              CLIPOBJ *clip, XLATEOBJ *xlate, RECTL *rdst,
                              POINTL *psrc, POINTL *pmask, BRUSHOBJ *brush,
                              POINTL *pbrush, ROP4 rop) {
#ifdef NT5_ONLY
  if (rop == 0xCCCC && src && src->iBitmapFormat == BMF_32BPP) {
    BLENDOBJ blend = {{AC_SRC_OVER, 0, 255, 0}};
    RECTL rsrc = {psrc->x, psrc->y, psrc->x + rdst->right - rdst->left,
                  psrc->y + rdst->bottom - rdst->top};
    return EngAlphaBlend(dst, src, clip, xlate, rdst, &rsrc, &blend);
  }
#endif
  return EngBitBlt(dst, src, mask, clip, xlate, rdst, psrc, pmask, brush,
                   pbrush, rop);
}

static DWORD APIENTRY WaitVBlank(PDD_WAITFORVERTICALBLANKDATA w) {
  w->bIsInVB = FALSE;
  w->ddRVal = DD_OK;
  return DDHAL_DRIVER_HANDLED;
}

static D3DNTHAL_GLOBALDRIVERDATA d3d;
static D3DNTHAL_CALLBACKS d3dcb;

static BOOL APIENTRY GetDirectDrawInfo(DHPDEV pdev, DD_HALINFO *hal,
                                       DWORD *heaps, VIDEOMEMORY *vm,
                                       DWORD *fourccs, DWORD *fourcc) {
  (void)pdev;
  (void)vm;
  (void)fourcc;
  hal->dwSize = sizeof *hal;
  hal->ddCaps.dwSize = sizeof hal->ddCaps;
  hal->ddCaps.dwCaps = DDCAPS_BLT;
  hal->ddCaps.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE;
  d3d.dwSize = sizeof d3d;
  d3d.hwCaps.dwSize = sizeof d3d.hwCaps;
  d3dcb.dwSize = sizeof d3dcb;
  hal->lpD3DGlobalDriverData = &d3d;
  hal->lpD3DHALCallbacks = &d3dcb;
  *heaps = 0;
  *fourccs = 0;
  return TRUE;
}

static BOOL APIENTRY EnableDirectDraw(DHPDEV pdev, DD_CALLBACKS *cb,
                                      DD_SURFACECALLBACKS *scb,
                                      DD_PALETTECALLBACKS *pcb) {
  (void)pdev;
  cb->dwSize = sizeof *cb;
  cb->dwFlags = DDHAL_CB32_WAITFORVERTICALBLANK;
  cb->WaitForVerticalBlank = WaitVBlank;
  scb->dwSize = sizeof *scb;
  scb->dwFlags = 0;
  pcb->dwSize = sizeof *pcb;
  pcb->dwFlags = 0;
  return TRUE;
}

static void APIENTRY DisableDirectDraw(DHPDEV pdev) { (void)pdev; }

static DRVFN functions[] = {
    {INDEX_DrvBitBlt, (PFN)MyBitBlt},
    {INDEX_DrvGetDirectDrawInfo, (PFN)GetDirectDrawInfo},
    {INDEX_DrvEnableDirectDraw, (PFN)EnableDirectDraw},
    {INDEX_DrvDisableDirectDraw, (PFN)(void (*)(void))DisableDirectDraw}};

BOOL APIENTRY DrvEnableDriver(ULONG version, ULONG size, DRVENABLEDATA *ded);
BOOL APIENTRY DrvEnableDriver(ULONG version, ULONG size, DRVENABLEDATA *ded) {
  if (size < sizeof *ded)
    return FALSE;
  ded->iDriverVersion = version < DDI_DRIVER_VERSION_NT4 ? version
                                                         : DDI_DRIVER_VERSION_NT4;
  ded->c = sizeof functions / sizeof functions[0];
  ded->pdrvfn = functions;
  return TRUE;
}
