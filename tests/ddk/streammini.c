/* A stream class minidriver (video capture, a TV tuner) for Windows 98,
   98 SE and ME (and 2000 and later), with <strmini.h> and STREAM.SYS: it
   registers with the stream class driver, and its adapter request
   handler completes each request at once. run-tests.sh builds it with
   nt4-ddk.cfg and -lstream and checks with check-imports.py --wdm that it
   imports only what 98's STREAM.SYS has; with -DME_ONLY it also calls
   StreamClassFilterReenumerateStreams, which ME added, and the check for
   98 SE must fail. */
#include <ntddk.h>
#include <windef.h>
#include <ks.h>
#include <strmini.h>

typedef struct {
  ULONG requests;
} DEVICE_EXTENSION;

static VOID STREAMAPI ReceivePacket(PHW_STREAM_REQUEST_BLOCK srb) {
  DEVICE_EXTENSION *ext = srb->HwDeviceExtension;
  ext->requests++;
  switch (srb->Command) {
  case SRB_INITIALIZE_DEVICE:
    srb->Status = STATUS_SUCCESS;
    break;
  case SRB_UNINITIALIZE_DEVICE:
    /* (A stdcall function that mingw-w64's strmini.h declared without
       STREAMAPI; fix-ddk-headers.sh adds it.) */
    StreamClassAbortOutstandingRequests(ext, NULL, STATUS_CANCELLED);
    srb->Status = STATUS_SUCCESS;
    break;
  default:
    srb->Status = STATUS_NOT_IMPLEMENTED;
  }
#ifdef ME_ONLY
  if (srb->Command == SRB_INITIALIZE_DEVICE)
    StreamClassFilterReenumerateStreams(srb->HwDeviceExtension, 0);
#endif
  StreamClassDeviceNotification(DeviceRequestComplete, srb->HwDeviceExtension,
                                srb, NULL, NULL, 0);
}

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING path) {
  HW_INITIALIZATION_DATA init;
  RtlZeroMemory(&init, sizeof init);
  init.HwInitializationDataSize = sizeof init;
  init.HwReceivePacket = ReceivePacket;
  init.DeviceExtensionSize = sizeof(DEVICE_EXTENSION);
  init.PerRequestExtensionSize = 0;
  init.FilterInstanceExtensionSize = 0;
  init.BusMasterDMA = FALSE;
  init.Dma24BitAddresses = FALSE;
  init.BufferAlignment = 3;
  init.TurnOffSynchronization = FALSE;
  init.DmaBufferSize = 0;
  return StreamClassRegisterAdapter(driver, path, &init);
}
