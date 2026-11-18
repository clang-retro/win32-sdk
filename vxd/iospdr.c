/* An IOS port driver (.PDR) for Windows 95, 98 and ME, with no devices.
   IOS loads each .PDR in SYSTEM\IOSUBSYS at boot. This one registers
   with IOS_Register when it loads, as a miscellaneous port driver.
   Then IOS calls its async event routine, and the driver does three
   things. On AEP_INITIALIZE it allocates a block through the ILB's
   service routine (ISP_ALLOC_MEM). On AEP_BOOT_COMPLETE it frees the
   block and records what it saw in HKLM\Software\clang-retro\pdr, value
   "aep":

     init 1 mem 1 boot 1 lgn 19

   Here init, mem and boot count the events and allocations, and lgn is
   the load group IOS gave the AEP_INITIALIZE. Last, it returns
   AEP_FAILURE, since it has no devices, so IOS unloads it. run-tests.sh
   builds it at each level of optimization. */
#include <vxd.h>
#include <aep.h>
#include <drp.h>
#include <ilb.h>
#include <isp.h>

VXD_DEVICE(IOSPDR, UNDEFINED_DEVICE_ID, UNDEFINED_INIT_ORDER, 1, 0);

static ILB ilb;
static DWORD inits, mems, boots, lgn;
static ULONG block;

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

static void record(void) {
  char text[64], *o = text;
  DWORD key;
  o = put(o, "init ", inits);
  o = put(o, " mem ", mems);
  o = put(o, " boot ", boots);
  o = put(o, " lgn ", lgn);
  *o = 0;
  if (_RegCreateKey(HKEY_LOCAL_MACHINE, "Software\\clang-retro\\pdr",
                    &key) == 0) {
    _RegSetValueEx(key, "aep", 0, REG_SZ, text, (DWORD)(o - text) + 1);
    _RegCloseKey(key);
  }
}

/* IOS calls it with the AEP, cdecl, as it calls each driver's. */
static void aer(AEP *aep) {
  switch (aep->AEP_func) {
  case AEP_INITIALIZE: {
    ISP_mem_alloc isp = {{ISP_ALLOC_MEM, 0}, 64, 0, 0, 0, {0}};
    inits++;
    lgn = aep->AEP_lgn;
    ilb.ILB_service_rtn(&isp.ISP_mem_a_hdr);
    if (isp.ISP_mem_a_hdr.ISP_result == 0 && isp.ISP_mem_ptr) {
      block = isp.ISP_mem_ptr;
      ((char *)block)[0] = 'X';
      mems++;
    }
    aep->AEP_result = AEP_SUCCESS;
    break;
  }
  case AEP_BOOT_COMPLETE:
    boots++;
    if (block) {
      ISP_mem_dealloc isp = {{ISP_DEALLOC_MEM, 0}, block};
      ilb.ILB_service_rtn(&isp.ISP_mem_da_hdr);
      block = 0;
    }
    record();
    aep->AEP_result = (USHORT)AEP_FAILURE;  /* no devices: unload */
    break;
  default:
    aep->AEP_result = AEP_SUCCESS;
  }
}

static DRP drp = {
    .DRP_eyecatch_str = {'X', 'X', 'X', 'X', 'X', 'X', 'X', 'X'},
    .DRP_LGN = DRP_MISC_PD,
    .DRP_aer = (PVOID)aer,
    .DRP_ilb = &ilb,
    .DRP_ascii_name = "CLANG IOSPDR",
};

/* IOS_Register takes the DRP on the stack and sets DRP_reg_result. */
static void ios_register(DRP *d) {
  __asm__ volatile("pushl %0\n\t"
                   "int $0x20\n\t"
                   ".long %c1\n\t"
                   "addl $4, %%esp"
                   :
                   : "r"(d), "i"(VXDSVC_IOS_Register)
                   : "eax", "ecx", "edx", "memory", "cc");
}

int vxd_control(DWORD msg, struct vxd_regs *r) {
  (void)r;
  if (msg == SYS_DYNAMIC_DEVICE_INIT) {
    ios_register(&drp);
    /* Fail to load if IOS turned it down (it will not call aer). */
    return drp.DRP_reg_result == DRP_ABORT ||
           drp.DRP_reg_result == DRP_INVALID_LAYER;
  }
  return 0;
}
