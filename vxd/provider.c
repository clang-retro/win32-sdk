/* A dynamic VxD with every way in that VXD_DEVICE_EX gives, built by
   run-tests.sh: device ID 0x3E5A (not one Microsoft assigned), a V86 and
   a PM API entry point, three services and two Win32 services.
     service 0  PROVIDE_Get_Version  register: EAX = 0x0100
     service 1  _PROVIDE_Add         C: returns a + b
     service 2  PROVIDE_Double       register: EAX = EAX * 2, carry set
                                     if it overflows
     Win32 0    w32_probe, 3 parameters: records the registers and
                stack it was called with, which DeviceIoControl 2
                returns, and returns 0x600D in the caller's EAX
     Win32 1    w32_add (VXD_WIN32_SERVICE), 2 parameters: a + b
   The V86 and PM entry points return AX = 0x5A5A + AX and set the
   caller's carry flag if BX is not 0x1234.

   client.c calls it with DeviceIoControl 1, and it calls its own services
   the way another VxD would (int 20h and the service number), answering

     provide ver 256 add 42 double 84 ovf 1

   apitest.c (a DOS program) calls the V86 entry point and w32svc.c (a
   Win32 one) the Win32 services. */
#include <vxd.h>

#define PROVIDE_ID 0x3E5A

int get_version(struct vxd_regs *r);
int dbl(struct vxd_regs *r);
int api(struct vxd_regs *r);
DWORD _PROVIDE_Add(DWORD a, DWORD b);
void w32_probe(void);
DWORD w32_add(struct vxd_client_regs *c, DWORD vm, DWORD a, DWORD b);

VXD_CALLBACK(PROVIDE_Get_Version, get_version);
VXD_CALLBACK(PROVIDE_Double, dbl);
VXD_CALLBACK(PROVIDE_API, api);
VXD_SERVICES(PROVIDE, PROVIDE_Get_Version, _PROVIDE_Add, PROVIDE_Double);
VXD_WIN32_SERVICE(PROVIDE_W32_Add, w32_add, 2);
VXD_WIN32_SERVICES(PROVIDE, VXD_WIN32(w32_probe, 3),
                   VXD_WIN32(PROVIDE_W32_Add, 2));
VXD_DEVICE_EX(PROVIDE, PROVIDE_ID, UNDEFINED_INIT_ORDER, 1, 0, PROVIDE_API,
              PROVIDE_API, PROVIDE_Services);

int get_version(struct vxd_regs *r) {
  r->eax = 0x0100;
  return 0;
}

DWORD _PROVIDE_Add(DWORD a, DWORD b) { return a + b; }

int dbl(struct vxd_regs *r) {
  int ovf = r->eax > 0x7FFFFFFFUL;
  r->eax *= 2;
  return ovf;
}

int api(struct vxd_regs *r) {
  struct vxd_client_regs *c = (struct vxd_client_regs *)r->ebp;
  c->eax = (c->eax & 0xFFFF0000UL) | ((0x5A5A + c->eax) & 0xFFFF);
  if ((c->ebx & 0xFFFF) == 0x1234)
    c->eflags &= ~VXD_CLIENT_CARRY;
  else
    c->eflags |= VXD_CLIENT_CARRY;
  return 0;
}

DWORD w32_add(struct vxd_client_regs *c, DWORD vm, DWORD a, DWORD b) {
  (void)c;
  (void)vm;
  return a + b;
}

/* What the Win32 service saw: the registers and the first dwords on its
   stack (the caller's registers, the VM, then its parameters), filled in
   by w32_probe, which returns 0x600D in the caller's EAX the way
   VXD_WIN32_SERVICE does; then what _Register_Win32_Services
   returned and the DDB's Win32 service table after it. */
DWORD probe[14];
__asm__(".text\n.globl _w32_probe\n_w32_probe:\n"
        "  movl %eax, _probe\n  movl %ebx, _probe+4\n  movl %ecx, _probe+8\n"
        "  movl %edx, _probe+12\n  movl %esi, _probe+16\n"
        "  movl %edi, _probe+20\n  movl %ebp, _probe+24\n"
        "  movl %esp, _probe+28\n"
        "  movl 4(%esp), %eax\n  movl %eax, _probe+32\n"
        "  movl 8(%esp), %eax\n  movl %eax, _probe+36\n"
        "  movl 12(%esp), %eax\n  movl %eax, _probe+40\n"
        "  movl 16(%esp), %eax\n  movl %eax, _probe+44\n"
        "  movl 4(%esp), %eax\n  movl $0x600D, 28(%eax)\n  ret $20\n");

/* The services as another VxD calls them. */
VXD_C_SERVICE(Call_PROVIDE_Add, PROVIDE_ID, 1);
DWORD Call_PROVIDE_Add(DWORD a, DWORD b);

static char *put_dec(char *out, DWORD q) {
  char d[12];
  int k = 0;
  do {
    d[k++] = (char)('0' + (int)(q % 10));
    q /= 10;
  } while (q);
  while (k)
    *out++ = d[--k];
  return out;
}

static char *put(char *o, const char *s, DWORD n) {
  while (*s)
    *o++ = *s++;
  return put_dec(o, n);
}

static DWORD ioctl(struct DIOCParams *p) {
  char *out = (char *)p->lpvOutBuffer, *o = out;
  struct vxd_regs r = {0};
  DWORD ver, dbl1, ovf;
  int i;
  if (p->dwIoControlCode == DIOC_OPEN || p->dwIoControlCode == DIOC_CLOSEHANDLE)
    return 0;
  if (p->dwIoControlCode == 2 && p->cbOutBuffer >= sizeof probe) {
    for (i = 0; i < 14; i++)
      ((DWORD *)out)[i] = probe[i];
    if (p->lpcbBytesReturned)
      *(DWORD *)p->lpcbBytesReturned = sizeof probe;
    return 0;
  }
  if (p->dwIoControlCode != 1 || p->cbOutBuffer < 80)
    return 50; /* ERROR_NOT_SUPPORTED */
  VXD_CALL_ID(&r, VXD_SERVICE(PROVIDE_ID, 0));
  ver = r.eax;
  r.eax = 42;
  VXD_CALL_ID(&r, VXD_SERVICE(PROVIDE_ID, 2));
  dbl1 = r.eax;
  r.eax = 0x80000000UL;
  ovf = VXD_CALL_ID(&r, VXD_SERVICE(PROVIDE_ID, 2));
  o = put(o, "provide ver ", ver);
  o = put(o, " add ", Call_PROVIDE_Add(40, 2));
  o = put(o, " double ", dbl1);
  o = put(o, " ovf ", ovf);
  *o++ = 0;
  if (p->lpcbBytesReturned)
    *(DWORD *)p->lpcbBytesReturned = (DWORD)(o - out);
  return 0;
}

int vxd_control(DWORD msg, struct vxd_regs *r) {
  if (msg == SYS_DYNAMIC_DEVICE_INIT) {
    probe[12] = VXD_REGISTER_WIN32_SERVICES(PROVIDE);
    probe[13] = PROVIDE_DDB.DDB_Win32_Service_Table;
  }
  if (msg == W32_DEVICEIOCONTROL)
    r->eax = ioctl((struct DIOCParams *)r->esi);
  return 0;
}
