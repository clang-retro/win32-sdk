/* A dynamic VxD whose DeviceIoControl finishes later. For code 1 with
   an OVERLAPPED, it locks the output buffer and the OVERLAPPED with
   vxd_lock_global, returns VXD_DIOC_PENDING (DeviceIoControl returns
   FALSE, ERROR_IO_PENDING), and 200 ms later, from a time-out (another
   context), writes its answer and calls VWIN32_DIOCCompletion, which ends
   the program's GetOverlappedResult. Without an OVERLAPPED it answers at
   once. asyncc.c is the program; it writes what it saw:

     async pending 1 bytes 13 text async done 1 sync

   (pending: DeviceIoControl returned ERROR_IO_PENDING; bytes: what
   GetOverlappedResult said; text: the answer, from the time-out; sync:
   the answer of the call without an OVERLAPPED.) */
#include <vxd.h>

VXD_DEVICE(ASYNC, UNDEFINED_DEVICE_ID, UNDEFINED_INIT_ORDER, 1, 0);

/* The request being finished: global addresses, valid in any context. */
static struct vxd_overlapped *pending;
static char *out;
static DWORD out_size, done;

static DWORD answer(char *buf, DWORD size, const char *text) {
  DWORD n = 0;
  while (n + 1 < size && text[n]) {
    buf[n] = text[n];
    n++;
  }
  buf[n++] = 0;
  return n;
}

VXD_CALLBACK(on_timeout, timeout_fn);
int timeout_fn(struct vxd_regs *r) {
  struct vxd_overlapped *o = pending;
  DWORD n;
  (void)r;
  if (!o)
    return 0;
  done++;
  n = answer(out, out_size, done == 1 ? "async done 1" : "async done 2+");
  pending = 0;
  VWIN32_DIOCCompletion(o, n);
  vxd_unlock_global(out, out_size);
  vxd_unlock_global(o, sizeof *o);
  return 0;
}

static DWORD ioctl(struct DIOCParams *p) {
  if (p->dwIoControlCode == DIOC_OPEN ||
      p->dwIoControlCode == DIOC_CLOSEHANDLE)
    return 0;
  if (p->dwIoControlCode != 1 || p->cbOutBuffer < 16)
    return 50; /* ERROR_NOT_SUPPORTED */
  if (!p->lpoOverlapped) {
    DWORD n = answer((char *)p->lpvOutBuffer, p->cbOutBuffer, "sync");
    if (p->lpcbBytesReturned)
      *(DWORD *)p->lpcbBytesReturned = n;
    return 0;
  }
  if (pending)
    return 170; /* ERROR_BUSY: one request at a time */
  out_size = p->cbOutBuffer;
  out = vxd_lock_global(p->lpvOutBuffer, out_size);
  pending = vxd_lock_global(p->lpoOverlapped, sizeof *pending);
  if (!out || !pending) {
    vxd_unlock_global(out, out_size);
    vxd_unlock_global(pending, sizeof *pending);
    pending = 0;
    return 8; /* ERROR_NOT_ENOUGH_MEMORY */
  }
  if (!VMMCALL_Set_Global_Time_Out(200, 0, on_timeout)) {
    vxd_unlock_global(out, out_size);
    vxd_unlock_global(pending, sizeof *pending);
    pending = 0;
    return 8;
  }
  return VXD_DIOC_PENDING;
}

int vxd_control(DWORD msg, struct vxd_regs *r) {
  if (msg == W32_DEVICEIOCONTROL)
    r->eax = ioctl((struct DIOCParams *)r->esi);
  return 0;
}
