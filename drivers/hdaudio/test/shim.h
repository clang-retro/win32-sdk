/* What hda.c needs from <ntddk.h>, for hdatest.c, which runs hda.c on a
   bare (emulated) PC without Windows. */
#ifndef SHIM_H
#define SHIM_H
typedef unsigned char UCHAR, BOOLEAN, KIRQL;
typedef unsigned short USHORT;
typedef unsigned int ULONG;
typedef int LONG, NTSTATUS;
typedef void *PVOID;
typedef ULONG KSPIN_LOCK;
#define TRUE 1
#define FALSE 0
#ifndef NULL
#define NULL ((void *)0)
#endif
#define NT_SUCCESS(s) ((NTSTATUS)(s) >= 0)
#define STATUS_SUCCESS 0
#define STATUS_NO_SUCH_DEVICE ((NTSTATUS)0xC000000E)
#define STATUS_INVALID_PARAMETER ((NTSTATUS)0xC000000D)
#define STATUS_DEVICE_NOT_READY ((NTSTATUS)0xC00000A3)
#define STATUS_DEVICE_NOT_CONNECTED ((NTSTATUS)0xC000009D)
#define STATUS_DEVICE_CONFIGURATION_ERROR ((NTSTATUS)0xC0000182)
void KeStallExecutionProcessor(ULONG us);
#define KeInitializeSpinLock(l) (*(l) = 0)
#define KeAcquireSpinLock(l, i) (*(i) = 0, (void)(l))
#define KeReleaseSpinLock(l, i) ((void)(l), (void)(i))
#define RtlZeroMemory(p, n) __builtin_memset((p), 0, (n))
#endif
