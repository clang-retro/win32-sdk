/* An AVStream minidriver (<ks.h>, -lks): DriverEntry hands KS.SYS a
   device descriptor with one capture filter, whose one output pin is
   pin-centric (KSPIN_DISPATCH's Process). AVStream came with 98 Second
   Edition's KS.SYS (KsInitializeDriver), so run-tests.sh checks that 98
   SE and ME can load it and 98 cannot. With -DME_ONLY, Process fills
   the frame at the leading edge, with the stream pointer functions ME
   added, and the check for 98 SE must fail too. */
#include <ntddk.h>
#include <windef.h>
#include <ks.h>

/* Local copies, so the driver needs no GUID library. */
static const GUID video_category = {
    0x6994ad05, 0x93ef, 0x11d0, {0xa3, 0xcc, 0x00, 0xa0, 0xc9, 0x22, 0x31, 0x96}};
static const GUID major_stream = {
    0xe436eb83, 0x524f, 0x11ce, {0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}};
static const GUID sub_none = {
    0xe436eb8e, 0x524f, 0x11ce, {0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}};
static const GUID spec_none = {
    0x0f6417d6, 0xc318, 0x11d0, {0xa4, 0x3f, 0x00, 0xa0, 0xc9, 0x22, 0x31, 0x96}};

static NTSTATUS NTAPI PinProcess(PKSPIN pin) {
#ifdef ME_ONLY
  PKSSTREAM_POINTER sp =
      KsPinGetLeadingEdgeStreamPointer(pin, KSSTREAM_POINTER_STATE_LOCKED);
  if (!sp)
    return STATUS_SUCCESS;
  RtlFillMemory(sp->OffsetOut.Data, sp->OffsetOut.Remaining, 0x80);
  sp->StreamHeader->DataUsed = sp->OffsetOut.Remaining;
  KsStreamPointerAdvanceOffsetsAndUnlock(sp, 0, sp->OffsetOut.Remaining,
                                         TRUE);
#else
  (void)pin;
#endif
  return STATUS_SUCCESS;
}

static const KSPIN_DISPATCH pin_dispatch = {.Process = PinProcess};

static KSDATARANGE range = {
    .FormatSize = sizeof(KSDATARANGE),
    .SampleSize = 640 * 480 * 2,
};
static PKSDATARANGE ranges[] = {&range};

static const KSALLOCATOR_FRAMING_EX framing = {
    .CountItems = 1,
    .PinFlags = 0,
    .OutputCompression = {1, 1, 0},
    .PinWeight = 0,
    .FramingItem = {{.MemoryType = {0},
                     .BusType = {0},
                     .MemoryFlags = KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY,
                     .BusFlags = 0,
                     .Flags = KSALLOCATOR_FLAG_PARTIAL_READ_SUPPORT,
                     .Frames = 2,
                     .FileAlignment = FILE_LONG_ALIGNMENT,
                     .FramingRange = {{640 * 480 * 2, 640 * 480 * 2, 1}, 0,
                                      0}}},
};

static const KSPIN_DESCRIPTOR_EX pins[] = {{
    .Dispatch = &pin_dispatch,
    .PinDescriptor = {.DataRangesCount = 1,
                      .DataRanges = ranges,
                      .DataFlow = KSPIN_DATAFLOW_OUT,
                      .Communication = KSPIN_COMMUNICATION_BOTH,
                      .Category = &video_category},
    .Flags = KSPIN_FLAG_PROCESS_IN_RUN_STATE_ONLY,
    .InstancesPossible = 1,
    .InstancesNecessary = 1,
    .AllocatorFraming = &framing,
}};

static const GUID categories[] = {
    {0x6994ad05, 0x93ef, 0x11d0, {0xa3, 0xcc, 0x00, 0xa0, 0xc9, 0x22, 0x31, 0x96}}};

static const KSFILTER_DESCRIPTOR filter = {
    .Version = KSFILTER_DESCRIPTOR_VERSION,
    .ReferenceGuid = &video_category,
    .PinDescriptorsCount = 1,
    .PinDescriptorSize = sizeof(KSPIN_DESCRIPTOR_EX),
    .PinDescriptors = pins,
    .CategoriesCount = 1,
    .Categories = categories,
};
static const KSFILTER_DESCRIPTOR *const filters[] = {&filter};

static const KSDEVICE_DESCRIPTOR device = {
    .FilterDescriptorsCount = 1,
    .FilterDescriptors = filters,
};

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING path) {
  range.MajorFormat = major_stream;
  range.SubFormat = sub_none;
  range.Specifier = spec_none;
  return KsInitializeDriver(driver, path, &device);
}
