/* A stand-in for a DDK's ntddk.h for run-test.sh: a few declarations
   without calling conventions, as the DDKs write them (cl /Gz makes them
   __stdcall). */
typedef long NTSTATUS;
typedef unsigned long ULONG;
typedef struct _DRIVER_OBJECT *PDRIVER_OBJECT;
typedef void *PUNICODE_STRING;
typedef void *PDEVICE_OBJECT;
NTSTATUS IoCreateDevice(PDRIVER_OBJECT, ULONG, PUNICODE_STRING, ULONG, ULONG,
                        unsigned char, PDEVICE_OBJECT *);
ULONG __cdecl DbgPrint(const char *, ...);
