/* vcache.h: VCACHE, the block cache file systems use: its buffer
   handles, flags and structures.

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   VXD_WINVER whose DDK has it. */
#ifndef VXD_VCACHE_H
#define VXD_VCACHE_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
#define VCache_Device_ID 0x048B
#define VCLF_LOCKED 0x10
#define VCL_MAXCACHENAMELEN 80
#define VCache_DEVICE_ID 0x048B
#define VCache_VERSION 0x0100
#if VXD_WINVER >= 0x040A
#define VCache_OPTION_NONE 0x00
#define VCache_OPTION_REGEX_SERVICES 0x01
#define VCache_OPTION_NEWLRU_SERVICES 0x02
#define VC_DEREG 0x00
#define VC_DISCARD 0x01
#define VC_COMPRESS 0x02
#define VCFLAG_NONE 0x00
#define VCFLAG_COMPRESS 0x01
#define VCFLAG_NOSTAT 0x02
#define VCFLAG_COMPRESS_NOTIFY 0x04
#define VCFB_None 0x00
#endif
#define VCFB_Create 0x01
#define VCFB_Hold 0x02
#define VCFB_MakeMRU 0x04
#define VCFB_LowPri 0x08
#define VCFB_MustCreate 0x10
#if VXD_WINVER >= 0x040A
#define VCFB_LRUOnly 0x10
#endif
#define VCFB_RemoveFromLRU 0x20
#if VXD_WINVER >= 0x040A
#define VCFB_Peek 0x40
#define VCFB_Tag 0x80
#define TAGBLOCK_NONE 0x00
#define TAGBLOCK_PATH 0x01
#define TAGBLOCK_FILE 0x02
#define TAGBLOCK_SRCH 0x04
#define TAGBLOCK_DIR 0x08
#define TAGBLOCK_FAT 0x10
#endif
#define VCACHE_BUFSIZE 4096
typedef unsigned long HLOOKUP;
#if VXD_WINVER >= 0x040A
struct _VCACHE_BLOCK {
  DWORD Reserved1[2];
  DWORD FSKey1;
  DWORD FSKey2;
  PVOID BufPtr;
  DWORD FSData[7];
  WORD Reserved2;
  BYTE Dirty;
};
typedef struct _VCACHE_BLOCK VCACHE_BLOCK;
typedef struct _VCACHE_BLOCK *PVCACHE_BLOCK;
#endif

#pragma pack(pop)

#endif
