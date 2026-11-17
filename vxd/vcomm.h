/* vcomm.h: what a VCOMM port driver (a VxD that drives a serial or
   parallel port for Windows 95, 98 and ME) hands to VCOMM and gets from
   it: the table of its port functions (struct PortFunctions), the state
   of an open port (struct PortData), the device control block and the
   other structures those functions take, and their constants.

   A port driver registers with _VCOMM_Register_Port_Driver and adds each
   port with _VCOMM_Add_Port(refdata, PortOpen, name) (<vxdsvc.h>).
   VCOMM calls PortOpen when a client opens the port; it returns the
   port's PortData, whose PDfunctions points to the driver's
   PortFunctions, and VCOMM then calls those functions with it (the C
   calling convention, each returning nonzero for success). The layouts
   are those of the 95, 98 and ME DDKs' vcomm.h (packed: PortData's
   bMSRShadow, wFlags and LossByte share a dword). The Win32 versions of
   the DCB and COMMPROP, in <winbase.h>, are different structures; do not
   include both headers. */
#ifndef VXD_VCOMM_H
#define VXD_VCOMM_H

#include <vxd.h>

#pragma pack(push, 1)

typedef struct _PortData PortData;
typedef struct _DCB _DCB;
typedef struct _COMMPROP _COMMPROP;
typedef struct _COMSTAT _COMSTAT;

/* Called by VCOMM (or a port driver for its clients) on a notification:
   CN_RECEIVE, CN_TRANSMIT or CN_EVENT with the events (EV_*) detected. */
typedef void (*CommNotifyProc)(PortData *port, DWORD refdata, DWORD event,
                               DWORD subevent);
/* Given to _VCOMM_Add_Port: opens the port named name (or NULL for the
   port the driver added), for the VM vm; returns its PortData, or NULL
   with an IE_* code in *error. */
typedef PortData *(*PortOpenProc)(const char *name, DWORD vm, DWORD *error);

typedef struct _PortFunctions {
  int (*pPortSetCommState)(PortData *port, _DCB *dcb,
                           DWORD action /* the f* DCB field masks */);
  int (*pPortGetCommState)(PortData *port, _DCB *dcb);
  int (*pPortSetup)(PortData *port, BYTE *rxqueue, DWORD rxsize,
                    BYTE *txqueue, DWORD txsize);
  int (*pPortTransmitChar)(PortData *port, char ch);
  int (*pPortClose)(PortData *port);
  int (*pPortGetQueueStatus)(PortData *port, _COMSTAT *stat);
  int (*pPortClearError)(PortData *port, _COMSTAT *stat /* or NULL */,
                         DWORD *errors /* CE_* */);
  /* The DDK's documentation gives (port, eventmask, shadow), its
     sample driver and VCOMM's own use (port, shadow). */
  int (*pPortSetModemStatusShadow)(PortData *port, BYTE *shadow);
  int (*pPortGetProperties)(PortData *port, _COMMPROP *props);
  int (*pPortEscapeFunction)(PortData *port, DWORD function /* SETDTR... */,
                             DWORD indata, void *outdata);
  int (*pPortPurge)(PortData *port, DWORD queue /* 0 transmit, 1 receive */);
  int (*pPortSetEventMask)(PortData *port, DWORD mask, DWORD *events);
  int (*pPortGetEventMask)(PortData *port, DWORD mask, DWORD *events);
  int (*pPortWrite)(PortData *port, const char *buf, DWORD count,
                    DWORD *written);
  int (*pPortRead)(PortData *port, char *buf, DWORD count, DWORD *read);
  int (*pPortEnableNotification)(PortData *port, CommNotifyProc proc,
                                 DWORD refdata);
  int (*pPortSetReadCallBack)(PortData *port, DWORD trigger,
                              CommNotifyProc proc, DWORD refdata);
  int (*pPortSetWriteCallBack)(PortData *port, DWORD trigger,
                               CommNotifyProc proc, DWORD refdata);
  int (*pPortGetModemStatus)(PortData *port, DWORD *status /* MS_* */);
  /* Windows 95 and later (not in Windows 3.1's VCOMM). */
  int (*pPortGetCommConfig)(PortData *port, void *config, DWORD *size);
  int (*pPortSetCommConfig)(PortData *port, void *config, DWORD size);
  int (*pPortGetError)(PortData *port, DWORD *error /* a Win32 error */);
  /* Undocumented, and missing from the end of the DDK's sample
     driver's table. Its parameters here are DeviceIoControl's. */
  int (*pPortDeviceIOCtl)(PortData *port, DWORD code, void *in, DWORD insize,
                          void *out, DWORD outsize, DWORD *returned);
} PortFunctions;

