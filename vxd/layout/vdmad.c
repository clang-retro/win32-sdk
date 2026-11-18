/* Made by tools/gen-vxd-headers.py: the layouts of <vdmad.h> as the 98 DDK's
   compiler laid them out. */
#define VXD_WINVER 0x040A
#include <vdmad.h>
#include <stddef.h>
#ifdef __cplusplus
#define A(e) static_assert(e, #e);
#else
#define A(e) _Static_assert(e, #e);
#endif
A(sizeof(struct DMA_Descriptor_Struc) == 16)
A(offsetof(struct DMA_Descriptor_Struc, DDS_size) == 0)
A(offsetof(struct DMA_Descriptor_Struc, DDS_linear) == 4)
A(offsetof(struct DMA_Descriptor_Struc, DDS_seg) == 8)
A(offsetof(struct DMA_Descriptor_Struc, DDS_bufferID) == 10)
A(offsetof(struct DMA_Descriptor_Struc, DDS_physical) == 12)
A(sizeof(struct Extended_DDS_Struc) == 16)
A(offsetof(struct Extended_DDS_Struc, EDDS_size) == 0)
A(offsetof(struct Extended_DDS_Struc, EDDS_linear) == 4)
A(offsetof(struct Extended_DDS_Struc, EDDS_seg) == 8)
A(offsetof(struct Extended_DDS_Struc, EDDS_reserved) == 10)
A(offsetof(struct Extended_DDS_Struc, DDS_avail) == 12)
A(offsetof(struct Extended_DDS_Struc, DDS_used) == 14)
