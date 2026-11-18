/* shell.h: The SHELL device: message box flags and the
   structures of its services and its broadcast and Windows event
   hooks.

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   VXD_WINVER whose DDK has it. */
#ifndef VXD_SHELL_H
#define VXD_SHELL_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
#define SBH_CALL_ORDER_DEFAULT 0
#define SBH_CALL_ORDER_REGISTRY 0x30000000
#define SBH_CALL_ORDER_CONFIGMG 0x40000000
#if VXD_WINVER >= 0x045A
#define SBH_CALL_ORDER_PCI 0x40000001
#endif
#if VXD_WINVER >= 0x040A
#define SBH_CALL_ORDER_NTKERN 0x50000000
#define SUUAE_SYSTEM_REQUIRED_BIT 0x00
#define SUUAE_SYSTEM_REQUIRED 0x01
#define SUUAE_DISPLAY_REQUIRED_BIT 0x01
#define SUUAE_DISPLAY_REQUIRED 0x02
#define SUUAE_LOW_LATENCY_BIT 0x03
#define SUUAE_LOW_LATENCY 0x08
#define SUUAE_CONTINUOUS_CHECK_BIT 29
#define SUUAE_CONTINUOUS_CHECK 0x20000000
#define SUUAE_CONTINUOUS_ADD_BIT 30
#define SUUAE_CONTINUOUS_ADD 0x40000000
#define SUUAE_CONTINUOUS_REMOVE 0
#define SUUAE_CONTINUOUS_BIT 31
#define SUUAE_CONTINUOUS (-2147483648)
#endif
#define MB_ABORTRETRYIGNORE 0x02
#define MB_YESNOCANCEL 0x03
#define MB_RETRYCANCEL 0x05
#define MB_ICONHAND 0x10
#define MB_DEFBUTTON1 0x00
#define MB_DEFBUTTON2 0x0100
#define MB_DEFBUTTON3 0x0200
#define MB_APPLMODAL 0x00
#define MB_NOFOCUS 0x8000
#define MB_HANGSYS 0x20000000
#define MB_APPYTIME 0x10000000
#define IDABORT 3
#define IDRETRY 4
#define IDIGNORE 5
#define SE_WP_PrtScBoost 0x00100000
#define SE_WP_PrtScBoostBit 20
#define SE_WP_DispUpdBoost 0x00200000
#define SE_WP_DispUpdBoostBit 21
#define SPM_UM_DoNotWaitForCrit 0x00020000
#define SPM_UM_DoNotWaitForCritBit 17
#define SPM_UM_AlwaysSchedule 0x00080000
#define SPM_UM_AlwaysScheduleBit 19
#define SGVMI_Windowed 0x04
#define SGVMI_ALTTABdis 0x20
#define SGVMI_ALTESCdis 0x40
#define SGVMI_ALTSPACEdis 0x80
#define SGVMI_ALTENTERdis 0x0100
#define SGVMI_ALTPRTSCdis 0x0200
#define SGVMI_PRTSCdis 0x0400
#define SGVMI_CTRLESCdis 0x0800
#define SGVMI_HasHotKey 0x4000
#define SGVMI_Polling 0x1000
#define SGVMI_FastPaste 0x00020000
#define SGVMI_NoHMA 0x2000
#define SGVMI_XMS_Lock 0x00010000
#define SGVMI_EMS_Lock 0x8000
#define SGVMI_V86_Lock 0x00040000
#define SGVMI_ClsExit 0x40000000
#define LMEM_FIXED 0x00
#define LMEM_ZEROINIT 0x40
#define LPTR 0x40
#define LMEM_STRING 0x00010000
#define LMEM_OEM2ANSI 0x00020000
#define QAAFL_APPYAVAIL 0x01
#define QAAFL_APPYNOW 0x02
#define CAAFL_RING0 0x01
#define CAAFL_TIMEOUT 0x02
#define SGDAIF_COMPARENAME 0
#define SGDAIF_TESTAPPBIT 1
#define SGDAIF_LASTVALID 1
#define SGDAIR_SUCCESS 0
#define SGDAIR_NOMATCH 1
#define SGDAIR_BADFUNCTION 2
#define BSM_ERROR_SOMEONE_FAILED 0
#define BSM_ERROR_SUCCESS 1
#define BSM_ERROR_OUT_OF_MEMORY 0xFFFFFFFF
#if VXD_WINVER >= 0x040A
struct _SHELL_SUUAE_INFO {
  ULONG ssiTimeLastActiveDisplay;
  ULONG ssiTimeLastActiveSystem;
  BYTE ssiHoldDisplay;
  BYTE ssiHoldSystem;
  BYTE ssiLowLatency;
};
typedef struct _SHELL_SUUAE_INFO SHELL_SUUAE_INFO;
typedef SHELL_SUUAE_INFO *PSHELL_SUUAE_INFO;
#endif
struct _SHEXPACKET {
  DWORD dwTotalSize;
  DWORD dwSize;
  DWORD ibOp;
  DWORD ibFile;
  DWORD ibParams;
  DWORD ibDir;
  DWORD dwReserved;
  DWORD nCmdShow;
};
typedef struct _SHEXPACKET SHEXPACKET;
#if VXD_WINVER >= 0x045A
typedef struct _SHEXPACKET *PSHEXPACKET;
#endif
typedef DWORD APPY_HANDLE;
typedef void (*APPY_CALLBACK)(DWORD);
typedef DWORD SYSBHOOK_HANDLE;
typedef BOOL (*SYSBHOOK_CALLBACK)(DWORD, DWORD, DWORD, DWORD);
#if VXD_WINVER >= 0x040A
typedef void (*SHELL_MESSAGE_COMPLETION_HANDLER)();
#endif

#pragma pack(pop)

#endif
