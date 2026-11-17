/* A dynamic VxD written the way the Windows 9x DDKs' samples are, with
   Microsoft's headers: built by run-tests.sh with each ms-*-vxd.cfg that
   import-ms-sdk.py made (kits stay on your machine), and linked with
   vxd-link.py --kit. Its init code (#pragma VxD_INIT_CODE_SEG) allocates
   a heap block through VXDWRAPS.CLB's _HeapAllocate; client.c calls it
   with DeviceIoControl code 1 and it answers, from pageable code, "ddk
   heap 1 vmm 4.10 vm 1" (on Windows 98; vm is 1 if Get_Sys_VM_Handle
   returned the current VM's handle, as it does in a Win32 program). */
#define WANTVXDWRAPS
#include <basedef.h>
#include <vmm.h>
#include <vxdwraps.h>
#include <vwin32.h>
#include <vxd.h>

#pragma VxD_LOCKED_DATA_SEG
VXD_DEVICE(MSDDK, UNDEFINED_DEVICE_ID, UNDEFINED_INIT_ORDER, 1, 0);
static PVOID block;
static DWORD sys_is_cur;

#pragma VxD_INIT_CODE_SEG
#undef CURSEG
#define CURSEG() ICODE
static DWORD init(void) {
  block = _HeapAllocate(32, HEAPZEROINIT);
  return block ? 0 : 1;
}

#pragma VxD_PAGEABLE_CODE_SEG
#undef CURSEG
#define CURSEG() PCODE
static char *put_dec(char *o, DWORD q) {
  char d[12];
  int k = 0;
  do {
    d[k++] = (char)('0' + q % 10);
    q /= 10;
  } while (q);
  while (k)
    *o++ = d[--k];
  return o;
}

static DWORD ioctl(PDIOCPARAMETERS p) {
  static const char heap[] = "ddk heap ", vmm[] = " vmm ", vm[] = " vm ";
  char *o = (char *)p->lpvOutBuffer, *out = o;
  WORD v = Get_VMM_Version();
  const char *s;
  if (p->dwIoControlCode == DIOC_OPEN || p->dwIoControlCode == DIOC_CLOSEHANDLE)
    return 0;
  if (p->dwIoControlCode != 1 || p->cbOutBuffer < 40)
    return 50;
  sys_is_cur = Get_Sys_VM_Handle() == Get_Cur_VM_Handle();
  for (s = heap; *s;)
    *o++ = *s++;
  o = put_dec(o, block != 0);
  for (s = vmm; *s;)
    *o++ = *s++;
  o = put_dec(o, v >> 8);
  *o++ = '.';
  o = put_dec(o, v & 0xFF);
  for (s = vm; *s;)
    *o++ = *s++;
  o = put_dec(o, sys_is_cur);
  *o++ = 0;
  if (p->lpcbBytesReturned)
    *(DWORD *)p->lpcbBytesReturned = (DWORD)(o - out);
  return 0;
}

#pragma VxD_LOCKED_CODE_SEG
#undef CURSEG
#define CURSEG() LCODE
int vxd_control(DWORD msg, struct vxd_regs *r) {
  switch (msg) {
  case SYS_DYNAMIC_DEVICE_INIT:
    return init();
  case SYS_DYNAMIC_DEVICE_EXIT:
    if (block)
      _HeapFree(block, 0);
    return 0;
  case W32_DEVICEIOCONTROL:
    r->eax = ioctl((PDIOCPARAMETERS)r->esi);
    return 0;
  }
  return 0;
}
