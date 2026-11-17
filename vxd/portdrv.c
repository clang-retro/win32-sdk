/* A VCOMM port driver for a loopback port: what is written to it can be
   read back. VCOMM loads it (as the devloader of a devnode whose
   PortDriver value names PORTDRV.VXD), it registers with
   _VCOMM_Register_Port_Driver, and VCOMM calls port_init for the port,
   which adds it. run-tests.sh builds it with each level of optimization
   and does not load it; the asserts check <vcomm.h>'s layouts against the
   DDK's. */
#include <vxd.h>
#include <vcomm.h>
#include <stddef.h>

#define A(e) _Static_assert(e, #e)
A(offsetof(PortData, bMSRShadow) == 0x38);
A(offsetof(PortData, wFlags) == 0x39);
A(offsetof(PortData, LossByte) == 0x3B);
A(offsetof(PortData, QInAddr) == 0x3C);
A(offsetof(PortData, cmto) == 0x6C);
A(offsetof(PortData, lpReadRequestQueue) == 0x80);
A(sizeof(PortData) == 0x94);
A(sizeof(PortFunctions) == 23 * 4);
A(offsetof(_DCB, ByteSize) == 22);
A(sizeof(_DCB) == 48);
A(offsetof(_COMMPROP, wcProvChar) == 0x3C);
A(sizeof(_COMMPROP) == 0x40);
A(sizeof(_COMSTAT) == 12);
A(sizeof(COMMTIMEOUTS) == 20);
A(sizeof(IORequest) == 0x34);

VXD_DEVICE(PORTDRV, UNDEFINED_DEVICE_ID, UNDEFINED_INIT_ORDER, 1, 0);

/* The port: PortData first, so VCOMM's handle is the whole structure. */
static struct loop {
  PortData pd;
  _DCB dcb;
  int open;
  char name[16];
  CommNotifyProc notify;
  DWORD refdata;
  char buf[256];
} port;

static int set_state(PortData *p, _DCB *dcb, DWORD action) {
  struct loop *l = (struct loop *)p;
  if (action & fBaudRate) l->dcb.BaudRate = dcb->BaudRate;
  if (action & fByteSize) l->dcb.ByteSize = dcb->ByteSize;
  if (action & fParity) l->dcb.Parity = dcb->Parity;
  if (action & fStopBits) l->dcb.StopBits = dcb->StopBits;
  if (action & fBitMask) l->dcb.BitMask = dcb->BitMask;
  return 1;
}

static int get_state(PortData *p, _DCB *dcb) {
  *dcb = ((struct loop *)p)->dcb;
  return 1;
}

static int setup(PortData *p, BYTE *rx, DWORD rxsize, BYTE *tx,
                 DWORD txsize) {
  (void)tx, (void)txsize;
  if (rx) {
    p->QInAddr = (DWORD)rx;
    p->QInSize = rxsize;
  }
  p->QInCount = p->QInGet = p->QInPut = 0;
  return 1;
}

/* Puts a byte in the receive queue, as an interrupt handler would. */
static void receive(PortData *p, char ch) {
  struct loop *l = (struct loop *)p;
  if (p->QInCount == p->QInSize) {
    p->dwCommError |= CE_RXOVER;
    return;
  }
  ((char *)p->QInAddr)[p->QInPut] = ch;
  p->QInPut = (p->QInPut + 1) % p->QInSize;
  p->QInCount++;
  p->dwDetectedEvents |= EV_RXCHAR & p->dwClientEventMask;
  if (l->notify)
    l->notify(p, l->refdata, CN_RECEIVE, 0);
}

static int transmit_char(PortData *p, char ch) {
  receive(p, ch);
  return 1;
}

static int write(PortData *p, const char *buf, DWORD n, DWORD *done) {
  for (*done = 0; *done < n; ++*done)
    receive(p, buf[*done]);
  p->dwDetectedEvents |= EV_TXEMPTY & p->dwClientEventMask;
  return 1;
}

static int read(PortData *p, char *buf, DWORD n, DWORD *done) {
  for (*done = 0; *done < n && p->QInCount; ++*done) {
    buf[*done] = ((char *)p->QInAddr)[p->QInGet];
    p->QInGet = (p->QInGet + 1) % p->QInSize;
    p->QInCount--;
  }
  return 1;
}

static int close(PortData *p) {
  ((struct loop *)p)->open = 0;
  return 1;
}

static int queue_status(PortData *p, _COMSTAT *s) {
  s->BitMask = 0;
  s->cbInque = p->QInCount;
  s->cbOutque = 0;
  return 1;
}

