/* Made by tools/gen-vxd-headers.py: the layouts of <med.h> as the 98 DDK's
   compiler laid them out. */
#define VXD_WINVER 0x040A
#include <med.h>
#include <stddef.h>
#ifdef __cplusplus
#define A(e) static_assert(e, #e);
#else
#define A(e) _Static_assert(e, #e);
#endif
A(sizeof(struct MED) == 20)
A(offsetof(struct MED, MED_Physical) == 0)
A(offsetof(struct MED, MED_Next) == 4)
A(offsetof(struct MED, MED_Adjacent) == 8)
A(offsetof(struct MED, MED_Type) == 12)
A(offsetof(struct MED, MED_Size) == 14)
A(offsetof(struct MED, MED_Flags) == 16)
A(offsetof(struct MED, MED_type_spec) == 18)
