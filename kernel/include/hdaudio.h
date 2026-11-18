/* hdaudio.h: The HD Audio bus DDI. A function driver for an HD Audio
   codec (a modem or audio function group) runs on Microsoft's HD Audio
   bus driver (HDAUDBUS.SYS: Windows XP and Server 2003 with the UAA
   update, KB888111, and Vista and later) and gets this interface from it
   with IRP_MN_QUERY_INTERFACE: codec verbs, DMA engines and buffers, the
   wall clock and link position registers, and unsolicited responses.

   Written for this SDK from the names, types and member order in
   Microsoft's public HD Audio DDI documentation; Microsoft's own
   hdaudio.h first came with the Vista WDK. GUID_HDAUDIO_BUS_INTERFACE
   is the interface XP's bus driver has; _BDL is the one with buffer
   descriptor lists. Include <initguid.h> first in one source file to
   define the GUIDs. */
#ifndef WDM_HDAUDIO_H
#define WDM_HDAUDIO_H

#include <ntddk.h>

DEFINE_GUID(GUID_HDAUDIO_BUS_INTERFACE, 0xd2eaf88b, 0xab18, 0x41a8, 0xb6,
            0x64, 0x8d, 0x59, 0x21, 0x67, 0x67, 0x1b);
DEFINE_GUID(GUID_HDAUDIO_BUS_INTERFACE_BDL, 0xb4d65397, 0x5634, 0x40b0,
            0xb0, 0x68, 0xf5, 0xb9, 0xf8, 0xb9, 0x67, 0xa5);
DEFINE_GUID(GUID_HDAUDIO_BUS_CLASS, 0xbbd1a745, 0xadd6, 0x4575, 0x9c, 0x2e,
            0x9b, 0x42, 0x8d, 0x1c, 0x32, 0x66);

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _HDAUDIO_CODEC_TRANSFER_
#define _HDAUDIO_CODEC_TRANSFER_
/* A codec verb: 12-bit verb ID with 8 bits of data, or 4-bit verb ID
   with 16 bits of data. */
typedef struct _HDAUDIO_CODEC_COMMAND {
  union {
    struct {
      ULONG Data : 8;
      ULONG VerbId : 12;
      ULONG Node : 8;
      ULONG CodecAddress : 4;
    } Verb8;
    struct {
      ULONG Data : 16;
      ULONG VerbId : 4;
      ULONG Node : 8;
      ULONG CodecAddress : 4;
    } Verb16;
    ULONG Command;
  };
} HDAUDIO_CODEC_COMMAND, *PHDAUDIO_CODEC_COMMAND;

typedef struct _HDAUDIO_CODEC_RESPONSE {
  union {
    struct {
      union {
        struct {
          ULONG Response : 26;
          ULONG Tag : 6;
        } Unsolicited;
        ULONG Response;
      };
      ULONG SDataIn : 4;
      ULONG IsUnsolicitedResponse : 1;
      ULONG : 25;
      ULONG HasFifoOverrun : 1;
      ULONG IsValid : 1;
    };
    ULONGLONG CompleteResponse;
  };
} HDAUDIO_CODEC_RESPONSE, *PHDAUDIO_CODEC_RESPONSE;

typedef struct _HDAUDIO_CODEC_TRANSFER {
  HDAUDIO_CODEC_COMMAND Output;
  HDAUDIO_CODEC_RESPONSE Input;
} HDAUDIO_CODEC_TRANSFER, *PHDAUDIO_CODEC_TRANSFER;
#endif

typedef enum _HDAUDIO_STREAM_STATE {
  ResetState = 0,
  StopState = 1,
  PauseState = 1,
  RunState = 2
} HDAUDIO_STREAM_STATE, *PHDAUDIO_STREAM_STATE;

typedef struct _HDAUDIO_STREAM_FORMAT {
  ULONG SampleRate;
  USHORT ValidBitsPerSample;
  USHORT ContainerSize;
  USHORT NumberOfChannels;
} HDAUDIO_STREAM_FORMAT, *PHDAUDIO_STREAM_FORMAT;

/* The stream format register's layout, as the bus driver programs it
   in the codec's converters. */
