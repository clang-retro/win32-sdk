/* A dynamic VxD that hooks two VMM services while it is loaded: the
   register service Get_System_Time with VXD_HOOK_REG, and the C service
   _HeapGetSize with VXD_HOOK_C. Both hooks count their calls and go on to
   the procedure they replaced. It hooks them at Sys_Dynamic_Device_Init
   and unhooks them at Sys_Dynamic_Device_Exit. For DeviceIoControl code 1
   it calls both services itself (through the VMM, so through its hooks)
   and answers

     hook time 1 heap 1 size 1

   (time and heap: 1 if the hook counted the call; size: 1 if the hooked
   _HeapGetSize still gave the right size, so the call went on to the
   VMM). client.c calls it (CLIENT.EXE HOOK.VXD). */
#include <vxd.h>

VXD_DEVICE(HOOK, UNDEFINED_DEVICE_ID, UNDEFINED_INIT_ORDER, 1, 0);

/* The procedures the hooks replaced; the VMM may change them while the
   hooks are in place, when another hook in the chain goes away. */
DWORD prev_time, prev_size;
static volatile DWORD time_calls, size_calls;
static int hooked;

VXD_HOOK_REG(time_hook, time_fn, prev_time);
int time_fn(struct vxd_regs *r) {
  (void)r;
  time_calls++;
  return VXD_HOOK_PASS;
}

DWORD size_fn(void *block, DWORD flags);
VXD_HOOK_C(size_hook, size_fn, prev_size);
DWORD size_fn(void *block, DWORD flags) {
  size_calls++;
  return ((DWORD(*)(void *, DWORD))prev_size)(block, flags);
}

static char *put(char *o, const char *s, DWORD n) {
  while (*s)
    *o++ = *s++;
  *o++ = (char)('0' + (n != 0));
  return o;
}

static DWORD ioctl(struct DIOCParams *p) {
  char *out = (char *)p->lpvOutBuffer, *o = out;
  DWORD t0, s0, size;
  void *block;
  if (p->dwIoControlCode == DIOC_OPEN ||
      p->dwIoControlCode == DIOC_CLOSEHANDLE)
    return 0;
  if (p->dwIoControlCode != 1 || p->cbOutBuffer < 32)
    return 50; /* ERROR_NOT_SUPPORTED */
  t0 = time_calls;
  (void)VMMCALL_Get_System_Time();
  t0 = time_calls - t0;
  block = _HeapAllocate(100, 0);
  if (!block)
    return 8; /* ERROR_NOT_ENOUGH_MEMORY */
  s0 = size_calls;
  size = _HeapGetSize(block, 0);
  s0 = size_calls - s0;
  _HeapFree(block, 0);
  o = put(o, "hook time ", t0);
  o = put(o, " heap ", s0);
  o = put(o, " size ", size >= 100);
  *o++ = 0;
  if (p->lpcbBytesReturned)
    *(DWORD *)p->lpcbBytesReturned = (DWORD)(o - out);
  return 0;
}

int vxd_control(DWORD msg, struct vxd_regs *r) {
  switch (msg) {
  case SYS_DYNAMIC_DEVICE_INIT:
    if (vxd_hook_service(VXDSVC_Get_System_Time, time_hook, &prev_time))
      return 1;
    if (vxd_hook_service(VXDSVC__HeapGetSize, size_hook, &prev_size)) {
      vxd_unhook_service(VXDSVC_Get_System_Time, time_hook);
      return 1;
    }
    hooked = 1;
    return 0;
  case SYS_DYNAMIC_DEVICE_EXIT:
    if (hooked) {
      vxd_unhook_service(VXDSVC__HeapGetSize, size_hook);
      vxd_unhook_service(VXDSVC_Get_System_Time, time_hook);
      hooked = 0;
    }
    return 0;
  case W32_DEVICEIOCONTROL:
    r->eax = ioctl((struct DIOCParams *)r->esi);
    return 0;
  }
  return 0;
}
