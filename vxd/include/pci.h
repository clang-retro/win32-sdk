/* pci.h: The PCI bus: the configuration space layout
   and the PCI enumerator's constants.

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   VXD_WINVER whose DDK has it. */
#ifndef VXD_PCI_H
#define VXD_PCI_H

#include <vxd.h>
#include <vxdtypes.h>
#include <configmg.h>

#pragma pack(push, 1)
#define PCI_ENUM_FUNC_GET_DEVICE_INFO 0
#define PCI_ENUM_FUNC_SET_DEVICE_INFO 1
#define NUM_PCI_ENUM_FUNC 2
#if VXD_WINVER >= 0x040A
#define PCI_ENUM_FUNC_GET_ENUM_HANDLERS 2
#define PCI_ENUM_FUNC_GET_ROM_DATA_SIZE 3
#define PCI_ENUM_FUNC_GET_ROM_DATA 4
#define PCI_ENUM_FUNC_BEGIN_VGA 5
#define PCI_ENUM_FUNC_END_VGA 6
#define PCI_ENUM_FUNC_NO_VGA_PROCESS 7
#define DEBUG_PCI_ENUM_FUNC_INSTALL_HOOK 0x8000
#define DEBUG_PCI_ENUM_FUNC_UNINSTALL_HOOK 0x8001
#define ResType_PCI_Bus_Number 0x43
#define PCI_LOCK 1
#define PCI_UNLOCK 0
#define SIZE_BYTE 0x00
#define SIZE_WORD 0x01
#define SIZE_DWORD 0x02
#define HOOKM_INSTALL 0x00
#define HOOKM_UNINSTALL 0x01
#define HOOKM_CONFIGREAD 0x00010000
#define HOOKM_CONFIGWRITE 0x00010001
#define HOOKM_CALLBIOS 0x00020000
struct Pci_Enum_Handlers_s {
  CMENUMHANDLER pfnPCIEnumHandler;
  CMENUMFUNCTION pfnPCIEnumFuncHandler;
};
typedef struct Pci_Enum_Handlers_s PCIENUMHANDLERS;
typedef PCIENUMHANDLERS *PPCIENUMHANDLERS;
struct _REGS {
  ULONG rEAX;
  ULONG rEBX;
  ULONG rECX;
  ULONG rEDX;
  ULONG rESI;
  ULONG rEDI;
};
typedef struct _REGS REGS;
typedef REGS *PREGS;
struct _CONFIGACCESS {
  UCHAR bBus;
  UCHAR bDevFunc;
  UCHAR bOffset;
  UCHAR bSize;
};
typedef struct _CONFIGACCESS CONFIGACCESS;
typedef CONFIGACCESS *PCONFIGACCESS;
typedef ULONG (*PPCIHOOKFUNC)(ULONG, ULONG, ULONG);
#endif

#pragma pack(pop)

#endif
