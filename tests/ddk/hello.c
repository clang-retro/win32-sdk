/* A small NT kernel-mode driver, built by run-tests.sh with each *-ddk.cfg
   (mingw-w64's DDK headers) and by kits-test.py with each imported
   ms-*-ddk.cfg (Microsoft's DDKs, NT 3.1 to Server 2003) and
   ms-w98-wdm.cfg (-DHELLO_WDM).

   It makes \Device\ClangHello and \DosDevices\ClangHello. Its one IOCTL
   takes a ULONG n and answers with "hello from clang" and the digits of
   n * 1000000007 / 7, a 64-bit multiplication and division, so the
   driver links the compiler's or the kernel's 64-bit arithmetic helpers,
   then the number of opens and "seh 1" if structured exception handling
   caught its division by zero (with __try, __except, __finally and
   GetExceptionCode, which take _except_handler3 or __C_specific_handler).
   It divides by zero rather than reading an unmapped address because
   Wine, which run-tests.sh runs it in, maps the low addresses NT leaves
   unmapped, and rather than calling ExRaiseStatus because Wine does not
   have it. client.c calls it. It uses only what the NT 3.1
   DDK has. */
#ifdef HELLO_WDM
#include <wdm.h> /* a WDM kit's (ms-w98-wdm), which has no ntddk.h */
#else
#include <ntddk.h>
#endif

#define IOCTL_HELLO \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

static WCHAR devname[] = L"\\Device\\ClangHello";
static WCHAR linkname[] = L"\\DosDevices\\ClangHello";
static ULONG opens;

/* 1 if __except gets the processor's divide error, and __finally runs on
   the way out. */
static ULONG seh_works(void) {
  volatile ULONG zero = 0, got = 0, cleaned = 0;
  __try {
    __try {
      got = 2 / zero;
    } __finally {
      cleaned = 1;
    }
  } __except (GetExceptionCode() == STATUS_INTEGER_DIVIDE_BY_ZERO
                  ? EXCEPTION_EXECUTE_HANDLER
                  : EXCEPTION_CONTINUE_SEARCH) {
    got = 1;
  }
  return got == 1 && cleaned == 1;
}

static NTSTATUS NTAPI dispatch(PDEVICE_OBJECT dev, PIRP irp) {
  PIO_STACK_LOCATION s = IoGetCurrentIrpStackLocation(irp);
  NTSTATUS st = STATUS_SUCCESS;
  ULONG info = 0;
  (void)dev;
  switch (s->MajorFunction) {
  case IRP_MJ_CREATE:
    opens++;
    break;
  case IRP_MJ_CLOSE:
    break;
  case IRP_MJ_DEVICE_CONTROL:
    if (s->Parameters.DeviceIoControl.IoControlCode == IOCTL_HELLO &&
        s->Parameters.DeviceIoControl.InputBufferLength >= sizeof(ULONG) &&
        s->Parameters.DeviceIoControl.OutputBufferLength >= 48) {
      char *out = irp->AssociatedIrp.SystemBuffer;
      ULONG n = *(ULONG *)out;
      unsigned __int64 q = (unsigned __int64)n * 1000000007 / 7;
      char digits[24];
      int i = 0, j;
      static const char msg[] = "hello from clang";
      RtlCopyMemory(out, msg, sizeof msg - 1);
      j = sizeof msg - 1;
      out[j++] = ' ';
      do {
        digits[i++] = (char)('0' + (int)(q % 10));
        q /= 10;
      } while (q);
      while (i)
        out[j++] = digits[--i];
      out[j++] = ' ';
      out[j++] = (char)('0' + (int)(opens % 10));
      RtlCopyMemory(out + j, " seh ", 5);
      j += 5;
      out[j++] = (char)('0' + (int)seh_works());
      out[j++] = 0;
      info = j;
    } else {
      st = STATUS_INVALID_DEVICE_REQUEST;
    }
    break;
  default:
    st = STATUS_INVALID_DEVICE_REQUEST;
  }
  irp->IoStatus.Status = st;
  irp->IoStatus.Information = info;
  IoCompleteRequest(irp, IO_NO_INCREMENT);
  return st;
}

static VOID NTAPI unload(PDRIVER_OBJECT drv) {
  UNICODE_STRING link;
  RtlInitUnicodeString(&link, linkname);
  IoDeleteSymbolicLink(&link);
  IoDeleteDevice(drv->DeviceObject);
}

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT drv, PUNICODE_STRING reg) {
  UNICODE_STRING dev, link;
  PDEVICE_OBJECT obj;
  NTSTATUS st;
  (void)reg;
  DbgPrint("clang hello driver: DriverEntry\n");
  RtlInitUnicodeString(&dev, devname);
  RtlInitUnicodeString(&link, linkname);
  st = IoCreateDevice(drv, 0, &dev, FILE_DEVICE_UNKNOWN, 0, FALSE, &obj);
  if (!NT_SUCCESS(st))
    return st;
  st = IoCreateSymbolicLink(&link, &dev);
  if (!NT_SUCCESS(st)) {
    IoDeleteDevice(obj);
    return st;
  }
  drv->MajorFunction[IRP_MJ_CREATE] = dispatch;
  drv->MajorFunction[IRP_MJ_CLOSE] = dispatch;
  drv->MajorFunction[IRP_MJ_DEVICE_CONTROL] = dispatch;
  drv->DriverUnload = unload;
  return STATUS_SUCCESS;
}
