/* vkd.h: The virtual keyboard device (VKD): the shift state
   bits (SS_*), the hot key flags and notifications VKD_Define_Hot_Key
   takes and calls back with (CallOn*, Hot_Key_*, HKSS_*), the paste
   buffer's Paste_Rec and completion codes, and the VKD_* protected mode
   API's functions and FKSS_* shift states. The services are VKD_* in
   <vxdsvc.h>.

   Made by tools/gen-vxd-inc.py from the names and values in the 95, 98
   and ME DDKs' vkd.inc; each is under the first VXD_WINVER whose DDK
   has it. */
#ifndef VXD_VKD_H
#define VXD_VKD_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
#if VXD_WINVER < 0x040A
#define VKD_Version 0x20a
#else
#define VKD_Version 0x300
#endif
#define SS_Shift 1
#define SS_LShift 2
#define SS_RShift 0x200
#define SS_Ctrl 0x80
#define SS_LCtrl 4
#define SS_RCtrl 0x400
#define SS_Alt 0x100
#define SS_LAlt 8
#define SS_RAlt 0x800
#define SS_CapLock 0x40
#define SS_NumLock 0x20
#define SS_ScrlLock 0x10
#define SS_CapLock_Dn 0x4000
#define SS_NumLock_Dn 0x2000
#define SS_ScrlLock_Dn 0x1000
#define SS_Win 0x8000
#define SS_Unused 0
#define SS_Toggle_mask 0x70
#define SS_Toggle_Dn_mask 0x7000
#define SS_LShift_mask 0xe
#define SS_Shift_mask 0xe0e
#define SS_Either_Shift 0x202
#define SS_Either_Ctrl 0x404
#define SS_Either_Alt 0x808
#define LED_ScLok 1
#define LED_NmLok 2
#define LED_CpLok 4
#define Toggle_To_LED 4
#define ExtendedKey_B 1
#define ExtendedKey 0x100
#define AllowExtended_B 0xff
#define AllowExtended 0xff00
#define CallOnPress 1
#define CallOnRelease 2
#define CallOnRepeat 4
#define CallOnComplete 8
#define CallOnUpDwn 3
#define CallOnAll 0
#define PriorityNotify 0x10
#define Monitor_Key 0x40
#define Local_Key 0x80
#define Hot_Key_Pressed 0
#define Hot_Key_Released 1
#define Hot_Key_Repeated 2
#define Hot_Key_Completed 3
#define Hot_Key_Monitor 0x40
#define Hot_Key_MonitorBit 6
#define Hot_Key_SysVM_Notify 0x80
#define Hot_Key_SysVM_Notify_bit 7
#define HKSS_Shift 0xfd8d0001
#define HKSS_Ctrl 0xfb8b0080
#define HKSS_Alt 0xf7870100
struct Paste_Rec {
  BYTE Paste_Char;
  BYTE Paste_ScanCode;
  WORD Paste_ShiftState;
};
typedef struct Paste_Rec Paste_Rec;
#define VKD_API_Version 0
#define VKD_PM_API_Version_Major 2
#define VKD_PM_API_Version_Minor 1
#define VKD_PM_API_Version 0x201
#define VKD_Force_Key 1
#define FKSS_Insert_Down 0x8000
#define FKSS_CapLck_Down 0x4000
#define FKSS_NumLck_Down 0x2000
#define FKSS_ScrLck_Down 0x1000
#define FKSS_Insert_On 0x80
#define FKSS_CapLck_On 0x40
#define FKSS_NumLck_On 0x20
#define FKSS_ScrLck_On 0x10
#define FKSS_Alt_Down 8
#define FKSS_Ctrl_Down 4
#define FKSS_Shift_Down 2
#define FKSS_Extended_Key 1
#define Paste_Complete 0
#define Paste_Aborted 1
#define Paste_VM_Term 2
#define MK_Shift 2
#define MK_Ctrl 4
#define MK_Alt 8
#define MK_Extended 0x80

#pragma pack(pop)

#endif
