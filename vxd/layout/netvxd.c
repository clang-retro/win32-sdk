/* Made by tools/gen-vxd-headers.py: the layouts of <netvxd.h> as the 98 DDK's
   compiler laid them out. */
#define VXD_WINVER 0x040A
#include <netvxd.h>
#include <stddef.h>
#ifdef __cplusplus
#define A(e) static_assert(e, #e);
#else
#define A(e) _Static_assert(e, #e);
#endif
