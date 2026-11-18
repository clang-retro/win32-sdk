/* An NDIS 5 miniport (a network card's driver, .sys) for Windows 98 SE
   and ME (and 2000 and later), whose NDIS.VXD loads it: it registers its
   handlers with NDIS, and its initialize handler finds no card.
   run-tests.sh builds it with nt4-ddk.cfg and -lndis and checks with
   check-imports.py --wdm 98se that 98 SE's NDIS.VXD has its imports;
   with -DXP_ONLY it also calls NdisGeneratePartialCancelId, which XP
   added, and the check must fail. */
#define NDIS_MINIPORT_DRIVER
#define NDIS50_MINIPORT 1
#include <ndis.h>

static NDIS_HANDLE wrapper;

static NDIS_STATUS NTAPI Initialize(PNDIS_STATUS open_error, PUINT medium,
                                    PNDIS_MEDIUM media, UINT nmedia,
                                    NDIS_HANDLE adapter, NDIS_HANDLE config) {
  (void)open_error, (void)medium, (void)media, (void)nmedia, (void)config;
#ifdef XP_ONLY
  (void)NdisGeneratePartialCancelId();
#endif
  NdisMSetAttributesEx(adapter, NULL, 0, NDIS_ATTRIBUTE_DESERIALIZE,
                       NdisInterfaceInternal);
  return NDIS_STATUS_ADAPTER_NOT_FOUND;
}

static VOID NTAPI Halt(NDIS_HANDLE context) { (void)context; }

static NDIS_STATUS NTAPI Query(NDIS_HANDLE context, NDIS_OID oid, PVOID buf,
                               ULONG len, PULONG written, PULONG needed) {
  (void)context, (void)oid, (void)buf, (void)len, (void)written,
      (void)needed;
  return NDIS_STATUS_NOT_SUPPORTED;
}

static NDIS_STATUS NTAPI Set(NDIS_HANDLE context, NDIS_OID oid, PVOID buf,
                             ULONG len, PULONG read, PULONG needed) {
  (void)context, (void)oid, (void)buf, (void)len, (void)read, (void)needed;
  return NDIS_STATUS_NOT_SUPPORTED;
}

static NDIS_STATUS NTAPI Reset(PBOOLEAN addressing, NDIS_HANDLE context) {
  (void)addressing, (void)context;
  return NDIS_STATUS_SUCCESS;
}

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING path) {
  NDIS_MINIPORT_CHARACTERISTICS mc;
  NDIS_STATUS status;
  NdisMInitializeWrapper(&wrapper, driver, path, NULL);
  NdisZeroMemory(&mc, sizeof mc);
  mc.MajorNdisVersion = 5;
  mc.MinorNdisVersion = 0;
  mc.InitializeHandler = Initialize;
  mc.HaltHandler = Halt;
  mc.QueryInformationHandler = Query;
  mc.SetInformationHandler = Set;
  mc.ResetHandler = Reset;
  status = NdisMRegisterMiniport(wrapper, &mc, sizeof mc);
  if (status != NDIS_STATUS_SUCCESS)
    NdisTerminateWrapper(wrapper, NULL);
  return status;
}
