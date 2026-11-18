/* vdmad.h: The virtual DMA device: DMA_* modes and flags and the
   structures of a DMA region and buffer.

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   VXD_WINVER whose DDK has it. */
#ifndef VXD_VDMAD_H
#define VXD_VDMAD_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
#define Function_Mask 0xF0
#define Channel_Mask 0x07
#define DMA_type_verify 0x00
#define DMA_type_write 0x04
#define DMA_type_read 0x08
#define DMA_AutoInit 0x10
#define DMA_AdrDec 0x20
#define DMA_demand_mode 0x00
#define DMA_single_mode 0x40
#define DMA_block_mode 0x80
#define DMA_cascade 0xC0
#define DMA_mode_mask 0xC0
#define DMA_chan_sel 0x03
#define DMA_masked 0x01
#define DMA_masked_bit 0
#define DMA_requested 0x02
#define DMA_req_bit 1
#define Programmed_IO 0x01
#define PS2_AutoInit 0x02
#define Transfer_Data 0x04
#define Write_Mem 0x08
#define _16_bit_xfer 0x40
#define _16_bit_xfer_bit 6
#define DMA_Buf_Copy 0x02
#define DMA_Buf_Copy_bit 1
#define DMA_No_Alloc_Buf 0x04
#define DMA_No_Alloc_Buf_bit 2
#define DMA_Align_64K 0x10
#define DMA_Align_64K_bit 4
#define DMA_Align_128K 0x20
#define DMA_Align_128K_bit 5
#define DMA_Get_PgTable 0x40
#define DMA_Get_PgTable_bit 6
#define DMA_Allow_NPs 0x80
#define DMA_Allow_NPs_bit 7
#define DMA_Not_Contiguous 0x01
#define DMA_Not_Aligned 0x02
#define DMA_Lock_Failed 0x03
#define DMA_No_Buffer 0x04
#define DMA_Buffer_Too_Small 0x05
#define DMA_Buffer_In_Use 0x06
#define DMA_Invalid_Region 0x07
#define DMA_Region_Not_Locked 0x08
#define DMA_Table_Too_Small 0x09
#define DMA_Invalid_Buffer 0x0A
#define DMA_Copy_Out_Range 0x0B
#define DMA_Invalid_Channel 0x0C
#define DMA_Disable_Cnt_Overflow 0x0D
#define DMA_Disable_Cnt_Underflow 0x0E
#define DMA_Func_Not_Supported 0x0F
#define DMA_NonZero_Reserved_Flags 0x10
#define DMA_SL_Get_PgTable 0x01
#define DMA_SL_Get_PgTable_bit 0
#define DMA_SL_Allow_NPs 0x02
#define DMA_SL_Allow_NPs_bit 1
#define DMA_SL_Dont_Dirty 0x04
#define DMA_SL_Dont_Dirty_bit 2
#define VDMAD_Callbacks_ReadNotify 0x01
#define VDMAD_Callbacks_AllUnmaskedWritesNotify 0x02
#define VDMAD_Callbacks_EDXhasportinfo 0x04
#define VDMAD_Callbacks_CntrlrStatus 0x08
#define VDMAD_ForcedCallbacks 0x8000
#define VDMAD_Callbacks_Valid 0x800F
#define VDMAD_Callback_ReadCount 0x01
#define VDMAD_Callback_ReadPage 0x02
#define VDMAD_Callback_ReadBase 0x03
struct DMA_Descriptor_Struc {
  ULONG DDS_size;
  ULONG DDS_linear;
  WORD DDS_seg;
  WORD DDS_bufferID;
  ULONG DDS_physical;
};
struct Extended_DDS_Struc {
  ULONG EDDS_size;
  ULONG EDDS_linear;
  WORD EDDS_seg;
  WORD EDDS_reserved;
  WORD DDS_avail;
  WORD DDS_used;
};
typedef DWORD HDMA;

#pragma pack(pop)

#endif
