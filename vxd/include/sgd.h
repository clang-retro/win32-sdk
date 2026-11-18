/* sgd.h: Scatter-gather descriptors.

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   VXD_WINVER whose DDK has it. */
#ifndef VXD_SGD_H
#define VXD_SGD_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
struct _SGD {
  ULONG SG_buff_ptr;
  ULONG SG_buff_size;
};
typedef struct _SGD SGD;
typedef struct _SGD *PSGD;

#pragma pack(pop)

#endif