typedef struct _HDAUDIO_CONVERTER_FORMAT {
  union {
    struct {
      USHORT NumberOfChannels : 4;
      USHORT BitsPerSample : 3;
      USHORT : 1;
      USHORT SampleRate : 7;
      USHORT StreamType : 1;
    };
    USHORT ConverterFormat;
  };
} HDAUDIO_CONVERTER_FORMAT, *PHDAUDIO_CONVERTER_FORMAT;

typedef struct _HDAUDIO_DEVICE_INFORMATION {
  USHORT Size;
  USHORT DeviceVersion;
  USHORT DriverVersion;
  USHORT CodecsDetected;
  BOOLEAN IsStripingSupported;
} HDAUDIO_DEVICE_INFORMATION, *PHDAUDIO_DEVICE_INFORMATION;

typedef struct _HDAUDIO_BUFFER_DESCRIPTOR {
  PHYSICAL_ADDRESS Address;
  ULONG Length;
  ULONG InterruptOnCompletion;
} HDAUDIO_BUFFER_DESCRIPTOR, *PHDAUDIO_BUFFER_DESCRIPTOR;

typedef VOID (NTAPI *PHDAUDIO_TRANSFER_COMPLETE_CALLBACK)(
    HDAUDIO_CODEC_TRANSFER *, PVOID);
typedef VOID (NTAPI *PHDAUDIO_UNSOLICITED_RESPONSE_CALLBACK)(
    HDAUDIO_CODEC_RESPONSE, PVOID);
typedef void (NTAPI *PHDAUDIO_BDL_ISR)(VOID *Context, ULONG InterruptBitMask);

typedef NTSTATUS (NTAPI *PTRANSFER_CODEC_VERBS)(
    PVOID _context, ULONG Count, PHDAUDIO_CODEC_TRANSFER CodecTransfer,
    PHDAUDIO_TRANSFER_COMPLETE_CALLBACK Callback, PVOID Context);
typedef NTSTATUS (NTAPI *PALLOCATE_CAPTURE_DMA_ENGINE)(
    PVOID _context, UCHAR CodecAddress, PHDAUDIO_STREAM_FORMAT StreamFormat,
    PHANDLE Handle, PHDAUDIO_CONVERTER_FORMAT ConverterFormat);
typedef NTSTATUS (NTAPI *PALLOCATE_RENDER_DMA_ENGINE)(
    PVOID _context, PHDAUDIO_STREAM_FORMAT StreamFormat, BOOLEAN Stripe,
    PHANDLE Handle, PHDAUDIO_CONVERTER_FORMAT ConverterFormat);
typedef NTSTATUS (NTAPI *PCHANGE_BANDWIDTH_ALLOCATION)(
    PVOID _context, HANDLE Handle, PHDAUDIO_STREAM_FORMAT StreamFormat,
    PHDAUDIO_CONVERTER_FORMAT ConverterFormat);
typedef NTSTATUS (NTAPI *PALLOCATE_DMA_BUFFER)(
    PVOID _context, HANDLE Handle, SIZE_T RequestedBufferSize,
    PMDL *BufferMdl, PSIZE_T AllocatedBufferSize, PUCHAR StreamId,
    PULONG FifoSize);
typedef NTSTATUS (NTAPI *PFREE_DMA_BUFFER)(PVOID _context, HANDLE Handle);
typedef NTSTATUS (NTAPI *PFREE_DMA_ENGINE)(PVOID _context, HANDLE Handle);
typedef NTSTATUS (NTAPI *PSET_DMA_ENGINE_STATE)(
    PVOID _context, HDAUDIO_STREAM_STATE StreamState, ULONG NumberOfHandles,
    PHANDLE Handles);
typedef VOID (NTAPI *PGET_WALL_CLOCK_REGISTER)(PVOID _context,
                                               PULONG *Wallclock);
typedef NTSTATUS (NTAPI *PGET_LINK_POSITION_REGISTER)(PVOID _context,
                                                      HANDLE Handle,
                                                      PULONG *Position);
typedef NTSTATUS (NTAPI *PREGISTER_EVENT_CALLBACK)(
    PVOID _context, PHDAUDIO_UNSOLICITED_RESPONSE_CALLBACK Routine,
    PVOID Context, PUCHAR Tag);
typedef NTSTATUS (NTAPI *PUNREGISTER_EVENT_CALLBACK)(PVOID _context,
                                                     UCHAR Tag);
