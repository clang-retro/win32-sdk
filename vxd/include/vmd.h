/* vmd.h: The virtual mouse device (VMOUSE): the mouse types
   (VMD_Type_*), the VMDAPI_* functions, the Mouse_Instance a mini mouse
   driver fills in and its MIF_* flags, and the cursor drawing VxD
   interface (the cursor procedure's functions, DISPLAYMODEINFO, the text
   and graphics cursors and Int10Notify). The services are VMD_* in
   <vxdsvc.h>.

   Made by tools/gen-vxd-inc.py from the names and values in the 95, 98
   and ME DDKs' vmd.inc; each is under the first VXD_WINVER whose DDK
   has it. */
#ifndef VXD_VMD_H
#define VXD_VMD_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
#define VMD_Major_Version 4
#define VMD_Minor_Version 0
#define VMDINT33_Major_Version 8
#define VMDINT33_Minor_Version 0x30
#define VMDAPI_GET_VERSION 0
#define VMDAPI_UPDATE_MOUSE_STATE 1
#define VMDAPI_QUERY_WINDOWED_MOUSE_SUPPORT 2
#define VMDAPI_SET_MOUSE_EVENT_CALLBACK 3
#define VMDAPI_SET_MOUSE_FOCUS 4
#define VMDAPI_GET_MOUSE_INFO 5
#define VMDAPI_SET_MOUSE_TYPE 0x100
#define VMDAPI_DETECT_MOUSE 0x101
#define VMDAPI_GET_ADDON 0x102
#define VMDAPI_MOUSEDRV_CONSISTENT 0x103
#define VMD_Type_Undefined 0
#define VMD_Type_Bus 1
#define VMD_Type_Serial 2
#define VMD_Type_InPort 3
#define VMD_Type_PS2 4
#define VMD_Type_HP 5
#define VMD_Type_Serial_w_port 0x82
#define VMD_Type_MSeries 6
#define VMD_Type_CSeries 7
struct Mouse_Instance {
  WORD MI_Flags;
  WORD MI_IO_Base;
  BYTE MI_IRQNumber;
  BYTE MI_PortNum;
  WORD MI_MouseType;
  DWORD MI_Reference;
  DWORD MI_hDevNode;
  DWORD MI_hIRQ;
  DWORD MI_HWINT;
  DWORD MI_Reset;
  DWORD MI_Disable;
  DWORD MI_Initialize;
  DWORD MI_LoadHandle;
};
typedef struct Mouse_Instance Mouse_Instance;
#define MIF_Alloc_Bit 0
#define MIF_Alloc 1
#define MIF_Active_Bit 1
#define MIF_Active 2
#define MIF_Detected_Bit 2
#define MIF_Detected 4
#define MIF_BadDevNode_Bit 3
#define MIF_BadDevNode 8
#define MIF_NewConfig_Bit 4
#define MIF_NewConfig 0x10
#define MIF_BadIO_Bit 5
#define MIF_BadIO 0x20
#define MIF_BadIRQ_Bit 6
#define MIF_BadIRQ 0x40
#define MIF_NoIRQ_Bit 7
#define MIF_NoIRQ 0x80
#define MIF_BUSMOUSE_Bit 8
#define MIF_BUSMOUSE 0x100
#define MIF_INPMOUSE_Bit 9
#define MIF_INPMOUSE 0x200
#define MIF_AUXMOUSE_Bit 0xa
#define MIF_AUXMOUSE 0x400
#define MIF_OTHERMOUSE_Bit 0xb
#define MIF_OTHERMOUSE 0x800
struct Point_Struc {
  WORD PS_Y;
  WORD PS_X;
};
typedef struct Point_Struc Point_Struc;
struct DISPLAYMODEINFO {
  WORD wSize;
  WORD wFlags;
  BYTE DMI_Virtual_Cursor_Maximum[4];
  BYTE XChar;
  BYTE YChar;
  BYTE DMI_Cell_Dimensions[4];
  BYTE cBitsPerPixel;
  WORD wPageSize;
  BYTE MaxBlocks;
  BYTE XCursorMax;
  BYTE YCursorMax;
  BYTE bMaxTrail;
  BYTE cDrawPages;
};
typedef struct DISPLAYMODEINFO DISPLAYMODEINFO;
#define MI_GRAPHICSCURSOR 1
#define MI_COLORCURSOR 2
#define MI_NOCURSOR 4
struct CursorProcInfo {
  DWORD CursorProcAddress;
  DWORD NextCursorProcInfo;
  WORD Video_Mode_Low;
  WORD Video_Mode_High;
  BYTE Priority;
  WORD Flags;
};
typedef struct CursorProcInfo CursorProcInfo;
#define DC_SupportsOverride 1
struct TEXTCURSOR {
  WORD cbSize;
  DWORD dwOffset32Blocks;
  BYTE bFlags;
  BYTE cBlocks;
  WORD XORMask;
  WORD ANDMask;
};
typedef struct TEXTCURSOR TEXTCURSOR;
#define BF_HARDWARE 1
struct GRAPHICSCURSOR {
  WORD cbSize;
  DWORD dwOffset32Masks;
  WORD wWidth;
  WORD wHeight;
  WORD wHotX;
  WORD wHotY;
  WORD wType;
};
typedef struct GRAPHICSCURSOR GRAPHICSCURSOR;
struct Int10Notify {
  BYTE FunctionNumber;
  DWORD NextNotifyStruc;
};
typedef struct Int10Notify Int10Notify;
#define SETTEXTCURSOR 0
#define SETGRAPHICSCURSOR 1
#define MOVECURSOR 2
#define REMOVECURSOR 3
#define SETDISPLAYMODE 4
#define SETDISPLAYPAGE 5
#define SAVERESTORESTATE 6
#define GETDISPLAYMODEINFO 7
#define QUERYDISPLAYMODE 8
#define SETCONDITIONALREGION 9
#define QUERYMEMORYUNDERCURSOR 0xa
#define SETCURSORTRAIL 0xb
#define RESETCURSORDRIVER 0xc
#define DEFAULT_AND_MASK 0x77ff
#define DEFAULT_XOR_MASK 0x7700
#define TEXT_SMALL_BLOCK_SIZE 1
#define TEXT_BYTES_PER_BLOCK 6
#define TEXT_SMALL_MASK_SIZE 6
#define GR_SMALL_WIDTH 0x10
#define GR_SMALL_HEIGHT 0x10
#define GR_SMALL_MASK_SIZE 0x20
struct SMALLTEXTCURSOR {
  struct TEXTCURSOR tc;
  BYTE Blocks[6];
};
typedef struct SMALLTEXTCURSOR SMALLTEXTCURSOR;
struct SMALLGRAPHICSCURSOR {
  struct GRAPHICSCURSOR gc;
  BYTE Masks[64];
};
typedef struct SMALLGRAPHICSCURSOR SMALLGRAPHICSCURSOR;
union SMALLCURSOR {
  struct SMALLGRAPHICSCURSOR smgc;
  struct SMALLTEXTCURSOR smtc;
};
typedef union SMALLCURSOR SMALLCURSOR;

#pragma pack(pop)

#endif
