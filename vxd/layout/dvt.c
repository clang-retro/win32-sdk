/* Made by tools/gen-vxd-headers.py: the layouts of <dvt.h> as the 98 DDK's
   compiler laid them out. */
#define VXD_WINVER 0x040A
#include <dvt.h>
#include <stddef.h>
#ifdef __cplusplus
#define A(e) static_assert(e, #e);
#else
#define A(e) _Static_assert(e, #e);
#endif
A(sizeof(struct DVT) == 88)
A(offsetof(struct DVT, DVT_reserved_1) == 0)
A(offsetof(struct DVT, DVT_next_dvt) == 2)
A(offsetof(struct DVT, DVT_device_cnt) == 6)
A(offsetof(struct DVT, DVT_aer) == 8)
A(offsetof(struct DVT, DVT_ddb) == 12)
A(offsetof(struct DVT, DVT_ddb_init) == 16)
A(offsetof(struct DVT, DVT_ascii_name) == 20)
A(offsetof(struct DVT, DVT_create_date) == 36)
A(offsetof(struct DVT, DVT_create_time) == 44)
A(offsetof(struct DVT, DVT_rev_level) == 52)
A(offsetof(struct DVT, DVT_feature_code) == 56)
A(offsetof(struct DVT, DVT_if_requirements) == 60)
A(offsetof(struct DVT, DVT_bus_type) == 62)
A(offsetof(struct DVT, DVT_reference_data) == 63)
A(offsetof(struct DVT, DVT_first_drive) == 67)
A(offsetof(struct DVT, DVT_current_lgn) == 68)
A(offsetof(struct DVT, DVT_LoadHandle) == 69)
A(offsetof(struct DVT, DVT_scsi_max_target) == 73)
A(offsetof(struct DVT, DVT_scsi_max_lun) == 74)
A(offsetof(struct DVT, DVT_entry_point) == 75)
A(offsetof(struct DVT, DVT_init_count) == 79)
A(offsetof(struct DVT, DVT_reserved) == 80)