typedef NTSTATUS (NTAPI *PGET_DEVICE_INFORMATION)(
    PVOID _context, PHDAUDIO_DEVICE_INFORMATION DeviceInformation);
typedef VOID (NTAPI *PGET_RESOURCE_INFORMATION)(
    PVOID _context, PUCHAR CodecAddress, PUCHAR FunctionGroupStartNode);
typedef NTSTATUS (NTAPI *PALLOCATE_CONTIGUOUS_DMA_BUFFER)(
    PVOID _context, HANDLE Handle, ULONG RequestedBufferSize,
    PVOID *DataBuffer, PHDAUDIO_BUFFER_DESCRIPTOR *BdlBuffer);
typedef NTSTATUS (NTAPI *PSETUP_DMA_ENGINE_WITH_BDL)(
    PVOID _context, HANDLE Handle, ULONG BufferLength, ULONG Lvi,
    PHDAUDIO_BDL_ISR Isr, PVOID Context, PUCHAR StreamId, PULONG FifoSize);
typedef NTSTATUS (NTAPI *PFREE_CONTIGUOUS_DMA_BUFFER)(PVOID _context,
                                                      HANDLE Handle);

typedef struct _HDAUDIO_BUS_INTERFACE {
  USHORT Size;
  USHORT Version;
  PVOID Context;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
  PTRANSFER_CODEC_VERBS TransferCodecVerbs;
  PALLOCATE_CAPTURE_DMA_ENGINE AllocateCaptureDmaEngine;
  PALLOCATE_RENDER_DMA_ENGINE AllocateRenderDmaEngine;
  PCHANGE_BANDWIDTH_ALLOCATION ChangeBandwidthAllocation;
  PALLOCATE_DMA_BUFFER AllocateDmaBuffer;
  PFREE_DMA_BUFFER FreeDmaBuffer;
  PFREE_DMA_ENGINE FreeDmaEngine;
  PSET_DMA_ENGINE_STATE SetDmaEngineState;
  PGET_WALL_CLOCK_REGISTER GetWallClockRegister;
  PGET_LINK_POSITION_REGISTER GetLinkPositionRegister;
  PREGISTER_EVENT_CALLBACK RegisterEventCallback;
  PUNREGISTER_EVENT_CALLBACK UnregisterEventCallback;
  PGET_DEVICE_INFORMATION GetDeviceInformation;
  PGET_RESOURCE_INFORMATION GetResourceInformation;
} HDAUDIO_BUS_INTERFACE, *PHDAUDIO_BUS_INTERFACE;

typedef struct _HDAUDIO_BUS_INTERFACE_BDL {
  USHORT Size;
  USHORT Version;
  PVOID Context;
  PINTERFACE_REFERENCE InterfaceReference;
  PINTERFACE_DEREFERENCE InterfaceDereference;
  PTRANSFER_CODEC_VERBS TransferCodecVerbs;
  PALLOCATE_CAPTURE_DMA_ENGINE AllocateCaptureDmaEngine;
  PALLOCATE_RENDER_DMA_ENGINE AllocateRenderDmaEngine;
  PCHANGE_BANDWIDTH_ALLOCATION ChangeBandwidthAllocation;
  PALLOCATE_CONTIGUOUS_DMA_BUFFER AllocateContiguousDmaBuffer;
  PSETUP_DMA_ENGINE_WITH_BDL SetupDmaEngineWithBdl;
  PFREE_CONTIGUOUS_DMA_BUFFER FreeContiguousDmaBuffer;
  PFREE_DMA_ENGINE FreeDmaEngine;
  PSET_DMA_ENGINE_STATE SetDmaEngineState;
  PGET_WALL_CLOCK_REGISTER GetWallClockRegister;
  PGET_LINK_POSITION_REGISTER GetLinkPositionRegister;
  PREGISTER_EVENT_CALLBACK RegisterEventCallback;
  PUNREGISTER_EVENT_CALLBACK UnregisterEventCallback;
  PGET_DEVICE_INFORMATION GetDeviceInformation;
  PGET_RESOURCE_INFORMATION GetResourceInformation;
} HDAUDIO_BUS_INTERFACE_BDL, *PHDAUDIO_BUS_INTERFACE_BDL;

#ifdef __cplusplus
}
#endif

#endif