typedef struct COMMTIMEOUTS {
  DWORD ReadIntervalTimeout;
  DWORD ReadTotalTimeoutMultiplier;
  DWORD ReadTotalTimeoutConstant;
  DWORD WriteTotalTimeoutMultiplier;
  DWORD WriteTotalTimeoutConstant;
} COMMTIMEOUTS, *LPCOMMTIMEOUTS;

/* An open port: the driver allocates it (usually as the start of its own
   larger structure) and keeps the queue and event fields up to date. */
struct _PortData {
  WORD PDLength;              /* sizeof (PortData) */
  WORD PDVersion;             /* 0x10A */
  PortFunctions *PDfunctions; /* the driver's functions */
  DWORD PDNumFunctions;       /* how many of them it has */
  DWORD dwLastError;          /* IE_* of the last operation */
  DWORD dwClientEventMask;    /* EV_* the client enabled */
  DWORD lpClientEventNotify;  /* the client's CommNotifyProcs */
  DWORD lpClientReadNotify;
  DWORD lpClientWriteNotify;
  DWORD dwClientRefData;
  DWORD dwWin31Req;
  DWORD dwClientEvent;
  DWORD dwCallerVMId;
  DWORD dwDetectedEvents;     /* EV_* detected and enabled */
  DWORD dwCommError;          /* CE_* */
  BYTE bMSRShadow;            /* the modem status register's shadow */
  WORD wFlags;                /* Event_Sent ... */
  BYTE LossByte;              /* VCD sets it when it takes COM1-4 away */
  DWORD QInAddr;              /* the receive queue */
  DWORD QInSize;
  DWORD QOutAddr;             /* the transmit queue */
  DWORD QOutSize;
  DWORD QInCount;             /* bytes in the receive queue */
  DWORD QInGet;               /* offsets to take and put bytes at */
  DWORD QInPut;
  DWORD QOutCount;
  DWORD QOutGet;
  DWORD QOutPut;
  DWORD ValidPortData;
  DWORD lpLoadHandle;         /* the driver's */
  COMMTIMEOUTS cmto;
  DWORD lpReadRequestQueue;   /* pending IORequests */
  DWORD lpWriteRequestQueue;
  DWORD dwLastReceiveTime;
  DWORD dwReserved1;
  DWORD dwReserved2;
};

/* The device control block. */
struct _DCB {
  DWORD DCBLength;   /* sizeof (_DCB) */
  DWORD BaudRate;    /* bits per second, or a CBR_* index */
  DWORD BitMask;     /* fBinary ... */
  DWORD XonLim;      /* send XON below this many bytes in the queue */
  DWORD XoffLim;     /* send XOFF above (queue size - this) */
  WORD wReserved;
  BYTE ByteSize;     /* 4 to 8 */
  BYTE Parity;       /* NOPARITY ... */
  BYTE StopBits;     /* ONESTOPBIT ... */
  char XonChar;
  char XoffChar;
  char ErrorChar;    /* replaces bytes with parity errors */
  char EofChar;
  char EvtChar1;     /* EV_RXFLAG characters */
  char EvtChar2;
  BYTE bReserved;
  DWORD RlsTimeout;  /* milliseconds to wait for RLSD, CTS and DSR */
  DWORD CtsTimeout;
  DWORD DsrTimeout;
  DWORD TxDelay;     /* milliseconds between characters */
};

