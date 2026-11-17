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
   such call into a direct one the first time it runs. VMMCALL_* below
   wrap the few VMM services this header knows; VXD_SERVICE and
   VXD_INT20 build others. Nothing here is taken from Microsoft's DDKs. */
#ifndef CLANG_RETRO_VXD_H
#define CLANG_RETRO_VXD_H

#ifndef __i386__
#error "VxDs are 32-bit x86 code: use --config=win95-vxd.cfg"
#endif

#ifndef _WINDEF_
typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;
#endif

/* The VMM and the version numbers of the DDB and of Get_VMM_Version. */
#define VMM_DEVICE_ID 0x0001
#define UNDEFINED_DEVICE_ID 0x0000
#define UNDEFINED_INIT_ORDER 0x80000000UL
#define VXD_DDK_VERSION_31 0x030A /* Windows 3.1 */
#define VXD_DDK_VERSION_40 0x0400 /* Windows 95, 98, ME */
#ifndef VXD_DDK_VERSION
#define VXD_DDK_VERSION VXD_DDK_VERSION_40
#endif

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

#endif
