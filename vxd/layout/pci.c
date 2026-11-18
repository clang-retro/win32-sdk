/* Made by tools/gen-vxd-headers.py: the layouts of <pci.h> as the 98 DDK's
   compiler laid them out. */
#define VXD_WINVER 0x040A
#include <pci.h>
#include <stddef.h>
#ifdef __cplusplus
#define A(e) static_assert(e, #e);
#else
#define A(e) _Static_assert(e, #e);
#endif
A(sizeof(struct Pci_Enum_Handlers_s) == 8)
A(offsetof(struct Pci_Enum_Handlers_s, pfnPCIEnumHandler) == 0)
A(offsetof(struct Pci_Enum_Handlers_s, pfnPCIEnumFuncHandler) == 4)
A(sizeof(struct _REGS) == 24)
A(offsetof(struct _REGS, rEAX) == 0)
A(offsetof(struct _REGS, rEBX) == 4)
A(offsetof(struct _REGS, rECX) == 8)
A(offsetof(struct _REGS, rEDX) == 12)
A(offsetof(struct _REGS, rESI) == 16)
A(offsetof(struct _REGS, rEDI) == 20)
A(sizeof(struct _CONFIGACCESS) == 4)
A(offsetof(struct _CONFIGACCESS, bBus) == 0)
A(offsetof(struct _CONFIGACCESS, bDevFunc) == 1)
A(offsetof(struct _CONFIGACCESS, bOffset) == 2)
A(offsetof(struct _CONFIGACCESS, bSize) == 3)
