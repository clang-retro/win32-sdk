/* Made by tools/gen-vxd-headers.py: the layouts of <vmmreg.h> as the 98 DDK's
   compiler laid them out. */
#define VXD_WINVER 0x040A
#include <vmmreg.h>
#include <stddef.h>
#ifdef __cplusplus
#define A(e) static_assert(e, #e);
#else
#define A(e) _Static_assert(e, #e);
#endif
A(sizeof(struct val_context) == 12)
A(offsetof(struct val_context, valuelen) == 0)
A(offsetof(struct val_context, value_context) == 4)
A(offsetof(struct val_context, val_buff_ptr) == 8)
A(sizeof(struct pvalue) == 16)
A(offsetof(struct pvalue, pv_valuename) == 0)
A(offsetof(struct pvalue, pv_valuelen) == 4)
A(offsetof(struct pvalue, pv_value_context) == 8)
A(offsetof(struct pvalue, pv_type) == 12)
A(sizeof(struct provider_info) == 20)
A(offsetof(struct provider_info, pi_R0_1val) == 0)
A(offsetof(struct provider_info, pi_R0_allvals) == 4)
A(offsetof(struct provider_info, pi_R3_1val) == 8)
A(offsetof(struct provider_info, pi_R3_allvals) == 12)
A(offsetof(struct provider_info, pi_flags) == 16)
A(sizeof(struct value_ent) == 16)
A(offsetof(struct value_ent, ve_valuename) == 0)
A(offsetof(struct value_ent, ve_valuelen) == 4)
A(offsetof(struct value_ent, ve_valueptr) == 8)
A(offsetof(struct value_ent, ve_type) == 12)
