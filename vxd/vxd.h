/* vxd.h: virtual device drivers (VxDs) for Windows 3.1 (386 enhanced
   mode), 95, 98 and ME with clang, from clang-retro/win32-sdk.

   SPDX-License-Identifier: AGPL-3.0-only
   Copyright (C) clang-retro contributors.

   A VxD is a 32-bit flat ring-0 module in the LE format that the Virtual
   Machine Manager (VMM) loads. It exports one thing, its Device
   Descriptor Block (DDB), which names it and points at its control
   procedure, which the VMM calls with a system control message in EAX
   and its parameters in the other registers.

     #include <vxd.h>
     VXD_DEVICE(HELLO, UNDEFINED_DEVICE_ID, UNDEFINED_INIT_ORDER, 1, 0)
     int vxd_control(DWORD msg, struct vxd_regs *r) { ... return 0; }

   VXD_DEVICE defines HELLO_DDB and HELLO_Control, which saves the
   registers in a struct vxd_regs, calls vxd_control and loads them back:
   a control function changes r->eax and the others to return values. It
   returns 0 for success (the carry flag clear) and anything else for
   failure (carry set). Build with --config=win95-vxd.cfg -c and link with
   vxd-link.py (see the README, "VxDs").

   VMM and VxD services are called with "int 20h" followed by a dword
   that holds the device ID and the service number; the VMM patches each
   such call into a direct one the first time it runs. vxdsvc.h, which
   this includes, numbers the services of Windows 3.1 to ME; VxDCall
   calls the register ones, libvxd.a has the C ones (_HeapAllocate and
   the like), and VMMCALL_* below wrap a few. The numbers and names come
   from tools/vxd-services.txt; no file of Microsoft's DDKs is used.

   With the configs import-ms-sdk.py makes from Microsoft's VxD DDKs
   (ms-w95-vxd.cfg and the like, which define VXD_MS_DDK), this includes
   the DDK's basedef.h and vmm.h and leaves to them what they define: it
   adds VXD_DEVICE, VXD_CALLBACK, VXD_CALL and the VXD_*_CODE and _DATA
   kinds. */
#ifndef CLANG_RETRO_VXD_H
#define CLANG_RETRO_VXD_H

#if defined(VXD_MS_DDK) && !defined(_VMM_)
#include <basedef.h>
#include <vmm.h>
#endif

#ifndef __i386__
#error "VxDs are 32-bit x86 code: use --config=win95-vxd.cfg"
#endif

#if !defined(_WINDEF_) && !defined(_VMM_)
typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;
#endif

/* The VMM and the version numbers of the DDB and of Get_VMM_Version. */
#ifndef _VMM_
#define VMM_DEVICE_ID 0x0001
#define UNDEFINED_DEVICE_ID 0x0000
#define UNDEFINED_INIT_ORDER 0x80000000UL
#endif
#define VXD_DDK_VERSION_31 0x030A /* Windows 3.1 */
#define VXD_DDK_VERSION_40 0x0400 /* Windows 95, 98, ME */
#ifndef VXD_DDK_VERSION
#define VXD_DDK_VERSION VXD_DDK_VERSION_40
#endif

#ifndef _VMM_
/* System control messages (EAX in the control procedure). */
#define SYS_CRITICAL_INIT 0x0000
#define DEVICE_INIT 0x0001
#define INIT_COMPLETE 0x0002
#define SYS_VM_INIT 0x0003
#define SYS_VM_TERMINATE 0x0004
#define SYSTEM_EXIT 0x0005
#define SYS_CRITICAL_EXIT 0x0006
#define CREATE_VM 0x0007
#define VM_CRITICAL_INIT 0x0008
#define VM_INIT 0x0009
#define VM_TERMINATE 0x000A
#define VM_NOT_EXECUTEABLE 0x000B
#define DESTROY_VM 0x000C
#define VM_SUSPEND 0x000D
#define VM_RESUME 0x000E
#define SET_DEVICE_FOCUS 0x000F
#define BEGIN_MESSAGE_MODE 0x0010
#define END_MESSAGE_MODE 0x0011
#define REBOOT_PROCESSOR 0x0012
#define QUERY_DESTROY 0x0013
#define DEBUG_QUERY 0x0014
#define BEGIN_PM_APP 0x0015
#define END_PM_APP 0x0016
#define DEVICE_REBOOT_NOTIFY 0x0017
#define CRIT_REBOOT_NOTIFY 0x0018
#define CLOSE_VM_NOTIFY 0x0019
#define POWER_EVENT 0x001A
/* Windows 95 and later. */
#define SYS_DYNAMIC_DEVICE_INIT 0x001B
#define SYS_DYNAMIC_DEVICE_EXIT 0x001C
#define W32_DEVICEIOCONTROL 0x0023
#endif

