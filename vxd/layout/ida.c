/* Made by tools/gen-vxd-headers.py: the layouts of <ida.h> as the 98 DDK's
   compiler laid them out. */
#define VXD_WINVER 0x040A
#include <ida.h>
#include <stddef.h>
#ifdef __cplusplus
#define A(e) static_assert(e, #e);
#else
#define A(e) _Static_assert(e, #e);
#endif
A(sizeof(struct IDA) == 110)
A(offsetof(struct IDA, IDA_first_ice) == 0)
A(offsetof(struct IDA, IDA_current_ice) == 4)
A(offsetof(struct IDA, IDA_ios_mem_phys) == 8)
A(offsetof(struct IDA, IDA_current_ice_len) == 12)
A(offsetof(struct IDA, IDA_ios_timer) == 16)
A(offsetof(struct IDA, IDA_ios_counter) == 20)
A(offsetof(struct IDA, IDA_start_eci) == 24)
A(offsetof(struct IDA, IDA_start_cct) == 28)
A(offsetof(struct IDA, IDA_esdi_p_in_use) == 32)
A(offsetof(struct IDA, IDA_esdi_s_in_use) == 33)
A(offsetof(struct IDA, IDA_Num_Floppies) == 34)
A(offsetof(struct IDA, PAD0) == 35)
A(offsetof(struct IDA, IDA_platform) == 36)
A(offsetof(struct IDA, PAD1) == 38)
A(offsetof(struct IDA, IDA_flags) == 40)
A(offsetof(struct IDA, IDA_platform_config_data) == 44)
A(offsetof(struct IDA, IDA_first_dcb) == 48)
A(offsetof(struct IDA, IDA_logical_dev_map) == 52)
A(offsetof(struct IDA, IDA_physical_dev_map) == 56)
A(offsetof(struct IDA, IDA_rmm_phys_dcb) == 60)
A(offsetof(struct IDA, IDA_first_vrp) == 64)
A(offsetof(struct IDA, IDA_drive_letter_map) == 68)
A(offsetof(struct IDA, IDA_cdrom_letter_map) == 72)
A(offsetof(struct IDA, IDA_rmd_ptr) == 76)
A(offsetof(struct IDA, IDA_aspi_cam_rmds) == 80)
A(offsetof(struct IDA, IDA_big_mem_buf_virt) == 84)
A(offsetof(struct IDA, IDA_big_mem_buf_phys) == 88)
A(offsetof(struct IDA, IDA_big_mem_buf_size) == 92)
A(offsetof(struct IDA, IDA_ios_mem_pool_size) == 96)
A(offsetof(struct IDA, IDA_dbl_vol_map) == 100)
A(offsetof(struct IDA, IDA_ontrack_data) == 104)
A(offsetof(struct IDA, IDA_rm_irq) == 108)
