/* mmdevldr.h: MMDEVLDR, the loader of the VxDs and
   16-bit drivers of sound cards and other multimedia devices: its device
   ID and version. The services (MMDEVLDR_Register_Device_Driver, which a
   sound card's VxD calls for its devnode, _MMDEVLDR_SetDevicePresence
   and the rest) are in <vxdsvc.h>.

   Made by tools/gen-vxd-inc.py from the names and values in the 95, 98
   and ME DDKs' mmdevldr.inc; each is under the first VXD_WINVER whose DDK
   has it. */
#ifndef VXD_MMDEVLDR_H
#define VXD_MMDEVLDR_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
#define Multimedia_OEM_ID 0x440
#define MMDEVLDR_Device_ID 0x44a
#define MMDEVLDR_Ver_Major 1
#define MMDEVLDR_Ver_Minor 0

#pragma pack(pop)

#endif