/* The Device Descriptor Block. The fields after DDB_Service_Table_Size
   are Windows 95's; Windows 3.1 does not read them. */
struct vxd_ddb {
  DWORD DDB_Next;
  WORD DDB_SDK_Version;
  WORD DDB_Req_Device_Number;
  BYTE DDB_Dev_Major_Version;
  BYTE DDB_Dev_Minor_Version;
  WORD DDB_Flags;
  char DDB_Name[8];
  DWORD DDB_Init_Order;
  DWORD DDB_Control_Proc;
  DWORD DDB_V86_API_Proc;
  DWORD DDB_PM_API_Proc;
  DWORD DDB_V86_API_CSIP;
  DWORD DDB_PM_API_CSIP;
  DWORD DDB_Reference_Data;
  DWORD DDB_Service_Table_Ptr;
  DWORD DDB_Service_Table_Size;
  DWORD DDB_Win32_Service_Table;
  DWORD DDB_Prev;
  DWORD DDB_Size;
  DWORD DDB_Reserved1;
  DWORD DDB_Reserved2;
  DWORD DDB_Reserved3;
};

/* The registers the VMM passed to the control procedure. */
struct vxd_regs {
  DWORD eax, ebx, ecx, edx, esi, edi, ebp;
};

#ifndef _VMM_
/* What W32_DEVICEIOCONTROL passes in ESI: a Win32 program's
   DeviceIoControl call. Its buffers are the program's, at their linear
   addresses, valid while the call lasts. dwIoControlCode is DIOC_OPEN
   when the program opens the VxD with CreateFile("\\\\.\\NAME.VXD") and
   DIOC_CLOSEHANDLE when it closes it; return 0 in EAX to allow them. */
struct DIOCParams {
  DWORD Internal1;
  DWORD VMHandle;
  DWORD Internal2;
  DWORD dwIoControlCode;
  DWORD lpvInBuffer;
  DWORD cbInBuffer;
  DWORD lpvOutBuffer;
  DWORD cbOutBuffer;
  DWORD lpcbBytesReturned;
  DWORD lpoOverlapped;
  DWORD hDevice;
  DWORD tagProcess;
};
#define DIOC_OPEN 0
#define DIOC_GETVERSION 0
#define DIOC_CLOSEHANDLE ((DWORD)-1)
#endif

#define VXD_STR_(x) #x
#define VXD_STR(x) VXD_STR_(x)

/* Define the DDB and the control procedure of a VxD named NAME (at most
   8 characters, the name Windows knows it by). The control procedure
   calls int vxd_control(DWORD msg, struct vxd_regs *r). */
