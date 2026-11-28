/* dxmini.h: kernel-mode DirectDraw in a video miniport driver, for
 * Windows 2000 and later: the DXAPI interface that DXAPI.SYS gets from
 * the miniport with IRP_MN_QUERY_INTERFACE (GUID_DxApi), and the
 * structures of its functions.
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 * Copyright (C) clang-retro contributors.
 *
 * Written for clang-retro from Microsoft's published documentation of
 * the dxmini.h interface. No text of a Microsoft header is used.
 *
 * A miniport that supports kernel-mode DirectDraw (video ports, overlay
 * flips at interrupt time, bus-master transfers) fills in a
 * DXAPI_INTERFACE in its HwQueryInterface function. Each function gets
 * the miniport's device extension, an input structure and an output
 * structure, and returns DX_OK or a DXERR_* code. */
#ifndef WDM_DXMINI_H
#define WDM_DXMINI_H

#include <ntdef.h>
#include <guiddef.h>

/* A miniport has the NT types, not the Win32 ones that this interface
 * uses. They are the same types as in <windef.h>. */
#ifndef _WINDEF_
typedef unsigned long DWORD;
typedef int BOOL;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* The interface that DXAPI.SYS asks for. */
DEFINE_GUID(GUID_DxApi, 0x8a79bef0, 0xb915, 0x11d0, 0x91, 0x44, 0x08,
            0x00, 0x36, 0xd2, 0xef, 0x02);

/* DXAPI_INTERFACE.Version. */
#define DXAPI_HALVERSION 0x0001

/* The return values of the functions. */
#define DX_OK 0x0
#define DXERR_UNSUPPORTED 0x80004001L
#define DXERR_GENERIC 0x80004005L
#define DXERR_OUTOFCAPS 0x88760168L

/* The surface and the video port, as DXAPI.SYS describes them. */
typedef struct _DDSURFACEDATA {
  DWORD ddsCaps;             /* DDSCAPS_* of the surface */
  DWORD dwSurfaceOffset;     /* offset in the frame buffer */
  ULONG_PTR fpLockPtr;       /* the surface's address when locked */
  DWORD dwWidth;
  DWORD dwHeight;
  LONG lPitch;
  DWORD dwOverlayFlags;      /* DDOVER_* */
  DWORD dwOverlayOffset;
  DWORD dwOverlaySrcWidth;
  DWORD dwOverlaySrcHeight;
  DWORD dwOverlayDestWidth;
  DWORD dwOverlayDestHeight;
  DWORD dwVideoPortId;       /* the video port that writes it */
  ULONG dwFormatFlags;       /* DDPF_* of the pixel format */
  ULONG dwFormatFourCC;
  ULONG dwFormatBitCount;
  ULONG dwRBitMask;
  ULONG dwGBitMask;
  ULONG dwBBitMask;
  ULONG dwDriverReserved1;
  ULONG dwDriverReserved2;
  ULONG dwDriverReserved3;
  ULONG dwDriverReserved4;
} DDSURFACEDATA, *PDDSURFACEDATA;

typedef struct DDVIDEOPORTDATA {
  DWORD dwVideoPortId;
  DWORD dwVPFlags;           /* DDVP_* */
  DWORD dwOriginOffset;      /* offset of the video in the surface */
  DWORD dwHeight;
  DWORD dwVBIHeight;         /* lines of vertical blanking data */
  DWORD dwDriverReserved1;
  DWORD dwDriverReserved2;
  DWORD dwDriverReserved3;
} DDVIDEOPORTDATA, *PDDVIDEOPORTDATA;

/* The interrupts. DXAPI.SYS registers a callback with DxEnableIrq, and
 * the miniport's interrupt routine calls it with the sources that
 * fired. */
#define DDIRQ_DISPLAY_VSYNC 0x00000001
#define DDIRQ_VPORT0_VSYNC 0x00000004
#define DDIRQ_VPORT0_LINE 0x00000008
#define DDIRQ_VPORT1_VSYNC 0x00000010
#define DDIRQ_VPORT1_LINE 0x00000020
#define DDIRQ_VPORT2_VSYNC 0x00000040
#define DDIRQ_VPORT2_LINE 0x00000080
#define DDIRQ_VPORT3_VSYNC 0x00000100
#define DDIRQ_VPORT3_LINE 0x00000200
#define DDIRQ_VPORT4_VSYNC 0x00000400
#define DDIRQ_VPORT4_LINE 0x00000800
#define DDIRQ_VPORT5_VSYNC 0x00001000
#define DDIRQ_VPORT5_LINE 0x00002000
#define DDIRQ_VPORT6_VSYNC 0x00004000
#define DDIRQ_VPORT6_LINE 0x00008000
#define DDIRQ_VPORT7_VSYNC 0x00010000
#define DDIRQ_VPORT7_LINE 0x00020000
#define DDIRQ_VPORT8_VSYNC 0x00040000
#define DDIRQ_VPORT8_LINE 0x00080000
#define DDIRQ_VPORT9_VSYNC 0x00010000
#define DDIRQ_VPORT9_LINE 0x00020000
#define DDIRQ_BUSMASTER 0x00000002

