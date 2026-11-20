/* Calls async.c's VxD (ASYNC.VXD next to this program): one
   DeviceIoControl with an OVERLAPPED, which must return FALSE with
   ERROR_IO_PENDING and finish in GetOverlappedResult, and one without,
   which finishes at once. Writes what it saw to standard output and to
   RESULT.TXT next to the program:

     async pending 1 bytes 13 text async done 1 sync

   With /exit it then shuts Windows down, for unattended runs. */
#include <windows.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
  char path[MAX_PATH + 8] = "\\\\.\\", dir[MAX_PATH], buf[64] = {0};
  char sync[64] = {0}, res[256], *s;
  OVERLAPPED o;
  DWORD got = 0, got2 = 0;
  int pending = 0, shut = argc > 1 && strcmp(argv[1], "/exit") == 0;
  HANDLE h;
  FILE *f;
  GetModuleFileNameA(NULL, dir, MAX_PATH);
  s = strrchr(dir, '\\');
  if (s)
    s[1] = 0;
  strcat(path, dir);
  strcat(path, "ASYNC.VXD");
  h = CreateFileA(path, 0, 0, NULL, 0,
                  FILE_FLAG_DELETE_ON_CLOSE | FILE_FLAG_OVERLAPPED, NULL);
  if (h == INVALID_HANDLE_VALUE) {
    sprintf(res, "open failed: %lu", GetLastError());
  } else {
    memset(&o, 0, sizeof o);
    o.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    if (DeviceIoControl(h, 1, NULL, 0, buf, sizeof buf, &got, &o))
      strcpy(buf, "(finished at once)");
    else if (GetLastError() == ERROR_IO_PENDING)
      pending = 1;
    if (pending && !GetOverlappedResult(h, &o, &got, TRUE))
      sprintf(buf, "(GetOverlappedResult failed: %lu)", GetLastError());
    if (!DeviceIoControl(h, 1, NULL, 0, sync, sizeof sync, &got2, NULL))
      sprintf(sync, "(failed: %lu)", GetLastError());
    sprintf(res, "async pending %d bytes %lu text %s %s", pending, got, buf,
            sync);
    CloseHandle(o.hEvent);
    CloseHandle(h);
  }
  printf("%s\n", res);
  strcat(dir, "RESULT.TXT");
  f = fopen(dir, "w");
  if (f) {
    fprintf(f, "%s\n", res);
    fclose(f);
  }
  if (shut)
    ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, 0);
  return 0;
}
