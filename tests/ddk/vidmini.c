/* A video miniport (VIDEOPRT.SYS, <video.h>) with kernel-mode DirectDraw
   (<dxmini.h>): its HwQueryInterface hands out the DXAPI interface for
   GUID_DxApi, through which DirectDraw's kernel half (DXAPI.SYS) flips
   overlays and asks about the interrupts. This one has only the
   overlay flip and the interrupt query. run-tests.sh builds it with
   nt4-ddk.cfg and -lvideoprt and checks its imports for NT 4.0 and
   later (check-imports.py --nt 4.0); VIDEOPRT.SYS calls
   HwQueryInterface from Windows 2000 on. */
#include <initguid.h>
#include <ntdef.h>
#include <dderror.h>
#include <devioctl.h>
#include <miniport.h>
#include <ntddvdeo.h>
#include <video.h>
#include <dxmini.h>

typedef struct {
  ULONG flips;
} EXT;

static DWORD NTAPI GetIrqInfo(PVOID ext, PVOID in, PDDGETIRQINFO out) {
  (void)ext;
  (void)in;
  out->dwFlags = 0; /* no interrupts */
  return DX_OK;
}

static DWORD NTAPI FlipOverlay(PVOID ext, PDDFLIPOVERLAYINFO in, PVOID out) {
  (void)out;
  if (!in->lpTargetSurface)
    return DXERR_GENERIC;
  ((EXT *)ext)->flips++;
  return DX_OK;
}

static VP_STATUS NTAPI QueryInterface(PVOID ext, PQUERY_INTERFACE q) {
  DXAPI_INTERFACE *dx;
  if (!IsEqualGUID(q->InterfaceType, &GUID_DxApi) ||
      q->Size < sizeof(DXAPI_INTERFACE))
    return ERROR_INVALID_PARAMETER;
  dx = (DXAPI_INTERFACE *)q->Interface;
  VideoPortZeroMemory(dx, sizeof *dx);
  dx->Size = sizeof *dx;
  dx->Version = 1;
  dx->Context = ext;
  dx->DxGetIrqInfo = GetIrqInfo;
  dx->DxFlipOverlay = FlipOverlay;
  return NO_ERROR;
}

static VP_STATUS NTAPI FindAdapter(PVOID ext, PVOID ctx, PWSTR args,
                                   PVIDEO_PORT_CONFIG_INFO info,
                                   PUCHAR again) {
  (void)ext;
  (void)ctx;
  (void)args;
  (void)info;
  *again = FALSE;
  return NO_ERROR;
}

static BOOLEAN NTAPI Initialize(PVOID ext) {
  (void)ext;
  return TRUE;
}

static BOOLEAN NTAPI StartIO(PVOID ext, PVIDEO_REQUEST_PACKET rp) {
  (void)ext;
  rp->StatusBlock->Status = ERROR_INVALID_FUNCTION;
  return TRUE;
}

ULONG NTAPI DriverEntry(PVOID ctx1, PVOID ctx2) {
  VIDEO_HW_INITIALIZATION_DATA init;
  VideoPortZeroMemory(&init, sizeof init);
  init.HwInitDataSize = sizeof init;
  init.HwFindAdapter = FindAdapter;
  init.HwInitialize = Initialize;
  init.HwStartIO = StartIO;
  init.HwQueryInterface = QueryInterface;
  init.HwDeviceExtensionSize = sizeof(EXT);
  return VideoPortInitialize(ctx1, ctx2, &init, NULL);
}