typedef struct _DX_IRQDATA {
  DWORD dwIrqFlags;          /* the DDIRQ_* sources that fired */
} DX_IRQDATA, *PDX_IRQDATA;
typedef void (*PDX_IRQCALLBACK)(PDX_IRQDATA pIrqData);

/* DDGETIRQINFO.dwFlags. */
#define IRQINFO_HANDLED 0x01    /* the miniport handles the interrupts */
#define IRQINFO_NOTHANDLED 0x02 /* it does not */

typedef struct _DDGETIRQINFO {
  DWORD dwFlags;             /* IRQINFO_* */
} DDGETIRQINFO, *PDDGETIRQINFO;

typedef struct _DDENABLEIRQINFO {
  DWORD dwIRQSources;        /* the DDIRQ_* sources to enable */
  DWORD dwLine;              /* the line for the DDIRQ_VPORT*_LINE ones */
  PDX_IRQCALLBACK IRQCallback;
  PDX_IRQDATA lpIRQData;
} DDENABLEIRQINFO, *PDDENABLEIRQINFO;

/* Skip or bob a video port field. */
#define DDSKIP_SKIPNEXT 1
#define DDSKIP_ENABLENEXT 2
typedef struct _DDSKIPNEXTFIELDINFO {
  PDDVIDEOPORTDATA lpVideoPortData;
  DWORD dwSkipFlags;         /* DDSKIP_* */
} DDSKIPNEXTFIELDINFO, *PDDSKIPNEXTFIELDINFO;

typedef struct _DDBOBNEXTFIELDINFO {
  PDDSURFACEDATA lpSurface;
} DDBOBNEXTFIELDINFO, *PDDBOBNEXTFIELDINFO;

/* Change the state (bob, weave, autoflip) of a surface. */
typedef struct _DDSETSTATEININFO {
  PDDSURFACEDATA lpSurfaceData;
  PDDVIDEOPORTDATA lpVideoPortData;
} DDSETSTATEININFO, *PDDSETSTATEININFO;
typedef struct _DDSETSTATEOUTINFO {
  BOOL bSoftwareAutoflip;
  DWORD dwSurfaceIndex;
  DWORD dwVBISurfaceIndex;
} DDSETSTATEOUTINFO, *PDDSETSTATEOUTINFO;

/* Lock a surface. */
typedef struct _DDLOCKININFO {
  PDDSURFACEDATA lpSurfaceData;
} DDLOCKININFO, *PDDLOCKININFO;
typedef struct _DDLOCKOUTINFO {
  ULONG_PTR dwSurfacePtr;
} DDLOCKOUTINFO, *PDDLOCKOUTINFO;

/* Flip an overlay or a video port. */
typedef struct _DDFLIPOVERLAYINFO {
  PDDSURFACEDATA lpCurrentSurface;
  PDDSURFACEDATA lpTargetSurface;
  DWORD dwFlags;             /* DDFLIP_* */
} DDFLIPOVERLAYINFO, *PDDFLIPOVERLAYINFO;

#define DDVPFLIP_VIDEO 0x00000001
#define DDVPFLIP_VBI 0x00000002
typedef struct _DDFLIPVIDEOPORTINFO {
  PDDVIDEOPORTDATA lpVideoPortData;
  PDDSURFACEDATA lpCurrentSurface;
  PDDSURFACEDATA lpTargetSurface;
  DWORD dwFlipVPFlags;       /* DDVPFLIP_* */
} DDFLIPVIDEOPORTINFO, *PDDFLIPVIDEOPORTINFO;

/* Which field the video port writes. */
typedef struct _DDGETPOLARITYININFO {
  PDDVIDEOPORTDATA lpVideoPortData;
} DDGETPOLARITYININFO, *PDDGETPOLARITYININFO;
typedef struct _DDGETPOLARITYOUTINFO {
  DWORD bPolarity;           /* TRUE: the even field */
} DDGETPOLARITYOUTINFO, *PDDGETPOLARITYOUTINFO;

/* Which surface the hardware autoflip writes now, or wrote last. */
typedef struct _DDGETCURRENTAUTOFLIPININFO {
  PDDVIDEOPORTDATA lpVideoPortData;
} DDGETCURRENTAUTOFLIPININFO, *PDDGETCURRENTAUTOFLIPININFO;
typedef struct _DDGETCURRENTAUTOFLIPOUTINFO {
  DWORD dwSurfaceIndex;
  DWORD dwVBISurfaceIndex;
} DDGETCURRENTAUTOFLIPOUTINFO, *PDDGETCURRENTAUTOFLIPOUTINFO;
typedef struct _DDGETPREVIOUSAUTOFLIPININFO {
  PDDVIDEOPORTDATA lpVideoPortData;
} DDGETPREVIOUSAUTOFLIPININFO, *PDDGETPREVIOUSAUTOFLIPININFO;
typedef struct _DDGETPREVIOUSAUTOFLIPOUTINFO {
  DWORD dwSurfaceIndex;
  DWORD dwVBISurfaceIndex;
} DDGETPREVIOUSAUTOFLIPOUTINFO, *PDDGETPREVIOUSAUTOFLIPOUTINFO;

