/* Made by tools/gen-vxd-headers.py: the layouts of <vcache.h> as the 98 DDK's
   compiler laid them out. */
#define VXD_WINVER 0x040A
#include <vcache.h>
#include <stddef.h>
#ifdef __cplusplus
#define A(e) static_assert(e, #e);
#else
#define A(e) _Static_assert(e, #e);
#endif
A(sizeof(struct _VCACHE_BLOCK) == 51)
A(offsetof(struct _VCACHE_BLOCK, Reserved1) == 0)
A(offsetof(struct _VCACHE_BLOCK, FSKey1) == 8)
A(offsetof(struct _VCACHE_BLOCK, FSKey2) == 12)
A(offsetof(struct _VCACHE_BLOCK, BufPtr) == 16)
A(offsetof(struct _VCACHE_BLOCK, FSData) == 20)
A(offsetof(struct _VCACHE_BLOCK, Reserved2) == 48)
A(offsetof(struct _VCACHE_BLOCK, Dirty) == 50)