static int clear_error(PortData *p, _COMSTAT *s, DWORD *errors) {
  if (s)
    queue_status(p, s);
  *errors = p->dwCommError;
  p->dwCommError = 0;
  return 1;
}

static int set_shadow(PortData *p, BYTE *shadow) {
  (void)p, (void)shadow;
  return 1;
}

static int properties(PortData *p, _COMMPROP *c) {
  (void)p;
  *c = (_COMMPROP){.wPacketLength = sizeof *c, .wPacketVersion = 2,
                    .dwServiceMask = SP_SERIALCOMM};
  c->dwMaxBaud = BAUD_USER;
  c->dwProvSubType = PST_RS232;
  c->dwSettableParams = SP_BAUD | SP_DATABITS | SP_PARITY | SP_STOPBITS;
  c->dwSettableBaud = BAUD_9600 | BAUD_19200 | BAUD_USER;
  c->wSettableData = DATABITS_7 | DATABITS_8;
  c->wSettableStopParity = STOPBITS_10 | PARITY_NONE | PARITY_EVEN;
  c->dwCurrentRxQueue = p->QInSize;
  return 1;
}

static int escape(PortData *p, DWORD function, DWORD in, void *out) {
  (void)p, (void)in, (void)out;
  return function == SETDTR || function == CLRDTR || function == SETRTS ||
         function == CLRRTS;
}

static int purge(PortData *p, DWORD queue) {
  if (queue)
    p->QInCount = p->QInGet = p->QInPut = 0;
  return 1;
}

static int set_events(PortData *p, DWORD mask, DWORD *events) {
  p->dwClientEventMask = mask;
  p->dwDetectedEvents &= mask;
  if (events)
    *events = p->dwDetectedEvents;
  return 1;
}

static int get_events(PortData *p, DWORD mask, DWORD *events) {
  *events = p->dwDetectedEvents & mask;
  p->dwDetectedEvents &= ~mask;
  return 1;
}

static int notification(PortData *p, CommNotifyProc proc, DWORD refdata) {
  struct loop *l = (struct loop *)p;
  l->notify = proc;
  l->refdata = refdata;
  return 1;
}

static int callback(PortData *p, DWORD trigger, CommNotifyProc proc,
                    DWORD refdata) {
  (void)trigger;
  return notification(p, proc, refdata);
}

static int modem_status(PortData *p, DWORD *status) {
  (void)p;
  *status = MS_CTS_ON | MS_DSR_ON | MS_RLSD_ON; /* a null modem */
  return 1;
}

static int get_error(PortData *p, DWORD *error) {
  *error = p->dwLastError ? 31 /* ERROR_GEN_FAILURE */ : 0;
  return 1;
}

static PortFunctions functions = {
    set_state,    get_state,     setup,        transmit_char, close,
    queue_status, clear_error,   set_shadow,   properties,    escape,
    purge,        set_events,    get_events,   write,         read,
    notification, callback,      callback,     modem_status,  0,
    0,            get_error,     0};

static PortData *open_port(const char *name, DWORD vm, DWORD *error) {
  PortData *p = &port.pd;
  (void)name;
  if (port.open) {
    *error = (DWORD)IE_OPEN;
    return 0;
  }
  port.open = 1;
  *p = (PortData){.PDLength = sizeof *p, .PDVersion = 0x10A,
                  .PDfunctions = &functions,
                  .PDNumFunctions = sizeof functions / sizeof(void *)};
  p->dwCallerVMId = vm;
  p->QInAddr = (DWORD)port.buf;
  p->QInSize = sizeof port.buf;
  port.dcb = (_DCB){.DCBLength = sizeof port.dcb, .BaudRate = 9600,
                     .BitMask = fBinary, .ByteSize = 8};
  port.notify = 0;
  return p;
}

/* VCOMM calls it for each devnode whose port driver this is. */
static void port_init(DWORD devnode, DWORD base, DWORD irq, char *name) {
  int i;
  (void)devnode, (void)base, (void)irq;
  for (i = 0; i < 15 && name[i]; i++)
    port.name[i] = name[i];
  _VCOMM_Add_Port(0, (void *)open_port, port.name);
}

int vxd_control(DWORD msg, struct vxd_regs *r) {
  (void)r;
  if (msg == SYS_DYNAMIC_DEVICE_INIT)
    _VCOMM_Register_Port_Driver((void *)port_init);
  return 0;
}
