/* Made by tools/gen-vxd-inc.py: the layouts of <vkd.h> as MASM lays out
   the 98 DDK's vkd.inc. */
#define VXD_WINVER 0x040A
#include <vkd.h>
#include <stddef.h>
#ifdef __cplusplus
#define A(e) static_assert(e, #e);
#else
#define A(e) _Static_assert(e, #e);
#endif
A(sizeof(Paste_Rec) == 4)
A(offsetof(Paste_Rec, Paste_Char) == 0)
A(offsetof(Paste_Rec, Paste_ScanCode) == 1)
A(offsetof(Paste_Rec, Paste_ShiftState) == 2)