/* Bus-master transfers from a surface into memory. */
#define DDTRANSFER_SYSTEMMEMORY 0x00000001
#define DDTRANSFER_NONLOCALVIDMEM 0x00000002
#define DDTRANSFER_INVERT 0x00000004
#define DDTRANSFER_CANCEL 0x00000080
#define DDTRANSFER_HALFLINES 0x00000100
typedef struct _DDTRANSFERININFO {
  PDDSURFACEDATA lpSurfaceData;
  DWORD dwStartLine;
  DWORD dwEndLine;
  ULONG_PTR dwTransferID;
  DWORD dwTransferFlags;     /* DDTRANSFER_* */
  struct _MDL *lpDestMDL;    /* the memory to write (a PMDL) */
} DDTRANSFERININFO, *PDDTRANSFERININFO;
typedef struct _DDTRANSFEROUTINFO {
  DWORD dwBufferPolarity;
} DDTRANSFEROUTINFO, *PDDTRANSFEROUTINFO;
typedef struct _DDGETTRANSFERSTATUSOUTINFO {
  ULONG_PTR dwTransferID;    /* the transfer that finished */
} DDGETTRANSFERSTATUSOUTINFO, *PDDGETTRANSFERSTATUSOUTINFO;

/* The functions of the interface: each takes the miniport's device
 * extension, its input and its output. */
typedef DWORD (NTAPI *PDX_GETIRQINFO)(PVOID, PVOID, PDDGETIRQINFO);
typedef DWORD (NTAPI *PDX_ENABLEIRQ)(PVOID, PDDENABLEIRQINFO, PVOID);
typedef DWORD (NTAPI *PDX_SKIPNEXTFIELD)(PVOID, PDDSKIPNEXTFIELDINFO, PVOID);
typedef DWORD (NTAPI *PDX_BOBNEXTFIELD)(PVOID, PDDBOBNEXTFIELDINFO, PVOID);
typedef DWORD (NTAPI *PDX_SETSTATE)(PVOID, PDDSETSTATEININFO,
                              PDDSETSTATEOUTINFO);
typedef DWORD (NTAPI *PDX_LOCK)(PVOID, PDDLOCKININFO, PDDLOCKOUTINFO);
typedef DWORD (NTAPI *PDX_FLIPOVERLAY)(PVOID, PDDFLIPOVERLAYINFO, PVOID);
typedef DWORD (NTAPI *PDX_FLIPVIDEOPORT)(PVOID, PDDFLIPVIDEOPORTINFO, PVOID);
typedef DWORD (NTAPI *PDX_GETPOLARITY)(PVOID, PDDGETPOLARITYININFO,
                                 PDDGETPOLARITYOUTINFO);
typedef DWORD (NTAPI *PDX_GETCURRENTAUTOFLIP)(PVOID, PDDGETCURRENTAUTOFLIPININFO,
                                        PDDGETCURRENTAUTOFLIPOUTINFO);
typedef DWORD (NTAPI *PDX_GETPREVIOUSAUTOFLIP)(PVOID,
                                         PDDGETPREVIOUSAUTOFLIPININFO,
                                         PDDGETPREVIOUSAUTOFLIPOUTINFO);
typedef DWORD (NTAPI *PDX_TRANSFER)(PVOID, PDDTRANSFERININFO,
                              PDDTRANSFEROUTINFO);
typedef DWORD (NTAPI *PDX_GETTRANSFERSTATUS)(PVOID, PVOID,
                                       PDDGETTRANSFERSTATUSOUTINFO);

/* What HwQueryInterface fills in for GUID_DxApi. Size, Version,
 * Context, InterfaceReference and InterfaceDereference are those of an
 * INTERFACE. */
typedef struct _DXAPI_INTERFACE {
  USHORT Size;               /* sizeof(DXAPI_INTERFACE) */
  USHORT Version;            /* DXAPI_HALVERSION */
  PVOID Context;             /* the device extension */
  PVOID InterfaceReference;
  PVOID InterfaceDereference;
  PDX_GETIRQINFO DxGetIrqInfo;
  PDX_ENABLEIRQ DxEnableIrq;
  PDX_SKIPNEXTFIELD DxSkipNextField;
  PDX_BOBNEXTFIELD DxBobNextField;
  PDX_SETSTATE DxSetState;
  PDX_LOCK DxLock;
  PDX_FLIPOVERLAY DxFlipOverlay;
  PDX_FLIPVIDEOPORT DxFlipVideoPort;
  PDX_GETPOLARITY DxGetPolarity;
  PDX_GETCURRENTAUTOFLIP DxGetCurrentAutoflip;
  PDX_GETPREVIOUSAUTOFLIP DxGetPreviousAutoflip;
  PDX_TRANSFER DxTransfer;
  PDX_GETTRANSFERSTATUS DxGetTransferStatus;
} DXAPI_INTERFACE, *PDXAPI_INTERFACE;

#ifdef __cplusplus
}
#endif

#endif
