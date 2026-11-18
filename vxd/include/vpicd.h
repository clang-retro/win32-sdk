/* vpicd.h: The virtualized interrupt controller: the VPICD_* flags
   and the structures for virtualizing an IRQ.

   Made by tools/gen-vxd-headers.py from the names, values and layouts
   in the 95, 98 and ME DDKs; each declaration is under the first
   VXD_WINVER whose DDK has it. */
#ifndef VXD_VPICD_H
#define VXD_VPICD_H

#include <vxd.h>
#include <vxdtypes.h>

#pragma pack(push, 1)
#define VPICD_STAT_IRET_PENDING 0x01
#define VPICD_STAT_IRET_PENDING_BIT 0
#define VPICD_STAT_IN_SERVICE 0x02
#define VPICD_STAT_IN_SERVICE_BIT 1
#define VPICD_STAT_PHYS_MASK 0x04
#define VPICD_STAT_PHYS_MASK_BIT 2
#define VPICD_STAT_PHYS_IN_SERV 0x08
#define VPICD_STAT_PHYS_IN_SERV_BIT 3
#define VPICD_STAT_VIRT_MASK 0x10
#define VPICD_STAT_VIRT_MASK_BIT 4
#define VPICD_STAT_VIRT_REQ 0x20
#define VPICD_STAT_VIRT_REQ_BIT 5
#define VPICD_STAT_PHYS_REQ 0x40
#define VPICD_STAT_PHYS_REQ_BIT 6
#define VPICD_STAT_VIRT_DEV_REQ 0x80
#define VPICD_STAT_VIRT_DEV_REQ_BIT 7
#define VPICD_STAT_GLOBAL 0x0100
#define VPICD_STAT_GLOBAL_BIT 8
#define VPICD_STAT_OWNED_BY_VM 0x0200
#define VPICD_STAT_OWNED_BY_VM_BIT 9
#define VPICD_OPT_READ_HW_IRR_BIT 0
#define VPICD_OPT_CAN_SHARE_BIT 1
#define VPICD_OPT_REF_DATA_BIT 2
#define VPICD_OPT_VIRT_INT_REJECT 0x10
#define VPICD_OPT_VIRT_INT_REJECT_BIT 4
#define VPICD_OPT_SHARE_PMODE_ONLY 0x20
#define VPICD_OPT_SHARE_PMODE_ONLY_BIT 5
#if VXD_WINVER >= 0x040A
#define VPICD_OPT_SUPPORT_EDGE_SHARE_BIT 8
#define VPICD_OPT_SUPPORT_EDGE_SHARE 0x0100
#define VPICD_OPT_SORTABLE_BIT 9
#define VPICD_OPT_SORTABLE 0x0200
#endif
#if VXD_WINVER >= 0x040A
#define VPICD_OPT_ALL 0x033F
#else
#define VPICD_OPT_ALL 0x3F
#endif
#define VPICD_FDO_NO_CONTENTION 0x00010000
#define VPICD_FDO_NO_CONTENTION_BIT 16
#define VPICD_FDO_FAVOR_FOCUS 0x00020000
#define VPICD_FDO_FAVOR_FOCUS_BIT 17
#if VXD_WINVER >= 0x040A
#define NUM_IRQL_LEVELS 32
#define VPICD_TRIGGER_HANDLER_GET 0x00
#define VPICD_TRIGGER_HANDLER_SET 0x01
#endif
typedef struct VPICD_IRQ_Descriptor VID;
typedef VID *PVID;
typedef ULONG HIRQ;
#if VXD_WINVER >= 0x040A
typedef ULONG (*VPICDTRIGGERHANDLER)(ULONG, ULONG);
#endif

#pragma pack(pop)

#endif
