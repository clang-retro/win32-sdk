/* Made by tools/gen-vxd-headers.py: the layouts of <ior.h> as the 98 DDK's
   compiler laid them out. */
#define VXD_WINVER 0x040A
#include <ior.h>
#include <stddef.h>
#ifdef __cplusplus
#define A(e) static_assert(e, #e);
#else
#define A(e) _Static_assert(e, #e);
#endif
A(sizeof(struct type_sdeffsd_req_usage) == 20)
A(offsetof(struct type_sdeffsd_req_usage, _IOR_ioctl_drive) == 0)
A(offsetof(struct type_sdeffsd_req_usage, _IOR_ioctl_function) == 2)
A(offsetof(struct type_sdeffsd_req_usage, _IOR_ioctl_control_param) == 4)
A(offsetof(struct type_sdeffsd_req_usage, _IOR_ioctl_buffer_ptr) == 8)
A(offsetof(struct type_sdeffsd_req_usage, _IOR_ioctl_client_params) == 12)
A(offsetof(struct type_sdeffsd_req_usage, _IOR_ioctl_return) == 16)
A(sizeof(union urequestor_usage) == 20)
A(offsetof(union urequestor_usage, _IOR_requestor_usage) == 0)
A(offsetof(union urequestor_usage, sdeffsd_req_usage) == 0)
A(sizeof(struct _IOR) == 88)
A(offsetof(struct _IOR, IOR_next) == 0)
A(offsetof(struct _IOR, IOR_func) == 4)
A(offsetof(struct _IOR, IOR_status) == 6)
A(offsetof(struct _IOR, IOR_flags) == 8)
A(offsetof(struct _IOR, IOR_callback) == 12)
A(offsetof(struct _IOR, IOR_start_addr) == 16)
A(offsetof(struct _IOR, IOR_xfer_count) == 24)
A(offsetof(struct _IOR, IOR_buffer_ptr) == 28)
A(offsetof(struct _IOR, IOR_private_client) == 32)
A(offsetof(struct _IOR, IOR_private_IOS) == 36)
A(offsetof(struct _IOR, IOR_private_port) == 40)
A(offsetof(struct _IOR, _ureq) == 44)
A(offsetof(struct _IOR, IOR_req_req_handle) == 64)
A(offsetof(struct _IOR, IOR_req_vol_handle) == 68)
A(offsetof(struct _IOR, IOR_sgd_lin_phys) == 72)
A(offsetof(struct _IOR, IOR_num_sgds) == 76)
A(offsetof(struct _IOR, IOR_vol_designtr) == 77)
A(offsetof(struct _IOR, IOR_ios_private_1) == 78)
A(offsetof(struct _IOR, IOR_reserved_2) == 80)
