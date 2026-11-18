/* Made by tools/gen-vxd-headers.py: the layouts of <shell.h> as the 98 DDK's
   compiler laid them out. */
#define VXD_WINVER 0x040A
#include <shell.h>
#include <stddef.h>
#ifdef __cplusplus
#define A(e) static_assert(e, #e);
#else
#define A(e) _Static_assert(e, #e);
#endif
A(sizeof(struct _SHELL_SUUAE_INFO) == 11)
A(offsetof(struct _SHELL_SUUAE_INFO, ssiTimeLastActiveDisplay) == 0)
A(offsetof(struct _SHELL_SUUAE_INFO, ssiTimeLastActiveSystem) == 4)
A(offsetof(struct _SHELL_SUUAE_INFO, ssiHoldDisplay) == 8)
A(offsetof(struct _SHELL_SUUAE_INFO, ssiHoldSystem) == 9)
A(offsetof(struct _SHELL_SUUAE_INFO, ssiLowLatency) == 10)
A(sizeof(struct _SHEXPACKET) == 32)
A(offsetof(struct _SHEXPACKET, dwTotalSize) == 0)
A(offsetof(struct _SHEXPACKET, dwSize) == 4)
A(offsetof(struct _SHEXPACKET, ibOp) == 8)
A(offsetof(struct _SHEXPACKET, ibFile) == 12)
A(offsetof(struct _SHEXPACKET, ibParams) == 16)
A(offsetof(struct _SHEXPACKET, ibDir) == 20)
A(offsetof(struct _SHEXPACKET, dwReserved) == 24)
A(offsetof(struct _SHEXPACKET, nCmdShow) == 28)
