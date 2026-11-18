/* ios.h: The I/O supervisor (IOS): the layers of
   its calldown chain, the DCB a port driver makes for each device and
   the flags of both.

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   VXD_WINVER whose DDK has it. */
#ifndef VXD_IOS_H
#define VXD_IOS_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
#define IOS_Device_ID 0x10
#define IOS_Major_Ver 0x03
#define IOS_Minor_Ver 0x0A
#define IOSEA_LOCK_THREAD 0x0100

#pragma pack(pop)

#endif
