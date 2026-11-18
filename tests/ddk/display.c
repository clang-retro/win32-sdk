/* A display driver's entry (GDI's DDI, <winddi.h>) for Windows NT 4.0 and
   later: DrvEnableDriver hands GDI the functions it implements; the one
   it has, DrvBitBlt, passes the call on to GDI's own EngBitBlt.
   run-tests.sh builds it with nt4-ddk.cfg as a native DLL that imports
   win32k.sys and checks with check-imports.py --nt 4.0 that NT 4.0's
   win32k.sys has its imports; with -DNT5_ONLY it also calls EngAlphaBlend,
   which 2000 added, and the check for 4.0 must fail. */
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

static DRVFN functions[] = {{INDEX_DrvBitBlt, (PFN)MyBitBlt}};

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
