/* blockdev.h: The block device descriptor
   (BLOCKDEV and BDD) and its commands.

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   VXD_WINVER whose DDK has it. */
#ifndef VXD_BLOCKDEV_H
#define VXD_BLOCKDEV_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
#define BD_Priv_Data_Size 0x30
#define BD_Major_Ver 3
#define BD_Minor_Ver 0x0A
#define BDF_Int13_Drive 0x01
#define BDF_Int13_Drive_Bit 0
#define BDF_Writeable 0x02
#define BDF_Writeable_Bit 1
#define BDF_Removable 0x04
#define BDF_Removable_Bit 2
#define BDF_Remote 0x08
#define BDF_Remote_Bit 3
#define BDF_Serial_Cmd 0x10
#define BDF_Serial_Cmd_Bit 4
#define BDF_Cache 0x20
#define BDF_Cache_Bit 5
#define BDF_Bypass_Queue 0x0800
#define BDF_Bypass_Queue_Bit 11
#define BDF_VERSION_002 0x0400
#define BDF_VERSION_002_BIT 10
#define BDT_360K_5_Inch_Floppy 0x00
#define BDT_1200K_5_Inch_Floppy 0x01
#define BDT_720K_3_Inch_Floppy 0x02
#define BDT_Single_Dens_8_Inch 0x03
#define BDT_Double_Dens_8_Inch 0x04
#define BDT_Fixed_Disk 0x05
#define BDT_Tape_Drive 0x06
#define BDT_Other 0x07
#define BDC_Read 0
#define BDC_Write 1
#define BDC_Verify 2
#define BDC_Cancel 3
#define BDC_Lock_Volume 0x09
#define BDC_Unlock_Volume 0x0A
#define BDS_Success 0
#define BDS_Success_With_Retries 1
#define BDS_Success_With_ECC 2
#define BDS_First_Error_Code 0x10
#define BDS_Invalid_Sector_Number 0x10
#define BDS_Canceled 0x11
#define BDS_Cmd_In_Progress 0x12
#define BDS_Invalid_Cmd_Ptr 0x13
#define BDS_Media_Error 0x14
#define BDS_Device_Error 0x15
#define BDS_Invalid_Command 0x16
#define BDS_No_Device 0x17
#define BDS_No_Media 0x18
#define BDS_Uncertain_Media 0x19
#define BDS_Unrec_Error 0x1A
#define BDS_Hw_Failure 0x1B
#define BDS_Unformatted_Media 0x1C
#define BDS_Memory_Error 0x1D
#define BDS_Time_Out 0x1E
#define BDS_Write_Protect 0x1F
#define BDS_Not_Ready 0x20
#define BDS_Busy 0x21
#define BDCF_High_Priority 0x01
#define BDCF_High_Priority_Bit 0
#define BDCF_Scatter_Gather 0x02
#define BDCF_Scatter_Gather_Bit 1
#define BDCF_Dont_Cache 0x04
#define BDCF_Dont_Cache_Bit 2
#define BDCF_Bypass_Voltrk_Bit 3
#define BDCF_Bypass_Voltrk 0x08
#define BDCF_SWAPPER_IO 0x20
#define BDCF_Double_Buffer 0x40
#define BDCF_Double_Buffer_Bit 6
#define BDCF_Bypass_Queue_Bit 11
#define BDCF_Bypass_Queue 0x0800
#define BDCF_Logical_Start_Sector_Bit 19
#define BDCF_Logical_Start_Sector 0x00080000
#define BD_SC_Get_Version 0x00
#define BD_SC_Err_Invalid_Cmd 0x01
#define BlockDev_API_Hw_Detect_Start 0x01
#define BlockDev_API_Hw_Detect_End 0x02
struct BlockDev_Device_Descriptor {
  ULONG BDD_Next;
  BYTE BDD_Major_Version;
  BYTE BDD_Minor_Version;
  BYTE BDD_Device_Type;
  BYTE BDD_Int_13h_Number;
  ULONG BDD_Flags;
  ULONG BDD_Name_Ptr;
  ULONG BDD_Max_Sector[2];
  ULONG BDD_Sector_Size;
  ULONG BDD_Num_Heads;
  ULONG BDD_Num_Cylinders;
  ULONG BDD_Num_Sec_Per_Track;
  ULONG BDD_Sync_Cmd_Proc;
  ULONG BDD_Command_Proc;
  ULONG BDD_Hw_Int_Proc;
  BYTE BDD_Reserved_BlockDev[48];
};
typedef struct BlockDev_Device_Descriptor _BlockDev_Device_Descriptor;
struct BlockDev_Command_Block {
  ULONG BD_CB_Next;
  USHORT BD_CB_Command;
  USHORT BD_CB_Cmd_Status;
  ULONG BD_CB_Flags;
  ULONG BD_CB_Cmd_Cplt_Proc;
  ULONG BD_CB_Sector[2];
  ULONG BD_CB_Count;
  ULONG BD_CB_Buffer_Ptr;
  ULONG BD_CB_Reserved_Client;
  ULONG BD_CB_Reserved_BlockDev;
  ULONG BD_CB_Reserved_FastDisk;
  ULONG BD_CB_Spare[5];
  ULONG BD_CB_Req_Req_Handle;
  ULONG BD_CB_Req_Vol_Handle;
  ULONG BD_CD_SGD_Lin_Phys;
  UCHAR BD_CB_Num_SGDs;
  UCHAR BD_CB_Vol_Designtr;
  USHORT BD_CB_reserved_1;
};
typedef struct BlockDev_Command_Block _BlockDev_Command_Block;
struct BlockDev_Scatter_Gather {
  ULONG BD_SG_Count;
  ULONG BD_SG_Buffer_Ptr;
};
typedef struct BlockDev_Scatter_Gather _BlockDev_Scatter_Gather;

#pragma pack(pop)

#endif