/* What a port can do (PortGetProperties). */
struct _COMMPROP {
  WORD wPacketLength;
  WORD wPacketVersion;
  DWORD dwServiceMask;      /* SP_SERIALCOMM */
  DWORD dwReserved1;
  DWORD dwMaxTxQueue;       /* 0: no limit */
  DWORD dwMaxRxQueue;
  DWORD dwMaxBaud;          /* BAUD_* */
  DWORD dwProvSubType;      /* PST_* */
  DWORD dwProvCapabilities; /* PCF_* */
  DWORD dwSettableParams;   /* SP_* */
  DWORD dwSettableBaud;     /* BAUD_* */
  WORD wSettableData;       /* DATABITS_* */
  WORD wSettableStopParity; /* STOPBITS_*, PARITY_* */
  DWORD dwCurrentTxQueue;
  DWORD dwCurrentRxQueue;
  DWORD dwProvSpec1;
  DWORD dwProvSpec2;
  WORD wcProvChar[1];
  WORD filler;
};

struct _COMSTAT {
  DWORD BitMask;  /* fCtsHold ... */
  DWORD cbInque;  /* bytes in the receive queue */
  DWORD cbOutque; /* bytes in the transmit queue */
};

typedef struct _QSB {
  DWORD ReceiveQueueGetPtr;
  DWORD ReceiveQueueCount;
} _QSB;

/* A read or write request queued with _VCOMM_SetReadRequest or
   _VCOMM_SetWriteRequest. */
typedef struct IORequest {
  DWORD IOBuffer;                /* the buffer, in locked memory */
  DWORD IOCount;                 /* its size */
  DWORD IOCallCompletionRoutine; /* called when done */
  DWORD NumBytes;                /* bytes read or written */
  DWORD dwReserved[8];           /* VCOMM's */
  DWORD dwSig;
} IORequest;

#define WRITE_REQUEST 1
#define READ_REQUEST 0

#pragma pack(pop)

/* dwLastError and PortOpen's errors. */
#define IE_BADID (-1)
#define IE_OPEN (-2)
#define IE_NOPEN (-3)
#define IE_MEMORY (-4)
#define IE_DEFAULT (-5)
#define IE_INVALIDSERVICE (-6)
#define IE_HARDWARE (-10)
#define IE_BYTESIZE (-11)
#define IE_BAUDRATE (-12)
#define IE_EXTINVALID (-20)
#define IE_INVALIDPARAM (-21)
#define IE_TRANSMITCHARFAILED (-30) /* 0x4000 in the 95 DDK */
#define IE_DEVICEID (-31)

/* Events (dwClientEventMask, dwDetectedEvents, CN_EVENT's subevent). */
#define EV_RXCHAR 0x00000001UL
#define EV_RXFLAG 0x00000002UL
#define EV_TXEMPTY 0x00000004UL
#define EV_CTS 0x00000008UL
#define EV_DSR 0x00000010UL
#define EV_RLSD 0x00000020UL
#define EV_BREAK 0x00000040UL
#define EV_ERR 0x00000080UL
#define EV_RING 0x00000100UL
#define EV_PERR 0x00000200UL
#define EV_CTSS 0x00000400UL
#define EV_DSRS 0x00000800UL
#define EV_RLSDS 0x00001000UL
#define EV_RingTe 0x00002000UL
#define EV_TXCHAR 0x00004000UL
#define EV_DRIVER 0x00008000UL
#define EV_UNAVAIL 0x00010000UL
#define EV_AVAIL 0x00020000UL

/* Errors (dwCommError, PortClearError). */
#define CE_RXOVER 0x00000001UL
#define CE_OVERRUN 0x00000002UL
#define CE_RXPARITY 0x00000004UL
#define CE_FRAME 0x00000008UL
#define CE_BREAK 0x00000010UL
#define CE_CTSTO 0x00000020UL
#define CE_DSRTO 0x00000040UL
#define CE_RLSDTO 0x00000080UL
#define CE_TXFULL 0x00000100UL
#define CE_PTO 0x00000200UL
#define CE_IOE 0x00000400UL
#define CE_DNS 0x00000800UL
#define CE_OOP 0x00001000UL
#define CE_MODE 0x00008000UL

