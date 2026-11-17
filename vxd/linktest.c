/* Links a VxD that calls a service of each convention through vxdsvc.h
   and libvxd.a (run-tests.sh builds it with VXD_WINVER for ME and does
   not load it). */
#include <vxd.h>

VXD_DEVICE(LINKTEST, UNDEFINED_DEVICE_ID, UNDEFINED_INIT_ORDER, 1, 0);

VXD_PAGEABLE_CODE static DWORD calls(void) {
  struct vxd_regs r = {0};
  DWORD n = 0;
  n += VxDCall(Get_VMM_Version, &r);         /* register */
  n += VxDCall(VWIN32_GetCurrentProcessHandle, &r);
  n += (DWORD)_HeapAllocate(16, 0);          /* C, declared in vxd.h */
  n += (DWORD)_GetVxDName(0, 0);             /* C, typed in vxdsvc.h */
  n += _Allocate_Device_CB_Area(4, 0);       /* the same */
  n += _NtKernClose(0);                      /* stdcall */
  n += ObsoleteKfRaiseIrql(0);               /* fastcall */
  return n;
}

int vxd_control(DWORD msg, struct vxd_regs *r) {
  if (msg == W32_DEVICEIOCONTROL)
    r->eax = calls();
  return 0;
}
