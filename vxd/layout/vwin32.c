/* Made by tools/gen-vxd-headers.py: the layouts of <vwin32.h> as the 98 DDK's
   compiler laid them out. */
#define VXD_WINVER 0x040A
#include <vwin32.h>
#include <stddef.h>
#ifdef __cplusplus
#define A(e) static_assert(e, #e);
#else
#define A(e) _Static_assert(e, #e);
#endif
A(sizeof(struct vseb_s) == 18)
A(offsetof(struct vseb_s, vseb_resp) == 0)
A(offsetof(struct vseb_s, vseb_b3) == 4)
A(offsetof(struct vseb_s, vseb_b2) == 6)
A(offsetof(struct vseb_s, vseb_b1) == 8)
A(offsetof(struct vseb_s, vseb_pszCaption) == 10)
A(offsetof(struct vseb_s, vseb_pszText) == 14)
A(sizeof(struct DIOCParams) == 48)
A(offsetof(struct DIOCParams, Internal1) == 0)
A(offsetof(struct DIOCParams, VMHandle) == 4)
A(offsetof(struct DIOCParams, Internal2) == 8)
A(offsetof(struct DIOCParams, dwIoControlCode) == 12)
A(offsetof(struct DIOCParams, lpvInBuffer) == 16)
A(offsetof(struct DIOCParams, cbInBuffer) == 20)
A(offsetof(struct DIOCParams, lpvOutBuffer) == 24)
A(offsetof(struct DIOCParams, cbOutBuffer) == 28)
A(offsetof(struct DIOCParams, lpcbBytesReturned) == 32)
A(offsetof(struct DIOCParams, lpoOverlapped) == 36)
A(offsetof(struct DIOCParams, hDevice) == 40)
A(offsetof(struct DIOCParams, tagProcess) == 44)
A(sizeof(struct DIOCRegs) == 28)
A(offsetof(struct DIOCRegs, reg_EBX) == 0)
A(offsetof(struct DIOCRegs, reg_EDX) == 4)
A(offsetof(struct DIOCRegs, reg_ECX) == 8)
A(offsetof(struct DIOCRegs, reg_EAX) == 12)
A(offsetof(struct DIOCRegs, reg_EDI) == 16)
A(offsetof(struct DIOCRegs, reg_ESI) == 20)
A(offsetof(struct DIOCRegs, reg_Flags) == 24)
A(sizeof(struct _OVERLAPPED) == 20)
A(offsetof(struct _OVERLAPPED, O_Internal) == 0)
A(offsetof(struct _OVERLAPPED, O_InternalHigh) == 4)
A(offsetof(struct _OVERLAPPED, O_Offset) == 8)
A(offsetof(struct _OVERLAPPED, O_OffsetHigh) == 12)
A(offsetof(struct _OVERLAPPED, O_hEvent) == 16)
A(sizeof(struct _R0OBJTYPETABLE) == 12)
A(offsetof(struct _R0OBJTYPETABLE, ott_dwSize) == 0)
A(offsetof(struct _R0OBJTYPETABLE, ott_pfnFree) == 4)
A(offsetof(struct _R0OBJTYPETABLE, ott_pfnDup) == 8)