/* Modem status (bMSRShadow, PortGetModemStatus). */
#define MS_CTS_ON 0x0010
#define MS_DSR_ON 0x0020
#define MS_RING_ON 0x0040
#define MS_RLSD_ON 0x0080
#define MS_Modem_Status 0x00F0

/* wFlags. */
#define Event_Sent 0x0001
#define TimeOut_Error 0x0002
#define Flush_In_Progress 0x0004
#define TxQueuesSet 0x0008
#define CloseComm_In_Progress 0x0010
#define Spec_Timeouts_Set 0x0020

/* _DCB BaudRate indexes. */
#define CBR_110 0x0000FF10UL
#define CBR_300 0x0000FF11UL
#define CBR_600 0x0000FF12UL
#define CBR_1200 0x0000FF13UL
#define CBR_2400 0x0000FF14UL
#define CBR_4800 0x0000FF15UL
#define CBR_9600 0x0000FF16UL
#define CBR_14400 0x0000FF17UL
#define CBR_19200 0x0000FF18UL
#define CBR_38400 0x0000FF1BUL
#define CBR_56000 0x0000FF1FUL
#define CBR_128000 0x0000FF23UL
#define CBR_256000 0x0000FF27UL

/* _DCB BitMask. */
#define fBinary 0x00000001UL
#define fRTSDisable 0x00000002UL
#define fParity 0x00000004UL
#define fOutXCTSFlow 0x00000008UL
#define fOutXDSRFlow 0x00000010UL
#define fEnqAck 0x00000020UL
#define fEtxAck 0x00000040UL
#define fDTRDisable 0x00000080UL
#define fOutX 0x00000100UL
#define fInX 0x00000200UL
#define fPErrChar 0x00000400UL
#define fNullStrip 0x00000800UL
#define fCharEvent 0x00001000UL
#define fDTRFlow 0x00002000UL
#define fRTSFlow 0x00004000UL
#define fWin30Compat 0x00008000UL
#define fDsrSensitivity 0x00010000UL
#define fTxContinueOnXoff 0x00020000UL
#define fDtrEnable 0x00040000UL
#define fAbortOnError 0x00080000UL
#define fRTSEnable 0x00100000UL
#define fRTSToggle 0x00200000UL

/* _DCB Parity and StopBits. */
#define NOPARITY 0
#define ODDPARITY 1
#define EVENPARITY 2
#define MARKPARITY 3
#define SPACEPARITY 4
#define ONESTOPBIT 0
#define ONE5STOPBITS 1
#define TWOSTOPBITS 2

/* PortSetCommState's action: the _DCB fields to apply. */
#define fBaudRate 0x00000001UL
#define fBitMask 0x00000002UL
#define fXonLim 0x00000004UL
#define fXoffLim 0x00000008UL
#define fByteSize 0x00000010UL
#define fbParity 0x00000020UL
#define fStopBits 0x00000040UL
#define fXonChar 0x00000080UL
#define fXoffChar 0x00000100UL
#define fErrorChar 0x00000200UL
#define fEofChar 0x00000400UL
#define fEvtChar1 0x00000800UL
#define fEvtChar2 0x00001000UL
#define fRlsTimeout 0x00002000UL
#define fCtsTimeout 0x00004000UL
#define fDsrTimeout 0x00008000UL
#define fTxDelay 0x00010000UL
#define fTimeout (fRlsTimeout | fDsrTimeout | fCtsTimeout)
#define fLCR 0x00000070UL

