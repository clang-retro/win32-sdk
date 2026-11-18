/* Made by tools/gen-vxd-headers.py: the layouts of <vpicd.h> as the 98 DDK's
   compiler laid them out. */
#define VXD_WINVER 0x040A
#include <vpicd.h>
#include <stddef.h>
#ifdef __cplusplus
#define A(e) static_assert(e, #e);
#else
#define A(e) _Static_assert(e, #e);
#endif
A(sizeof(struct VPICD_IRQ_Descriptor) == 32)
A(offsetof(struct VPICD_IRQ_Descriptor, VID_IRQ_Number) == 0)
A(offsetof(struct VPICD_IRQ_Descriptor, VID_Options) == 2)
A(offsetof(struct VPICD_IRQ_Descriptor, VID_Hw_Int_Proc) == 4)
A(offsetof(struct VPICD_IRQ_Descriptor, VID_Virt_Int_Proc) == 8)
A(offsetof(struct VPICD_IRQ_Descriptor, VID_EOI_Proc) == 12)
A(offsetof(struct VPICD_IRQ_Descriptor, VID_Mask_Change_Proc) == 16)
A(offsetof(struct VPICD_IRQ_Descriptor, VID_IRET_Proc) == 20)
A(offsetof(struct VPICD_IRQ_Descriptor, VID_IRET_Time_Out) == 24)
A(offsetof(struct VPICD_IRQ_Descriptor, VID_Hw_Int_Ref) == 28)
