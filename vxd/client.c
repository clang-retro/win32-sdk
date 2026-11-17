/* Loads hello.c's VxD (HELLO.VXD next to this program, or the file named
   on the command line), calls it and writes what it answered to standard
   output and to RESULT.TXT next to the program: "hello from clang vxd
   428571431 4.10" on Windows 98. With /exit it then shuts Windows down,
   for unattended runs. */
#include <windows.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
  char path[MAX_PATH + 8] = "\\\\.\\", dir[MAX_PATH], buf[64] = {0}, *s;
  const char *name = "HELLO.VXD";
  int i, shut = 0;
  DWORD n = 3, got = 0;
  HANDLE h;
  FILE *f;
  GetModuleFileNameA(NULL, dir, MAX_PATH);
  s = strrchr(dir, '\\');
  if (s)
    s[1] = 0;
  strcat(path, dir);
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "/exit") == 0)
      shut = 1;
    else
      name = argv[i];
  }
  strcat(path, name);
  h = CreateFileA(path, 0, 0, NULL, 0, FILE_FLAG_DELETE_ON_CLOSE, NULL);
  if (h == INVALID_HANDLE_VALUE)
    sprintf(buf, "open failed: %lu", GetLastError());
  else if (!DeviceIoControl(h, 1, &n, sizeof n, buf, sizeof buf, &got, NULL))
    sprintf(buf, "ioctl failed: %lu", GetLastError());
  if (h != INVALID_HANDLE_VALUE)
    CloseHandle(h);
  printf("%s\n", buf);
  strcat(dir, "RESULT.TXT");
  f = fopen(dir, "w");
  if (f) {
    fprintf(f, "%s\n", buf);
    fclose(f);
  }
  if (shut)
    ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, 0);
  return 0;
}