/* _COMMPROP. */
#define SP_SERIALCOMM 0x00000001UL
#define PST_UNSPECIFIED 0x00000000UL
#define PST_RS232 0x00000001UL
#define PST_PARALLELPORT 0x00000002UL
#define PST_RS422 0x00000003UL
#define PST_RS423 0x00000004UL
#define PST_RS449 0x00000005UL
#define PST_MODEM 0x00000006UL
#define PST_FAX 0x00000021UL
#define PST_SCANNER 0x00000022UL
#define PST_NETWORK_BRIDGE 0x00000100UL
#define PST_LAT 0x00000101UL
#define PST_TCPIP_TELNET 0x00000102UL
#define PST_X25 0x00000103UL
#define PCF_DTRDSR 0x0001UL
#define PCF_RTSCTS 0x0002UL
#define PCF_RLSD 0x0004UL
#define PCF_PARITY_CHECK 0x0008UL
#define PCF_XONXOFF 0x0010UL
#define PCF_SETXCHAR 0x0020UL
#define PCF_TOTALTIMEOUTS 0x0040UL
#define PCF_INTTIMEOUTS 0x0080UL
#define PCF_SPECIALCHARS 0x0100UL
#define PCF_16BITMODE 0x0200UL
#define SP_PARITY 0x0001UL
#define SP_BAUD 0x0002UL
#define SP_DATABITS 0x0004UL
#define SP_STOPBITS 0x0008UL
#define SP_HANDSHAKING 0x0010UL
#define SP_PARITY_CHECK 0x0020UL
#define SP_RLSD 0x0040UL
#define BAUD_075 0x00000001UL
#define BAUD_110 0x00000002UL
#define BAUD_134_5 0x00000004UL
#define BAUD_150 0x00000008UL
#define BAUD_300 0x00000010UL
#define BAUD_600 0x00000020UL
#define BAUD_1200 0x00000040UL
#define BAUD_1800 0x00000080UL
#define BAUD_2400 0x00000100UL
#define BAUD_4800 0x00000200UL
#define BAUD_7200 0x00000400UL
#define BAUD_9600 0x00000800UL
#define BAUD_14400 0x00001000UL
#define BAUD_19200 0x00002000UL
#define BAUD_38400 0x00004000UL
#define BAUD_56K 0x00008000UL
#define BAUD_128K 0x00010000UL
#define BAUD_USER 0x10000000UL
#define DATABITS_5 0x0001
#define DATABITS_6 0x0002
#define DATABITS_7 0x0004
#define DATABITS_8 0x0008
#define DATABITS_16 0x0010
#define DATABITS_16X 0x0020
#define STOPBITS_10 0x0001
#define STOPBITS_15 0x0002
#define STOPBITS_20 0x0004
#define PARITY_NONE 0x0100
#define PARITY_ODD 0x0200
#define PARITY_EVEN 0x0400
#define PARITY_MARK 0x0800
#define PARITY_SPACE 0x1000

/* _COMSTAT BitMask. */
#define fCtsHold 0x00000001UL
#define fDsrHold 0x00000002UL
#define fRlsdHold 0x00000004UL
#define fXoffHold 0x00000008UL
#define fXoffSent 0x00000010UL
#define fEof 0x00000020UL
#define fTximmed 0x00000040UL

/* PortEscapeFunction's functions (0-199 are Microsoft's). */
#define Dummy 0
#define SETXOFF 1
#define SETXON 2
#define SETRTS 3
#define CLRRTS 4
#define SETDTR 5
#define CLRDTR 6
#define RESETDEV 7
#define GETCOMBASEIRQ 10
#define GETCOMBASEIRQ1 11
#define SETBREAK 12
#define CLEARBREAK 13
#define CLRTIMERLOGIC 16
#define GETDEVICEID 17
#define SETECPADDRESS 18
#define SETUPDATETIMEADDR 19
#define IGNOREERRORONREADS 20
#define ENABLETIMERLOGIC 21
#define IGNORESELECT 22
#define PM_QUERY_D1 28 /* power management (98 and ME) */
#define PM_QUERY_D2 29
#define PM_QUERY_D3 30
#define PM_CANCEL_D1 31
#define PM_CANCEL_D2 32
#define PM_CANCEL_D3 33
#define PM_SET_D0 34
#define PM_SET_D1 35
#define PM_SET_D2 36
#define PM_SET_D3 37
#define PM_ARM_WAKEUP 38
#define PM_DISARM_WAKEUP 39
#define STARTNONSTDESCAPES 200
#define PEEKCHAR 200
#define PM_SUCCESS 0
#define PM_FAILURE 1

/* CommNotifyProc's event. */
#define CN_RECEIVE 1
#define CN_TRANSMIT 2
#define CN_EVENT 4

/* Port driver inquiry results. */
#define PD_UNSUPPORTED 0
#define PD_DEFAULT 1
#define PD_BEST (-1)

#endif
