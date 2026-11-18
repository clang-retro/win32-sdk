/* A display mini-VDD for Windows 95, 98 and ME: the VxD half of a
   display driver, which the "main" VDD (VDD.VXD) calls to save and
   restore the adapter's state around MS-DOS sessions and mode changes.
   A display driver's INF names it (HKR,DEFAULT,minivdd,,MINIVDD.VXD) and
   the VDD loads it at boot, so it is a static VxD.

   At Sys_Critical_Init it asks the VDD for its dispatch table
   (VDD_Get_Mini_Dispatch_Table: ECX is the number of entries, EDI the
   table) and fills in the functions it handles, indexed by the function
   numbers of <minivdd.h>. The VDD calls each one like a register service:
   EBX is the VM, EBP its client registers; the carry flag clear means
   handled. Entries left 0 the VDD handles itself. This one has
   REGISTER_DISPLAY_DRIVER (the display driver's call when it starts, its
   arguments in the client registers), DISPLAY_DRIVER_DISABLING and
   GET_CHIP_ID (EAX: an ID that tells this adapter from others).

   run-tests.sh links it with vxd-link.py --static and checks it with
   vxd-check.py: the procedures in the table are VXD_CALLBACKs, in
   locked code. */
#include <vxd.h>
#include <minivdd.h>

VXD_DEVICE(MINIVDD, UNDEFINED_DEVICE_ID, UNDEFINED_INIT_ORDER, 1, 0);

/* What the VDD's calls brought, public so that a debugger (WDEB386,
   with the .SYM file) can show them. */
DWORD registered, disabled;
DWORD display_ds, display_si;

VXD_CALLBACK(register_display_driver, register_fn);
int register_fn(struct vxd_regs *r) {
  struct vxd_client_regs *c = (struct vxd_client_regs *)r->ebp;
  /* The display driver passes its own data in DS:SI. */
  display_ds = c->ds;
  display_si = c->esi & 0xFFFF;
  registered++;
  return 0;
}

VXD_CALLBACK(display_driver_disabling, disabling_fn);
int disabling_fn(struct vxd_regs *r) {
  (void)r;
  disabled++;
  return 0;
}

VXD_CALLBACK(get_chip_id, chip_id_fn);
int chip_id_fn(struct vxd_regs *r) {
  r->eax = 0x434C4E47; /* "CLNG" */
  return 0;
}

static VXD_INIT_CODE int fill_dispatch_table(void) {
  struct vxd_regs r = {0};
  DWORD *table;
  if (VxDCall(VDD_Get_Mini_Dispatch_Table, &r))
    return 1;
  /* A cursory version check, as the DDK's samples do. */
  if (r.ecx < NBR_MINI_VDD_FUNCTIONS_40)
    return 1;
  table = (DWORD *)r.edi;
  table[REGISTER_DISPLAY_DRIVER] = (DWORD)register_display_driver;
  table[DISPLAY_DRIVER_DISABLING] = (DWORD)display_driver_disabling;
  table[GET_CHIP_ID] = (DWORD)get_chip_id;
  return 0;
}

int vxd_control(DWORD msg, struct vxd_regs *r) {
  (void)r;
  if (msg == SYS_CRITICAL_INIT)
    return fill_dispatch_table();
  return 0;
}
