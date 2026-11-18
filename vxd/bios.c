/* A dynamic VxD that calls the BIOS with nested execution (vxd_exec_int)
   when a program calls it, and uses the debug output macros. For
   DeviceIoControl code 1 it runs INT 12h (the base memory size, in KB)
   and INT 1Ah function 0 (the tick count) in the VM, and answers

     bios mem 640 ticks 1

   (mem: what INT 12h returned in AX, 639 or 640 on most machines;
   ticks: 1 if the tick count was not 0). client.c calls it (CLIENT.EXE
   BIOS.VXD). Built with -DVXD_DEBUG, it also prints what it does to the
   debugger with Trace_Out and Debug_Printf, and checks with VXD_ASSERT;
   without it, they are not compiled. */
#include <vxd.h>

VXD_DEVICE(BIOS, UNDEFINED_DEVICE_ID, UNDEFINED_INIT_ORDER, 1, 0);

static char *put(char *o, const char *s, DWORD n) {
  char d[12];
  int k = 0;
  while (*s)
    *o++ = *s++;
  do {
    d[k++] = (char)('0' + (int)(n % 10));
    n /= 10;
  } while (n);
  while (k)
    *o++ = d[--k];
  return o;
}

static DWORD ioctl(struct DIOCParams *p) {
  struct vxd_int_regs io = {0};
  char *out = (char *)p->lpvOutBuffer, *o = out;
  DWORD mem, ticks;
  if (p->dwIoControlCode == DIOC_OPEN ||
      p->dwIoControlCode == DIOC_CLOSEHANDLE)
    return 0;
  if (p->dwIoControlCode != 1 || p->cbOutBuffer < 40)
    return 50; /* ERROR_NOT_SUPPORTED */
  Trace_Out("BIOS: INT 12h");
  vxd_exec_int(0x12, &io);
  mem = io.eax & 0xFFFF;
  VXD_ASSERT(mem > 0 && mem <= 640);
  io = (struct vxd_int_regs){0};
  vxd_exec_int(0x1A, &io); /* AH = 0: CX:DX = ticks since midnight */
  ticks = (io.ecx & 0xFFFF) << 16 | (io.edx & 0xFFFF);
  Debug_Printf("BIOS: %d KB, %d ticks\r\n", mem, ticks);
  o = put(o, "bios mem ", mem);
  o = put(o, " ticks ", ticks != 0);
  *o++ = 0;
  if (p->lpcbBytesReturned)
    *(DWORD *)p->lpcbBytesReturned = (DWORD)(o - out);
  return 0;
}

int vxd_control(DWORD msg, struct vxd_regs *r) {
  if (msg == W32_DEVICEIOCONTROL)
    r->eax = ioctl((struct DIOCParams *)r->esi);
  return 0;
}
