/* Made by tools/gen-vxd-headers.py: the layouts of <ilb.h> as the 98 DDK's
   compiler laid them out. */
#define VXD_WINVER 0x040A
#include <ilb.h>
#include <stddef.h>
#ifdef __cplusplus
#define A(e) static_assert(e, #e);
#else
#define A(e) _Static_assert(e, #e);
#endif
A(sizeof(struct ILB) == 52)
A(offsetof(struct ILB, ILB_service_rtn) == 0)
A(offsetof(struct ILB, ILB_dprintf_rtn) == 4)
A(offsetof(struct ILB, ILB_Wait_10th_Sec) == 8)
A(offsetof(struct ILB, ILB_internal_request) == 12)
A(offsetof(struct ILB, ILB_io_criteria_rtn) == 16)
A(offsetof(struct ILB, ILB_int_io_criteria_rtn) == 20)
A(offsetof(struct ILB, ILB_dvt) == 24)
A(offsetof(struct ILB, ILB_ios_mem_virt) == 28)
A(offsetof(struct ILB, ILB_enqueue_iop) == 32)
A(offsetof(struct ILB, ILB_dequeue_iop) == 36)
A(offsetof(struct ILB, ILB_reserved_1) == 40)
A(offsetof(struct ILB, ILB_reserved_2) == 44)
A(offsetof(struct ILB, ILB_flags) == 48)
A(offsetof(struct ILB, ILB_driver_numb) == 50)
A(offsetof(struct ILB, ILB_reserved_3) == 51)
