/* A SCSI miniport for Windows 95, 98 and ME (win95-miniport.cfg): a
   virtual host adapter with no hardware, whose one device, target 0 LUN
   0, is a processor device that answers INQUIRY and TEST UNIT READY. The
   other targets time out, as empty ones do. run-tests.sh builds it and
   checks with check-imports.py --miniport that it imports only what
   SCSIPORT.PDR has, and its layouts with <srb.h>'s, which are 9x's (a
   mingw-w64 HW_INITIALIZATION_DATA is 4 bytes larger and SCSIPORT.PDR
   refuses it). */
#include <miniport.h>
#include <srb.h>
#include <scsi.h>

_Static_assert(sizeof(HW_INITIALIZATION_DATA) == 76, "9x's layout");
_Static_assert(sizeof(PORT_CONFIGURATION_INFORMATION) == 124, "9x's layout");

typedef struct {
  ULONG requests;
} EXTENSION;

static const char vendor[] = "CLANG   ";
static const char product[] = "VIRTUAL ADAPTER ";

static ULONG FindAdapter(PVOID ext, PVOID context, PVOID bus, PCHAR args,
                         PPORT_CONFIGURATION_INFORMATION config,
                         PBOOLEAN again) {
  (void)ext, (void)context, (void)bus, (void)args;
  *again = FALSE;
  config->NumberOfBuses = 1;
  config->InitiatorBusId[0] = 7;
  config->MaximumTransferLength = 0x10000;
  config->MaximumNumberOfTargets = 8;
  return SP_RETURN_FOUND;
}

static BOOLEAN Initialize(PVOID ext) {
  ((EXTENSION *)ext)->requests = 0;
  return TRUE;
}

static void inquiry(PSCSI_REQUEST_BLOCK srb) {
  INQUIRYDATA data = {0};
  ULONG n = sizeof data;
  int i;
  data.DeviceType = PROCESSOR_DEVICE;
  data.Versions = 2;
  data.ResponseDataFormat = 2;
  data.AdditionalLength = sizeof data - 5;
  for (i = 0; i < 8; i++)
    data.VendorId[i] = vendor[i];
  for (i = 0; i < 16; i++)
    data.ProductId[i] = product[i];
  if (n > srb->DataTransferLength)
    n = srb->DataTransferLength;
  ScsiPortMoveMemory(srb->DataBuffer, &data, n);
  srb->DataTransferLength = n;
  srb->SrbStatus = SRB_STATUS_SUCCESS;
}

static BOOLEAN StartIo(PVOID ext, PSCSI_REQUEST_BLOCK srb) {
  ((EXTENSION *)ext)->requests++;
  if (srb->Function != SRB_FUNCTION_EXECUTE_SCSI) {
    srb->SrbStatus = SRB_STATUS_SUCCESS;  /* flush, shutdown and the rest */
  } else if (srb->PathId || srb->TargetId || srb->Lun) {
    srb->SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;
  } else if (srb->Cdb[0] == SCSIOP_INQUIRY) {
    inquiry(srb);
  } else if (srb->Cdb[0] == SCSIOP_TEST_UNIT_READY) {
    srb->SrbStatus = SRB_STATUS_SUCCESS;
  } else {
    srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
  }
  ScsiPortNotification(RequestComplete, ext, srb);
  ScsiPortNotification(NextRequest, ext, NULL);
  return TRUE;
}

static BOOLEAN ResetBus(PVOID ext, ULONG path) {
  ScsiPortCompleteRequest(ext, (UCHAR)path, SP_UNTAGGED, SP_UNTAGGED,
                          SRB_STATUS_BUS_RESET);
  return TRUE;
}

ULONG DriverEntry(PVOID driver, PVOID path) {
  HW_INITIALIZATION_DATA init = {0};
  init.HwInitializationDataSize = sizeof init;
  init.AdapterInterfaceType = Isa;
  init.HwFindAdapter = FindAdapter;
  init.HwInitialize = Initialize;
  init.HwStartIo = StartIo;
  init.HwResetBus = ResetBus;
  init.DeviceExtensionSize = sizeof(EXTENSION);
  init.MapBuffers = TRUE;
  return ScsiPortInitialize(driver, path, &init, NULL);
}
