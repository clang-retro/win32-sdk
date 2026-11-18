/* Made by tools/gen-vxd-headers.py: the layouts of <drp.h> as the 98 DDK's
   compiler laid them out. */
#define VXD_WINVER 0x040A
#include <drp.h>
#include <stddef.h>
#ifdef __cplusplus
#define A(e) static_assert(e, #e);
#else
#define A(e) _Static_assert(e, #e);
#endif
A(sizeof(struct DRP) == 56)
A(offsetof(struct DRP, DRP_eyecatch_str) == 0)
A(offsetof(struct DRP, DRP_LGN) == 8)
A(offsetof(struct DRP, DRP_aer) == 12)
A(offsetof(struct DRP, DRP_ilb) == 16)
A(offsetof(struct DRP, DRP_ascii_name) == 20)
A(offsetof(struct DRP, DRP_revision) == 36)
A(offsetof(struct DRP, DRP_feature_code) == 37)
A(offsetof(struct DRP, DRP_if_requirements) == 41)
A(offsetof(struct DRP, DRP_bus_type) == 43)
A(offsetof(struct DRP, DRP_reg_result) == 44)
A(offsetof(struct DRP, DRP_reference_data) == 46)
A(offsetof(struct DRP, DRP_reserved1) == 50)
A(offsetof(struct DRP, DRP_reserved2) == 52)
