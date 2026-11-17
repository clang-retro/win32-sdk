/* Loads provider.c's VxD (PROVIDE.VXD next to this program) and calls its
   Win32 services through KERNEL32's VxDCall, then asks the VxD what its
   probe service saw. Writes, to standard output and RESULT.TXT next to the
   program:

     w32 probe 600d add 42
     regs eax ... (what the probe service was called with)
     stack ...
     register ... table ... (what _Register_Win32_Services returned, and
                             the DDB's Win32 service table after it)

   VxDCall is KERNEL32's export 1 and the ones after it (VxDCall0 to
   VxDCall8: the service, then 0 to 8 dword parameters); KERNEL32 does not
   let GetProcAddress find it by number, so this reads the export table. */
#include <windows.h>
#include <stdio.h>
#include <string.h>

static FARPROC kernel32_ordinal(WORD ord) {
  BYTE *base = (BYTE *)GetModuleHandleA("KERNEL32.DLL");
  IMAGE_NT_HEADERS *nt =
      (IMAGE_NT_HEADERS *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
  IMAGE_EXPORT_DIRECTORY *ex =
      (IMAGE_EXPORT_DIRECTORY *)(base + nt->OptionalHeader.DataDirectory[0]
                                            .VirtualAddress);
  DWORD *fns = (DWORD *)(base + ex->AddressOfFunctions);
  return (FARPROC)(base + fns[ord - ex->Base]);
}

#define PROVIDE_ID 0x3E5A

int main(void) {
  char path[MAX_PATH + 8] = "\\\\.\\", dir[MAX_PATH], buf[512], *s;
  DWORD (WINAPI *call3)(DWORD, DWORD, DWORD, DWORD);
  DWORD (WINAPI *call2)(DWORD, DWORD, DWORD);
  DWORD probe[14] = {0}, got = 0, ret, add;
  HANDLE h;
  FILE *f;
  GetModuleFileNameA(NULL, dir, MAX_PATH);
  s = strrchr(dir, '\\');
  if (s)
    s[1] = 0;
  strcat(path, dir);
  strcat(path, "PROVIDE.VXD");
  h = CreateFileA(path, 0, 0, NULL, 0, FILE_FLAG_DELETE_ON_CLOSE, NULL);
  if (h == INVALID_HANDLE_VALUE) {
    sprintf(buf, "open failed: %lu", GetLastError());
  } else {
    call3 = (DWORD(WINAPI *)(DWORD, DWORD, DWORD, DWORD))kernel32_ordinal(4);
    ret = call3(PROVIDE_ID << 16 | 0, 0x11111111, 0x22222222, 0x33333333);
    call2 = (DWORD(WINAPI *)(DWORD, DWORD, DWORD))kernel32_ordinal(3);
    add = call2(PROVIDE_ID << 16 | 1, 40, 2);
    DeviceIoControl(h, 2, NULL, 0, probe, sizeof probe, &got, NULL);
    sprintf(buf,
            "w32 probe %lx add %lu\n"
            "regs eax %08lx ebx %08lx ecx %08lx edx %08lx esi %08lx "
            "edi %08lx ebp %08lx esp %08lx\n"
            "stack %08lx %08lx %08lx %08lx\n"
            "register %08lx table %08lx",
            ret, add, probe[0], probe[1], probe[2], probe[3], probe[4],
            probe[5], probe[6], probe[7], probe[8], probe[9], probe[10],
            probe[11], probe[12], probe[13]);
    CloseHandle(h);
  }
  printf("%s\n", buf);
  strcat(dir, "RESULT.TXT");
  f = fopen(dir, "w");
  if (f) {
    fprintf(f, "%s\n", buf);
    fclose(f);
  }
  return 0;
}
