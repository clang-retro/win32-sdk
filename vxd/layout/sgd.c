/* Made by tools/gen-vxd-headers.py: the layouts of <sgd.h> as the 98 DDK's
   compiler laid them out. */
#define VXD_WINVER 0x040A
#include <sgd.h>
#include <stddef.h>
#ifdef __cplusplus
#define A(e) static_assert(e, #e);
#else
#define A(e) _Static_assert(e, #e);
#endif
A(sizeof(struct _SGD) == 8)
A(offsetof(struct _SGD, SG_buff_ptr) == 0)
A(offsetof(struct _SGD, SG_buff_size) == 4)