#define VXD_DEVICE(NAME, ID, ORDER, MAJOR, MINOR)                            \
  void NAME##_Control(void);                                                 \
  int vxd_control(DWORD msg, struct vxd_regs *r);                            \
  struct vxd_ddb NAME##_DDB = {                                              \
      0, VXD_DDK_VERSION, (ID), (MAJOR), (MINOR), 0,                         \
      {VXD_PAD8_(#NAME)}, (ORDER), (DWORD)NAME##_Control,                    \
      0, 0, 0, 0, 0, 0, 0, 0,                                                \
      0x50726576UL /* "Prev" */, sizeof(struct vxd_ddb),                     \
      0x52737631UL, 0x52737632UL, 0x52737633UL /* "Rsv1" ... */};           \
  __asm__(".text\n.globl _" VXD_STR(NAME) "_Control\n"                     \
          "_" VXD_STR(NAME) "_Control:\n"                                    \
          "  pushl %ebp\n  pushl %edi\n  pushl %esi\n  pushl %edx\n"        \
          "  pushl %ecx\n  pushl %ebx\n  pushl %eax\n"                      \
          "  movl %esp, %ecx\n  cld\n"                                       \
          "  pushl %ecx\n  pushl %eax\n  call _vxd_control\n"               \
          "  addl $8, %esp\n  testl %eax, %eax\n"                           \
          "  popl %eax\n  popl %ebx\n  popl %ecx\n  popl %edx\n"            \
          "  popl %esi\n  popl %edi\n  popl %ebp\n"                         \
          "  jnz 1f\n  clc\n  ret\n1:\n  stc\n  ret\n")

/* The name, padded with spaces to 8 characters. */
#define VXD_PAD8_(s)                                                         \
  (sizeof(s) > 1 ? s[0] : ' '), (sizeof(s) > 2 ? s[1] : ' '),                \
      (sizeof(s) > 3 ? s[2] : ' '), (sizeof(s) > 4 ? s[3] : ' '),            \
      (sizeof(s) > 5 ? s[4] : ' '), (sizeof(s) > 6 ? s[5] : ' '),            \
      (sizeof(s) > 7 ? s[6] : ' '), (sizeof(s) > 8 ? s[7] : ' ')

/* A service of a VxD: its device ID and number, as the dword after int
   20h. VXD_INT20 is the call, for inline assembly. */
#define VXD_SERVICE(DEV, NUM) (((DWORD)(DEV) << 16) | (NUM))
#define VXD_INT20(SERVICE) "int $0x20\n.long " VXD_STR(SERVICE) "\n"

/* The services of the VMM and of the VxDs of Windows: VXDSVC_<name> is
   the dword after int 20h, defined when VXD_WINVER (3.10 = 0x030A, 4.00
   = 0x0400 for 95, 4.03 for 95 OSR2, 4.10 = 0x040A for 98, 4.90 =
   0x045A for ME) is at least the version of Windows that has it. Each
   service that is called like a C function (its name starts with an
   underscore, as _HeapAllocate) has a stub in libvxd.a. Services new
   since the 95 DDK are given as 4.90 unless their table names 98 (no 98
   DDK was used). */
#ifndef VXD_WINVER
#define VXD_WINVER VXD_DDK_VERSION
#endif
#include <vxdsvc.h>

/* Call a register service: VxDCall(Get_Cur_VM_Handle, &r) loads EAX,
   EBX, ECX, EDX, ESI, EDI and EBP from the struct vxd_regs, calls the
   service and stores them back. It is 1 if the service set the carry
   flag (most of them do on failure), 0 if not. With a Microsoft DDK,
   whose vmm.h has its own VxDCall, it is VXD_CALL. */
#define VXD_CALL(NAME, R) vxd_call_((R), VXDSVC_##NAME)
#ifndef _VMM_
#define VxDCall VXD_CALL
#define VMMCall VXD_CALL
#endif
#define vxd_call_(R, SERVICE)                                                \
  __extension__({                                                            \
    struct vxd_regs *vxd_r_ = (R);                                           \
    int vxd_c_;                                                              \
    __asm__ __volatile__(                                                    \
        "pushl %%ebp\n\tpushl %%ebx\n\tpushl %%esi\n\tpushl %%edi\n\t"       \
        "pushl %%eax\n\t"                                                    \
        "movl 4(%%eax), %%ebx\n\tmovl 8(%%eax), %%ecx\n\t"                   \
        "movl 12(%%eax), %%edx\n\tmovl 16(%%eax), %%esi\n\t"                 \
        "movl 20(%%eax), %%edi\n\tmovl 24(%%eax), %%ebp\n\t"                 \
        "movl (%%eax), %%eax\n\t"                                            \
        "int $0x20\n\t.long %c[svc]\n\t"                                     \
        "xchgl %%eax, (%%esp)\n\t"                                           \
        "movl %%ebx, 4(%%eax)\n\tmovl %%ecx, 8(%%eax)\n\t"                   \
        "movl %%edx, 12(%%eax)\n\tmovl %%esi, 16(%%eax)\n\t"                 \
        "movl %%edi, 20(%%eax)\n\tmovl %%ebp, 24(%%eax)\n\t"                 \
        "popl (%%eax)\n\t"                                                   \
        "popl %%edi\n\tpopl %%esi\n\tpopl %%ebx\n\tpopl %%ebp"               \
        : "+a"(vxd_r_), "=@ccc"(vxd_c_)                                      \
        : [svc] "i"(SERVICE)                                                 \
        : "ecx", "edx", "memory");                                           \
    vxd_c_;                                                                  \
  })

/* A procedure the VMM or a VxD calls with registers (a time-out, an
   event, an I/O trap, a hardware interrupt): VXD_CALLBACK(NAME, FN)
   defines NAME, which saves the registers in a struct vxd_regs, calls
   int FN(struct vxd_regs *r) and loads them back, with the carry flag
   set if FN returned nonzero. Pass NAME's address to the service, as
   (DWORD)NAME. NAME is locked code. */
#define VXD_CALLBACK(NAME, FN)                                               \
  void NAME(void);                                                           \
  int FN(struct vxd_regs *r);                                                \
  __asm__(".text\n.globl _" VXD_STR(NAME) "\n"                               \
          "_" VXD_STR(NAME) ":\n"                                            \
          "  pushl %ebp\n  pushl %edi\n  pushl %esi\n  pushl %edx\n"        \
          "  pushl %ecx\n  pushl %ebx\n  pushl %eax\n"                      \
          "  movl %esp, %ecx\n  cld\n"                                       \
          "  pushl %ecx\n  call _" VXD_STR(FN) "\n"                          \
          "  addl $4, %esp\n  testl %eax, %eax\n"                           \
          "  popl %eax\n  popl %ebx\n  popl %ecx\n  popl %edx\n"            \
          "  popl %esi\n  popl %edi\n  popl %ebp\n"                         \
          "  jnz 1f\n  clc\n  ret\n1:\n  stc\n  ret\n")

/* Where code and data go. A VxD's code and data are locked (always in
   memory) unless marked: init code and data are thrown away once the
   VxD has initialized (after Init_Complete for a static VxD, after
   Sys_Dynamic_Device_Init for a dynamic one), and pageable ones can be
   paged out, so a hardware interrupt or anything that runs with
   interrupts off must not touch them. The DDB, the control procedure and
   the callbacks are locked. Strings and other constants are locked. The
   compiler keeps init and pageable functions out of line, but may fold
   a variable it sees is never written into the code that reads it.
     VXD_INIT_CODE int setup(void) { ... }
     VXD_PAGEABLE_DATA char buffer[4096]; */
#define VXD_LOCKED_CODE
#define VXD_LOCKED_DATA
#define VXD_INIT_CODE __attribute__((section("_ITEXT"), noinline))
#define VXD_INIT_DATA __attribute__((section("_IDATA")))
#define VXD_PAGEABLE_CODE __attribute__((section("_PTEXT"), noinline))
#define VXD_PAGEABLE_DATA __attribute__((section("_PDATA")))

/* Get_VMM_Version: AH.AL is the VMM's version (3.10 on Windows 3.1, 4.00
   on 95, 4.10 on 98, 4.90 on ME). */
static __inline__ WORD VMMCALL_Get_VMM_Version(void) {
  DWORD a, c;
  __asm__ __volatile__(VXD_INT20(0x00010000) : "=a"(a), "=c"(c)::"memory");
  return (WORD)a;
}

/* Get_Cur_VM_Handle and Get_Sys_VM_Handle: the current virtual machine,
   and the system one, where Windows programs run (EBX). */
static __inline__ DWORD VMMCALL_Get_Cur_VM_Handle(void) {
  DWORD vm;
  __asm__ __volatile__(VXD_INT20(0x00010001) : "=b"(vm)::"memory");
  return vm;
}
static __inline__ DWORD VMMCALL_Get_Sys_VM_Handle(void) {
  DWORD vm;
  __asm__ __volatile__(VXD_INT20(0x00010003) : "=b"(vm)::"memory");
  return vm;
}

/* Get_System_Time: milliseconds since Windows started. */
static __inline__ DWORD VMMCALL_Get_System_Time(void) {
  struct vxd_regs r = {0};
  VXD_CALL(Get_System_Time, &r);
  return r.eax;
}

/* Out_Debug_String: a line for the debugger (a debug build of the VMM,
   or WDEB386). */
static __inline__ void VMMCALL_Out_Debug_String(const char *s) {
  struct vxd_regs r = {0};
  r.esi = (DWORD)s;
  VXD_CALL(Out_Debug_String, &r);
}

/* Set_Global_Time_Out: call CALLBACK (a VXD_CALLBACK) once after MS
   milliseconds, with REF in EDX. The handle, for Cancel_Time_Out, or 0
   if it failed. */
static __inline__ DWORD VMMCALL_Set_Global_Time_Out(DWORD ms, DWORD ref,
                                                    void (*callback)(void)) {
  struct vxd_regs r = {0};
  r.eax = ms;
  r.edx = ref;
  r.esi = (DWORD)callback;
  VXD_CALL(Set_Global_Time_Out, &r);
  return r.esi;
}
static __inline__ void VMMCALL_Cancel_Time_Out(DWORD handle) {
  struct vxd_regs r = {0};
  r.esi = handle;
  VXD_CALL(Cancel_Time_Out, &r);
}

/* Schedule_Global_Event: call CALLBACK (a VXD_CALLBACK) with REF in EDX
   as soon as the VMM can, outside interrupt time; the handle, for
   Cancel_Global_Event. */
static __inline__ DWORD VMMCALL_Schedule_Global_Event(DWORD ref,
                                                      void (*callback)(void)) {
  struct vxd_regs r = {0};
  r.edx = ref;
  r.esi = (DWORD)callback;
  VXD_CALL(Schedule_Global_Event, &r);
  return r.esi;
}
static __inline__ void VMMCALL_Cancel_Global_Event(DWORD handle) {
  struct vxd_regs r = {0};
  r.esi = handle;
  VXD_CALL(Cancel_Global_Event, &r);
}

#ifndef _VMM_
/* SHELL_SYSMODAL_Message: a system-modal message box in VM VM (the
   system VM for Windows), with MB_* flags; the button pressed (IDOK, ...).
   Not from a hardware interrupt. */
#define MB_OK 0x00
#define MB_OKCANCEL 0x01
#define MB_YESNO 0x04
#define MB_ICONASTERISK 0x40
#define MB_ICONEXCLAMATION 0x30
#define MB_SYSTEMMODAL 0x1000
#define MB_NOWINDOW 0x40000000UL
#define MB_ASAP 0x80000000UL
#ifndef IDOK
#define IDOK 1
#define IDCANCEL 2
#define IDYES 6
#define IDNO 7
#endif
static __inline__ DWORD SHELL_SYSMODAL_Message(DWORD vm, DWORD flags,
                                               const char *text,
                                               const char *caption) {
  struct vxd_regs r = {0};
  r.ebx = vm;
  r.eax = flags;
  r.ecx = (DWORD)text;
  r.edi = (DWORD)caption;
  VXD_CALL(SHELL_SYSMODAL_Message, &r);
  return r.eax;
}

/* Memory. _HeapAllocate takes HEAP* flags (0 for locked memory);
   _PageAllocate takes a page type (PG_SYS for memory in every VM) and
   PAGE* flags, and returns the handle in its low dword and the linear
   address in its high one. */
#define HEAPZEROINIT 0x00000001UL
#define HEAPZEROREINIT 0x00000002UL
#define HEAPLOCKEDIFDP 0x00000100UL
#define HEAPSWAP 0x00000200UL
#define PG_VM 0
#define PG_SYS 1
#define PG_PRIVATE 3
#define PG_HOOKED 7
#define PAGEZEROINIT 0x00000001UL
#define PAGEUSEALIGN 0x00000002UL
#define PAGECONTIG 0x00000004UL
#define PAGEFIXED 0x00000008UL
#define PAGELOCKED 0x00000080UL
#define PAGELOCKEDIFDP 0x00000100UL
#define PAGEDISCARD 0x00010000UL
#define PAGEMAPGLOBAL 0x40000000UL
void *_HeapAllocate(DWORD nbytes, DWORD flags);
DWORD _HeapFree(void *block, DWORD flags);
void *_HeapReAllocate(void *block, DWORD nbytes, DWORD flags);
DWORD _HeapGetSize(void *block, DWORD flags);
unsigned long long _PageAllocate(DWORD npages, DWORD type, DWORD vm,
                                 DWORD alignmask, DWORD minphys,
                                 DWORD maxphys, DWORD *physaddr, DWORD flags);
DWORD _PageFree(DWORD handle, DWORD flags);
DWORD _LinPageLock(DWORD page, DWORD npages, DWORD flags);
DWORD _LinPageUnLock(DWORD page, DWORD npages, DWORD flags);
void *_MapPhysToLinear(DWORD phys, DWORD nbytes, DWORD flags);

/* The registry (Windows 95 and later); they return ERROR_SUCCESS (0) or
   an error. */
#if VXD_WINVER >= 0x0400
#define HKEY_CLASSES_ROOT 0x80000000UL
#define HKEY_CURRENT_USER 0x80000001UL
#define HKEY_LOCAL_MACHINE 0x80000002UL
#define HKEY_USERS 0x80000003UL
#define HKEY_CURRENT_CONFIG 0x80000005UL
#define HKEY_DYN_DATA 0x80000006UL
#ifndef REG_SZ
#define REG_SZ 1
#define REG_BINARY 3
#define REG_DWORD 4
#endif
DWORD _RegOpenKey(DWORD key, const char *subkey, DWORD *result);
DWORD _RegCreateKey(DWORD key, const char *subkey, DWORD *result);
DWORD _RegCloseKey(DWORD key);
DWORD _RegDeleteKey(DWORD key, const char *subkey);
DWORD _RegQueryValueEx(DWORD key, const char *name, DWORD *reserved,
                       DWORD *type, void *data, DWORD *size);
DWORD _RegSetValueEx(DWORD key, const char *name, DWORD reserved,
                     DWORD type, const void *data, DWORD size);
DWORD _RegDeleteValue(DWORD key, const char *name);
DWORD _RegFlushKey(DWORD key);

/* VWIN32: Win32 events (from OpenVxDHandle in the program) and APCs. */
DWORD _VWIN32_SetWin32Event(DWORD event);
DWORD _VWIN32_ResetWin32Event(DWORD event);
DWORD _VWIN32_CloseVxDHandle(DWORD handle);
void _VWIN32_QueueUserApc(void *apc, DWORD param, DWORD thread);
#endif

/* VPICD_Virtualize_IRQ takes one of these in EDI and returns the IRQ's
   handle in EAX (carry set if it failed); the procedures are
   VXD_CALLBACKs. */
struct VPICD_IRQ_Descriptor {
  WORD VID_IRQ_Number;
  WORD VID_Options;
  DWORD VID_Hw_Int_Proc;
  DWORD VID_Virt_Int_Proc;
  DWORD VID_EOI_Proc;
  DWORD VID_Mask_Change_Proc;
  DWORD VID_IRET_Proc;
  DWORD VID_IRET_Time_Out;
  DWORD VID_Hw_Int_Ref;
};
#define VPICD_OPT_READ_HW_IRR 0x01
#define VPICD_OPT_CAN_SHARE 0x02
#define VPICD_OPT_REF_DATA 0x04

/* Install_IO_Handler's callbacks get the kind of access in ECX. */
#define BYTE_INPUT 0x000
#define BYTE_OUTPUT 0x004
#define WORD_INPUT 0x008
#define WORD_OUTPUT 0x00C
#define DWORD_INPUT 0x010
#define DWORD_OUTPUT 0x014
#endif /* !_VMM_ */

#endif
