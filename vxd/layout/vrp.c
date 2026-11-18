/* Made by tools/gen-vxd-headers.py: the layouts of <vrp.h> as the 98 DDK's
   compiler laid them out. */
#define VXD_WINVER 0x040A
#include <vrp.h>
#include <stddef.h>
#ifdef __cplusplus
#define A(e) static_assert(e, #e);
#else
#define A(e) _Static_assert(e, #e);
#endif
A(sizeof(struct _VRP) == 48)
A(offsetof(struct _VRP, VRP_demand_flags) == 0)
A(offsetof(struct _VRP, VRP_event_flags) == 4)
A(offsetof(struct _VRP, VRP_max_sgd) == 8)
A(offsetof(struct _VRP, VRP_max_req_size) == 10)
A(offsetof(struct _VRP, VRP_delta_to_ior) == 12)
A(offsetof(struct _VRP, VRP_block_size) == 16)
A(offsetof(struct _VRP, VRP_fsd_hvol) == 20)
A(offsetof(struct _VRP, VRP_fsd_entry) == 24)
A(offsetof(struct _VRP, VRP_device_handle) == 28)
A(offsetof(struct _VRP, VRP_partition_offset) == 32)
A(offsetof(struct _VRP, VRP_next_vrp) == 36)
A(offsetof(struct _VRP, VRP_logical_handle) == 40)
A(offsetof(struct _VRP, VRP_reserved) == 44)
