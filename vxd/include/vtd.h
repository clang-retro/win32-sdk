/* vtd.h: The virtual timer device: its flags and structures.

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   VXD_WINVER whose DDK has it. */
#ifndef VXD_VTD_H
#define VXD_VTD_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
#if VXD_WINVER >= 0x040A
#define VTD_DELAYEX_NEEDRESUMEEXEC 1
#endif
#define VTD_API_GET_VERSION 0x00
#define VTD_API_GET_TIME 0x0100
#define VTD_API_GET_TIME_IN_CLOCK_TICKS 0
#define VTD_API_GET_TIME_IN_MS 1
#define VTD_API_GET_VM_EXEC_TIME 2
#if VXD_WINVER >= 0x045A
#define VTD_DELAY_EX_POLL_METHOD 0x00
#define VTD_DELAY_EX_RESUME_EXEC_METHOD 0x01
#define VTD_DELAY_EX_BLOCKING_METHOD 0x02
#endif

#pragma pack(pop)

#endif
