/* vwin32.h: VWIN32, the Win32 side of the VMM: the structures for
   events, APCs, threads and the Win32 services (DIOCParams is in
   <vxd.h>).

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   VXD_WINVER whose DDK has it. */
#ifndef VXD_VWIN32_H
#define VXD_VWIN32_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
#if VXD_WINVER >= 0x040A
#define THREAD_TYPE_WIN32 0x2A
#define THREAD_CONDITION_DOS_BOX 0x00
#define THREAD_CONDITION_V86_NEST 0x01
#define THREAD_CONDITION_WDM 0x02
#define THREAD_CONDITION_INDETERMINATE 0x03
#define THREAD_CONDITION_LOCKED_STACK 0x04
#define THREAD_CONDITION_PURE_WIN16 0x05
#define THREAD_CONDITION_THUNKED_WIN16 0x06
#define THREAD_CONDITION_THUNKED_WIN32 0x07
#define THREAD_CONDITION_PURE_WIN32 0x08
#define THREAD_CONDITION_APPY_TIME 0x09
#define THREAD_CONDITION_RING0_APPY_TIME 0x0A
#define THREAD_CONDITION_EXIT 0x0B
#define THREAD_CONDITION_INVALID_FLAGS 0xFFFFFFFF
#define THREAD_CONDITION_NORMAL_FLAGS 0x00
#define SEB_ANSI 0x4000
#define SEB_TERMINATE 0x2000
#endif
#define KERNEL_APC_IGNORE_MC 0x01
#define KERNEL_APC_STATIC 0x02
#define KERNEL_APC_WAKE 0x04
#define VWIN32_DIOC_GETVERSION 0x00
#define VWIN32_DIOC_DOS_IOCTL 1
#define VWIN32_DIOC_DOS_INT25 2
#define VWIN32_DIOC_DOS_INT26 3
#define VWIN32_DIOC_DOS_INT13 4
#define VWIN32_DIOC_SIMCTRLC 5
#if VXD_WINVER >= 0x040A
#define VWIN32_DIOC_DOS_DRIVEINFO 6
#endif
#define VWIN32_DIOC_CLOSEHANDLE (-1)
#if VXD_WINVER >= 0x040A
#define OPENVXD_TYPE_SEMAPHORE 0
#define OPENVXD_TYPE_EVENT 1
#define OPENVXD_TYPE_MUTEX 2
#define OPENVXD_TYPE_ANY 3
#define R0EHF_INHERIT 0x01
#define R0EHF_GLOBAL 0x02
#endif
struct vseb_s {
  DWORD vseb_resp;
  WORD vseb_b3;
  WORD vseb_b2;
  WORD vseb_b1;
  DWORD vseb_pszCaption;
  DWORD vseb_pszText;
};
typedef struct vseb_s VSEB;
typedef VSEB *PVSEB;
typedef struct DIOCParams DIOCPARAMETERS;
typedef DIOCPARAMETERS *PDIOCPARAMETERS;
struct DIOCRegs {
  DWORD reg_EBX;
  DWORD reg_EDX;
  DWORD reg_ECX;
  DWORD reg_EAX;
  DWORD reg_EDI;
  DWORD reg_ESI;
  DWORD reg_Flags;
};
typedef struct DIOCRegs DIOC_REGISTERS;
struct _OVERLAPPED {
  DWORD O_Internal;
  DWORD O_InternalHigh;
  DWORD O_Offset;
  DWORD O_OffsetHigh;
  HANDLE O_hEvent;
};
typedef struct _OVERLAPPED OVERLAPPED;
#if VXD_WINVER >= 0x040A
typedef void (*R0OBJFREE)(PVOID) __attribute__((stdcall));
typedef PVOID (*R0OBJDUP)(PVOID, DWORD) __attribute__((stdcall));
struct _R0OBJTYPETABLE {
  DWORD ott_dwSize;
  R0OBJFREE ott_pfnFree;
  R0OBJDUP ott_pfnDup;
};
typedef struct _R0OBJTYPETABLE R0OBJTYPETABLE;
typedef struct _R0OBJTYPETABLE *PR0OBJTYPETABLE;
#endif

#pragma pack(pop)

#endif
