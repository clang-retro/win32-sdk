/* A dynamic VxD for Windows 95, 98 and ME that calls VMM services and
   puts its code in each kind of object, built by run-tests.sh. When it
   loads, its init code allocates a heap block, writes a registry value
   (HKLM\Software\clang-retro\vxd), reads it back, sets a 50 ms time-out
   and schedules an event. client.c then calls it with DeviceIoControl
   code 1 and it answers, from pageable code, with what it saw:

     services heap 1 reg 1234 timer 1 event 1 page 42 init 7

   (heap: the block holds what the init code copied from init data; reg:
   the value read back; timer and event: how many times each callback
   ran; page: pageable data; init: a value init code computed). */
#include <vxd.h>

VXD_DEVICE(SVCTEST, UNDEFINED_DEVICE_ID, UNDEFINED_INIT_ORDER, 1, 0);

VXD_INIT_DATA static char init_text[] = "clang-retro";
VXD_INIT_DATA static volatile DWORD init_seed = 3;
VXD_PAGEABLE_DATA static volatile DWORD page_value = 42;

static char *block;
static DWORD reg_value, init_value, timer_runs, event_runs, timeout;

VXD_CALLBACK(on_timeout, timeout_fn);
int timeout_fn(struct vxd_regs *r) {
  (void)r;
  timeout = 0;
  timer_runs++;
  return 0;
}

VXD_CALLBACK(on_event, event_fn);
int event_fn(struct vxd_regs *r) {
  if (r->edx == 0x1234)
    event_runs++;
  return 0;
}

VXD_INIT_CODE static int init(void) {
  DWORD key, type, size = sizeof reg_value, v = 1234;
  unsigned i;
  block = _HeapAllocate(64, HEAPZEROINIT);
  if (!block)
    return 1;
  for (i = 0; init_text[i]; i++)
    block[i] = init_text[i];
  init_value = init_seed * 2 + 1;
  if (_RegCreateKey(HKEY_LOCAL_MACHINE, "Software\\clang-retro\\vxd", &key) ==
      0) {
    _RegSetValueEx(key, "test", 0, REG_DWORD, &v, sizeof v);
    _RegCloseKey(key);
  }
  if (_RegOpenKey(HKEY_LOCAL_MACHINE, "Software\\clang-retro\\vxd", &key) ==
      0) {
    _RegQueryValueEx(key, "test", 0, &type, &reg_value, &size);
    _RegCloseKey(key);
  }
  timeout = VMMCALL_Set_Global_Time_Out(50, 0, on_timeout);
  VMMCALL_Schedule_Global_Event(0x1234, on_event);
  return 0;
}

static char *put_dec(char *out, DWORD q) {
  char d[12];
  int k = 0;
  do {
    d[k++] = (char)('0' + (int)(q % 10));
    q /= 10;
  } while (q);
  while (k)
    *out++ = d[--k];
  return out;
}

static char *put(char *o, const char *s, DWORD n) {
  while (*s)
    *o++ = *s++;
  return put_dec(o, n);
}

VXD_PAGEABLE_CODE static DWORD ioctl(struct DIOCParams *p) {
  char *out = (char *)p->lpvOutBuffer, *o = out;
  const char *want = "clang-retro";
  DWORD heap_ok = 1;
  unsigned i;
  if (p->dwIoControlCode == DIOC_OPEN || p->dwIoControlCode == DIOC_CLOSEHANDLE)
    return 0;
  if (p->dwIoControlCode != 1 || p->cbOutBuffer < 80)
    return 50; /* ERROR_NOT_SUPPORTED */
  for (i = 0; want[i]; i++)
    if (block[i] != want[i])
      heap_ok = 0;
  o = put(o, "services heap ", heap_ok && _HeapGetSize(block, 0) >= 64);
  o = put(o, " reg ", reg_value);
  o = put(o, " timer ", timer_runs);
  o = put(o, " event ", event_runs);
  o = put(o, " page ", page_value);
  o = put(o, " init ", init_value);
  *o++ = 0;
  if (p->lpcbBytesReturned)
    *(DWORD *)p->lpcbBytesReturned = (DWORD)(o - out);
  return 0;
}

int vxd_control(DWORD msg, struct vxd_regs *r) {
  switch (msg) {
  case SYS_DYNAMIC_DEVICE_INIT:
    return init();
  case SYS_DYNAMIC_DEVICE_EXIT:
    if (timeout)
      VMMCALL_Cancel_Time_Out(timeout);
    if (block)
      _HeapFree(block, 0);
    return 0;
  case W32_DEVICEIOCONTROL:
    r->eax = ioctl((struct DIOCParams *)r->esi);
    return 0;
  default:
    return 0;
  }
}
