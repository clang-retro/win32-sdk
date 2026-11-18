/* Made by tools/gen-vxd-headers.py: the layouts of <ivt.h> as the 98 DDK's
   compiler laid them out. */
#define VXD_WINVER 0x040A
#include <ivt.h>
#include <stddef.h>
#ifdef __cplusplus
#define A(e) static_assert(e, #e);
#else
#define A(e) _Static_assert(e, #e);
#endif
A(sizeof(struct IVT) == 12)
A(offsetof(struct IVT, IVT_ios_mem_virtual) == 0)
A(offsetof(struct IVT, IVT_ios_mem_phys) == 4)
A(offsetof(struct IVT, IVT_first_dvt) == 8)
