/* Opens the device of hello.c, sends it IOCTL_HELLO with n = 3 and prints
   the answer: "hello from clang 428571431 1". */
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>

#define IOCTL_HELLO \
  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

int main(void) {
  char buf[64] = {0};
  DWORD n = 0;
  HANDLE h = CreateFileA("\\\\.\\ClangHello", GENERIC_READ | GENERIC_WRITE,
                         0, NULL, OPEN_EXISTING, 0, NULL);
  if (h == INVALID_HANDLE_VALUE) {
    printf("open failed: %lu\n", GetLastError());
    return 1;
  }
  *(ULONG *)buf = 3;
  if (!DeviceIoControl(h, IOCTL_HELLO, buf, sizeof(ULONG), buf, sizeof buf,
                       &n, NULL)) {
    printf("ioctl failed: %lu\n", GetLastError());
    return 1;
  }
  printf("%s\n", buf);
  CloseHandle(h);
  return 0;
}
