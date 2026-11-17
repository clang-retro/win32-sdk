/* A dynamic VxD for Windows 95, 98 and ME, built by run-tests.sh.
   client.c loads it with CreateFile("\\\\.\\C:\\...\\HELLO.VXD") and calls
   it with DeviceIoControl: code 1 takes a DWORD n and answers
   "hello from clang vxd N VV", where N is n * 1000000007 / 7 (64-bit
   arithmetic, from compiler-rt's builtins) and VV the VMM's version from
   Get_VMM_Version. */
#include <vxd.h>

VXD_DEVICE(HELLO, UNDEFINED_DEVICE_ID, UNDEFINED_INIT_ORDER, 1, 0);

static DWORD opens;

static char *put_dec(char *out, unsigned long long q) {
  char d[24];
  int k = 0;
  do {
    d[k++] = (char)('0' + (int)(q % 10));
    q /= 10;
  } while (q);
  while (k)
    *out++ = d[--k];
  return out;
}

static DWORD ioctl(struct DIOCParams *p) {
  static const char msg[] = "hello from clang vxd ";
  char *out = (char *)p->lpvOutBuffer, *o = out;
  WORD v;
  unsigned i;
  if (p->dwIoControlCode == DIOC_OPEN) {
    opens++;
    return 0;
  }
  if (p->dwIoControlCode == DIOC_CLOSEHANDLE)
    return 0;
  if (p->dwIoControlCode != 1 || p->cbInBuffer < 4 || p->cbOutBuffer < 48)
    return 50; /* ERROR_NOT_SUPPORTED */
  for (i = 0; msg[i]; i++)
    *o++ = msg[i];
  o = put_dec(o, (unsigned long long)*(DWORD *)p->lpvInBuffer * 1000000007 / 7);
  *o++ = ' ';
  v = VMMCALL_Get_VMM_Version();
  o = put_dec(o, v >> 8);
  *o++ = '.';
  o = put_dec(o, v & 0xFF);
  *o++ = 0;
  if (p->lpcbBytesReturned)
    *(DWORD *)p->lpcbBytesReturned = (DWORD)(o - out);
  return 0;
}

int vxd_control(DWORD msg, struct vxd_regs *r) {
  switch (msg) {
  case SYS_DYNAMIC_DEVICE_INIT:
  case SYS_DYNAMIC_DEVICE_EXIT:
    return 0;
  case W32_DEVICEIOCONTROL:
    r->eax = ioctl((struct DIOCParams *)r->esi);
    return 0;
  default:
    return 0;
  }
}
