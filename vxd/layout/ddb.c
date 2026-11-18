/* Made by tools/gen-vxd-headers.py: the layouts of <ddb.h> as the 98 DDK's
   compiler laid them out. */
#define VXD_WINVER 0x040A
#include <ddb.h>
#include <stddef.h>
#ifdef __cplusplus
#define A(e) static_assert(e, #e);
#else
#define A(e) _Static_assert(e, #e);
#endif
A(sizeof(struct DDB) == 32)
A(offsetof(struct DDB, DDB_phys_addr) == 0)
A(offsetof(struct DDB, DDB_Next_DDB) == 4)
A(offsetof(struct DDB, DDB_Next_DDB_init) == 8)
A(offsetof(struct DDB, DDB_dcb_ptr) == 12)
A(offsetof(struct DDB, DDB_number_buses) == 16)
A(offsetof(struct DDB, DDB_ios_flags) == 17)
A(offsetof(struct DDB, DDB_sig) == 18)
A(offsetof(struct DDB, DDB_dvt) == 20)
A(offsetof(struct DDB, DDB_devnode_ptr) == 24)
A(offsetof(struct DDB, DDB_pAcpiBlock) == 28)
