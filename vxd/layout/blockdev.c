/* Made by tools/gen-vxd-headers.py: the layouts of <blockdev.h> as the 98 DDK's
   compiler laid them out. */
#define VXD_WINVER 0x040A
#include <blockdev.h>
#include <stddef.h>
#ifdef __cplusplus
#define A(e) static_assert(e, #e);
#else
#define A(e) _Static_assert(e, #e);
#endif
A(sizeof(struct BlockDev_Device_Descriptor) == 100)
A(offsetof(struct BlockDev_Device_Descriptor, BDD_Next) == 0)
A(offsetof(struct BlockDev_Device_Descriptor, BDD_Major_Version) == 4)
A(offsetof(struct BlockDev_Device_Descriptor, BDD_Minor_Version) == 5)
A(offsetof(struct BlockDev_Device_Descriptor, BDD_Device_Type) == 6)
A(offsetof(struct BlockDev_Device_Descriptor, BDD_Int_13h_Number) == 7)
A(offsetof(struct BlockDev_Device_Descriptor, BDD_Flags) == 8)
A(offsetof(struct BlockDev_Device_Descriptor, BDD_Name_Ptr) == 12)
A(offsetof(struct BlockDev_Device_Descriptor, BDD_Max_Sector) == 16)
A(offsetof(struct BlockDev_Device_Descriptor, BDD_Sector_Size) == 24)
A(offsetof(struct BlockDev_Device_Descriptor, BDD_Num_Heads) == 28)
A(offsetof(struct BlockDev_Device_Descriptor, BDD_Num_Cylinders) == 32)
A(offsetof(struct BlockDev_Device_Descriptor, BDD_Num_Sec_Per_Track) == 36)
A(offsetof(struct BlockDev_Device_Descriptor, BDD_Sync_Cmd_Proc) == 40)
A(offsetof(struct BlockDev_Device_Descriptor, BDD_Command_Proc) == 44)
A(offsetof(struct BlockDev_Device_Descriptor, BDD_Hw_Int_Proc) == 48)
A(offsetof(struct BlockDev_Device_Descriptor, BDD_Reserved_BlockDev) == 52)
A(sizeof(struct BlockDev_Command_Block) == 80)
A(offsetof(struct BlockDev_Command_Block, BD_CB_Next) == 0)
A(offsetof(struct BlockDev_Command_Block, BD_CB_Command) == 4)
A(offsetof(struct BlockDev_Command_Block, BD_CB_Cmd_Status) == 6)
A(offsetof(struct BlockDev_Command_Block, BD_CB_Flags) == 8)
A(offsetof(struct BlockDev_Command_Block, BD_CB_Cmd_Cplt_Proc) == 12)
A(offsetof(struct BlockDev_Command_Block, BD_CB_Sector) == 16)
A(offsetof(struct BlockDev_Command_Block, BD_CB_Count) == 24)
A(offsetof(struct BlockDev_Command_Block, BD_CB_Buffer_Ptr) == 28)
A(offsetof(struct BlockDev_Command_Block, BD_CB_Reserved_Client) == 32)
A(offsetof(struct BlockDev_Command_Block, BD_CB_Reserved_BlockDev) == 36)
A(offsetof(struct BlockDev_Command_Block, BD_CB_Reserved_FastDisk) == 40)
A(offsetof(struct BlockDev_Command_Block, BD_CB_Spare) == 44)
A(offsetof(struct BlockDev_Command_Block, BD_CB_Req_Req_Handle) == 64)
A(offsetof(struct BlockDev_Command_Block, BD_CB_Req_Vol_Handle) == 68)
A(offsetof(struct BlockDev_Command_Block, BD_CD_SGD_Lin_Phys) == 72)
A(offsetof(struct BlockDev_Command_Block, BD_CB_Num_SGDs) == 76)
A(offsetof(struct BlockDev_Command_Block, BD_CB_Vol_Designtr) == 77)
A(offsetof(struct BlockDev_Command_Block, BD_CB_reserved_1) == 78)
A(sizeof(struct BlockDev_Scatter_Gather) == 8)
A(offsetof(struct BlockDev_Scatter_Gather, BD_SG_Count) == 0)
A(offsetof(struct BlockDev_Scatter_Gather, BD_SG_Buffer_Ptr) == 4)
