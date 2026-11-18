/* Made by tools/gen-vxd-headers.py: the layouts of <iop.h> as the 98 DDK's
   compiler laid them out. */
#define VXD_WINVER 0x040A
#include <iop.h>
#include <stddef.h>
#ifdef __cplusplus
#define A(e) static_assert(e, #e);
#else
#define A(e) _Static_assert(e, #e);
#endif
A(sizeof(struct IOP_callback_entry) == 8)
A(offsetof(struct IOP_callback_entry, IOP_CB_address) == 0)
A(offsetof(struct IOP_callback_entry, IOP_CB_ref_data) == 4)
A(sizeof(struct _IOP) == 188)
A(offsetof(struct _IOP, IOP_physical) == 0)
A(offsetof(struct _IOP, IOP_physical_dcb) == 4)
A(offsetof(struct _IOP, IOP_original_dcb) == 8)
A(offsetof(struct _IOP, IOP_timer) == 12)
A(offsetof(struct _IOP, IOP_timer_orig) == 14)
A(offsetof(struct _IOP, IOP_calldown_ptr) == 16)
A(offsetof(struct _IOP, IOP_callback_ptr) == 20)
A(offsetof(struct _IOP, IOP_voltrk_private) == 24)
A(offsetof(struct _IOP, IOP_Thread_Handle) == 28)
A(offsetof(struct _IOP, IOP_srb) == 32)
A(offsetof(struct _IOP, IOP_reserved) == 36)
A(offsetof(struct _IOP, IOP_callback_table) == 44)
A(offsetof(struct _IOP, IOP_format_head) == 92)
A(offsetof(struct _IOP, IOP_format_xfer_rate) == 93)
A(offsetof(struct _IOP, IOP_format_track) == 94)
A(offsetof(struct _IOP, IOP_format_num_sectors) == 96)
A(offsetof(struct _IOP, IOP_ior) == 100)
