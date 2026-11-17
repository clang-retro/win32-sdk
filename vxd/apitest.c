/* A DOS program, for a DOS box of Windows 95, 98 or ME: loads provider.c's
   VxD (PROVIDE.VXD next to this program, on drive A:) with VXDLDR's V86
   API, finds its V86 entry point with int 2Fh AX=1684h, calls it, and
   unloads it. Prints, and writes to A:\RESULT.TXT:

     v86 load 0 api 5a5b carry 0 bad 1 unload 0

   (load and unload: VXDLDR's error codes, 0 for none; api: AX after
   calling the entry point with AX=1 BX=1234h; bad: its carry flag when
   BX is wrong). Built with --config=dos.cfg -mcmodel=tiny. */
#include <stdio.h>

#define VXDLDR_ID 0x0027
#define PROVIDE_ID 0x3E5A

static unsigned short api[2]; /* offset, segment */

/* int 2Fh AX=1684h: the V86 API entry point of device ID, or 0:0. */
static int get_api(unsigned id) {
  unsigned short off, seg;
  __asm__ __volatile__("push %%es\n\txorw %%di, %%di\n\tmovw %%di, %%es\n\t"
                       "int $0x2f\n\tmovw %%es, %%ax\n\tpop %%es"
                       : "=D"(off), "=a"(seg)
                       : "a"(0x1684), "b"(id)
                       : "cx", "dx", "memory");
  api[0] = off;
  api[1] = seg;
  return off || seg;
}

/* Call the entry point with AX and BX (and DX); returns AX, and the carry
   flag in *carry. */
static unsigned call_api(unsigned ax, unsigned bx, unsigned dx, int *carry) {
  unsigned c;
  __asm__ __volatile__("push %%si\n\tpush %%di\n\tlcallw *%[api]\n\t"
                       "pop %%di\n\tpop %%si\n\tsbbw %%cx, %%cx"
                       : "+a"(ax), "+b"(bx), "+d"(dx), "=c"(c)
                       : [api] "m"(api)
                       : "memory", "cc");
  *carry = c;
  return ax;
}

int main(void) {
  static char file[] = "A:\\PROVIDE.VXD", name[] = "PROVIDE";
  unsigned load = 0xFFFF, unload = 0xFFFF, ax = 0;
  int carry = 0, api_carry = 0, bad = 0;
  FILE *f;
  if (get_api(VXDLDR_ID)) {
    load = call_api(1, 0, (unsigned)file, &carry); /* load device, DS:DX */
    if (!carry)
      load = 0;
    if (get_api(PROVIDE_ID)) {
      ax = call_api(1, 0x1234, 0, &api_carry);
      call_api(1, 0x4321, 0, &bad);
      bad = bad ? 1 : 0;
    }
    get_api(VXDLDR_ID);
    unload = call_api(2, 0xFFFF, (unsigned)name, &carry); /* by name */
    if (!carry)
      unload = 0;
  }
  printf("v86 load %x api %x carry %d bad %d unload %x\n", load, ax,
         api_carry, bad, unload);
  f = fopen("A:\\RESULT.TXT", "w");
  if (f) {
    fprintf(f, "v86 load %x api %x carry %d bad %d unload %x\n", load, ax,
            api_carry, bad, unload);
    fclose(f);
  }
  return 0;
}
