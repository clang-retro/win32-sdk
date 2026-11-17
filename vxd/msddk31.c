/* A Windows 3.1 VxD with the C headers import-ms-sdk.py makes from the
   3.1 DDK's MASM include files (ms-w31-vxd.cfg; the kit stays on your
   machine): built by run-tests.sh and linked with vxd-link.py --win31.
   Its PM API entry point (an entry point needs a device ID: 0x3E5B, not
   one Microsoft assigned) uses vmm.h's Client_Reg_Struc and equates, and
   the layouts the headers give are checked against vxd.h's. */
#include <stddef.h>
#include <vxd.h>
#include <vmm.h>
#include <vpicd.h>
#include <shell.h>

_Static_assert(offsetof(struct Client_Reg_Struc, Client_EAX) ==
                   offsetof(struct vxd_client_regs, eax),
               "Client_EAX");
_Static_assert(offsetof(struct Client_Reg_Struc, Client_EFlags) ==
                   offsetof(struct vxd_client_regs, eflags),
               "Client_EFlags");
_Static_assert(offsetof(struct VxD_Desc_Block, DDB_Control_Proc) ==
                   offsetof(struct vxd_ddb, DDB_Control_Proc),
               "DDB_Control_Proc");
_Static_assert(offsetof(struct VxD_Desc_Block, DDB_PM_API_Proc) ==
                   offsetof(struct vxd_ddb, DDB_PM_API_Proc),
               "DDB_PM_API_Proc");
_Static_assert(sizeof(struct Pushad_Struc) == 32, "Pushad_Struc");
_Static_assert(Sys_Critical_Init == SYS_CRITICAL_INIT, "Sys_Critical_Init");
_Static_assert(Device_Init == DEVICE_INIT, "Device_Init");
_Static_assert(VPICD_Device_ID == 3, "VPICD_Device_ID");

int api(struct vxd_regs *r);
VXD_CALLBACK(MSDDK31_API, api);
VXD_DEVICE_EX(MSDDK31, 0x3E5B, Undefined_Init_Order, 3, 10, 0,
              MSDDK31_API, 0);

/* AX = the VMM's device ID and the carry flag clear. */
int api(struct vxd_regs *r) {
  struct Client_Reg_Struc *c = (struct Client_Reg_Struc *)r->ebp;
  c->Client_EAX = (c->Client_EAX & 0xFFFF0000UL) | VMM_Device_ID;
  c->Client_EFlags &= ~(DWORD)CF_Mask;
  return 0;
}

int vxd_control(DWORD msg, struct vxd_regs *r) {
  (void)msg;
  (void)r;
  return 0;
}
