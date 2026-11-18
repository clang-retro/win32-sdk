/* The Intel High Definition Audio controller, for hdaudbus.sys and the
   tests: reset, the command rings (CORB and RIRB), and the stream engines
   (stream descriptors) with their buffer descriptor lists. Written from
   Intel's "High Definition Audio Specification", revision 1.0a. */
#include "hda.h"

static void stall(ULONG us) {
  while (us > 50) {
    KeStallExecutionProcessor(50);
    us -= 50;
  }
  KeStallExecutionProcessor(us);
}

/* Wait up to us microseconds for (reg & mask) == want. */
static BOOLEAN wait32(HDA *h, ULONG reg, ULONG mask, ULONG want, ULONG us) {
  for (ULONG t = 0; t <= us; t += 10) {
    if ((hda_rd32(h, reg) & mask) == want)
      return TRUE;
    stall(10);
  }
  return FALSE;
}

static BOOLEAN wait8(HDA *h, ULONG reg, UCHAR mask, UCHAR want, ULONG us) {
  for (ULONG t = 0; t <= us; t += 10) {
    if ((hda_rd8(h, reg) & mask) == want)
      return TRUE;
    stall(10);
  }
  return FALSE;
}

static void unsol_put(HDA *h, ULONG r, ULONG ex) {
  ULONG next = (h->UnsolTail + 1) % HDA_UNSOL_QUEUE;
  if (next == h->UnsolHead)
    return; /* full: dropped */
  h->Unsol[h->UnsolTail][0] = r;
  h->Unsol[h->UnsolTail][1] = ex & 0xF;
  h->UnsolTail = next;
}

/* Send a verb to a codec through the CORB and wait for its response in
   the RIRB; unsolicited responses read meanwhile are queued. FALSE when
   the codec does not answer within 20 ms (after a few of those the
   controller is given up on). */
BOOLEAN hda_command(HDA *h, ULONG verb, ULONG *response) {
  KIRQL irql;
  BOOLEAN got = FALSE;
  *response = 0xFFFFFFFF;
  if (h->Timeouts >= 4)
    return FALSE;
  KeAcquireSpinLock(&h->Lock, &irql);
  ULONG wp = ((hda_rd16(h, HDA_CORBWP) & 0xFF) + 1) % h->CorbEntries;
  h->Corb[wp] = verb;
  hda_wr16(h, HDA_CORBWP, (USHORT)wp);
  for (ULONG t = 0; t < 2000 && !got; t++) {
    ULONG rp = hda_rd16(h, HDA_RIRBWP) & 0xFF;
    while (h->RirbRead != rp) {
      h->RirbRead = (h->RirbRead + 1) % h->RirbEntries;
      ULONG r = h->Rirb[h->RirbRead * 2];
      ULONG ex = h->Rirb[h->RirbRead * 2 + 1];
      if (!(ex & 0x10)) { /* solicited */
        *response = r;
        got = TRUE;
      } else {
        unsol_put(h, r, ex);
      }
    }
    if (!got)
      stall(10);
  }
  /* Clear the response interrupt flag: some controllers stop taking
     commands once RINTCNT responses are unacknowledged. */
  hda_wr8(h, HDA_RIRBSTS, 0x05);
  if (!got)
    h->Timeouts++;
  KeReleaseSpinLock(&h->Lock, irql);
  return got;
}

/* The same, with 0xFFFFFFFF for no response. */
ULONG hda_verb(HDA *h, ULONG verb) {
  ULONG r;
  hda_command(h, verb, &r);
  return r;
}



static ULONG ring_size(UCHAR cap, ULONG *entries) {
  if ((cap & 0x40) || !(cap & 0x30)) {
    *entries = 256;
    return 2;
  }
  if (cap & 0x20) {
    *entries = 16;
    return 1;
  }
  *entries = 2;
  return 0;
}

/* Reset the controller and start its rings; h->Codecs says which codecs
   answered. h->Regs, h->Rings and h->RingsPhys are set. */
NTSTATUS hda_init(HDA *h) {
  KeInitializeSpinLock(&h->Lock);
  USHORT gcap = hda_rd16(h, HDA_GCAP);
  if (gcap == 0xFFFF)
    return STATUS_DEVICE_NOT_CONNECTED;
  h->InputStreams = (gcap >> 8) & 0xF;
  h->OutputStreams = (gcap >> 12) & 0xF;
  h->BidiStreams = (gcap >> 3) & 0x1F;
  h->Version = (USHORT)(hda_rd8(h, HDA_VMAJ) << 8 | hda_rd8(h, HDA_VMIN));
  ULONG engines = h->InputStreams + h->OutputStreams + h->BidiStreams;
  if (!h->OutputStreams || engines > HDA_MAX_ENGINES)
    return STATUS_DEVICE_CONFIGURATION_ERROR;
  /* Input engines then output then bidirectional ones (used as output);
     each has its own 256-byte BDL after the first page. */
  for (ULONG i = 0; i < engines; i++) {
    HDA_ENGINE *e = &h->Engines[i];
    RtlZeroMemory(e, sizeof *e);
    e->Index = i;
    e->Output = i >= h->InputStreams;
    e->Bdl = (HDA_BDL_ENTRY *)((UCHAR *)h->Rings + 4096 +
                               i * HDA_MAX_BDL * sizeof(HDA_BDL_ENTRY));
    e->BdlPhys = h->RingsPhys + 4096 + i * HDA_MAX_BDL * sizeof(HDA_BDL_ENTRY);
  }

  hda_wr32(h, HDA_INTCTL, 0);
  hda_wr8(h, HDA_CORBCTL, 0);
  hda_wr8(h, HDA_RIRBCTL, 0);
  wait8(h, HDA_CORBCTL, HDA_DMA_RUN, 0, 1000);
  wait8(h, HDA_RIRBCTL, HDA_DMA_RUN, 0, 1000);

  /* Controller reset (section 4.2.2), then 521 us or more for the codecs
     to ask for an address. */
  hda_wr32(h, HDA_GCTL, hda_rd32(h, HDA_GCTL) & ~HDA_GCTL_CRST);
  if (!wait32(h, HDA_GCTL, HDA_GCTL_CRST, 0, 100000))
    return STATUS_DEVICE_NOT_READY;
  stall(100);
  hda_wr32(h, HDA_GCTL, hda_rd32(h, HDA_GCTL) | HDA_GCTL_CRST);
  if (!wait32(h, HDA_GCTL, HDA_GCTL_CRST, HDA_GCTL_CRST, 100000))
    return STATUS_DEVICE_NOT_READY;
  stall(1000);
  h->Codecs = hda_rd16(h, HDA_STATESTS) & 0x7FFF;
  hda_wr16(h, HDA_STATESTS, h->Codecs);
  /* Unsolicited responses on. */
  hda_wr32(h, HDA_GCTL, hda_rd32(h, HDA_GCTL) | HDA_GCTL_UNSOL);

  /* The CORB at the start of the page and the RIRB 1 KB in (section
     4.4). */
  h->Corb = (volatile ULONG *)h->Rings;
  h->Rirb = (volatile ULONG *)((UCHAR *)h->Rings + 1024);
  RtlZeroMemory(h->Rings, 4096);
  h->UnsolHead = h->UnsolTail = 0;

  hda_wr8(h, HDA_CORBSIZE,
          (UCHAR)ring_size(hda_rd8(h, HDA_CORBSIZE) >> 4, &h->CorbEntries));
  hda_wr32(h, HDA_CORBLBASE, h->RingsPhys);
  hda_wr32(h, HDA_CORBUBASE, 0);
  hda_wr16(h, HDA_CORBRP, HDA_CORBRP_RST);
  /* The bit reads back as 1 when the reset is done; not all controllers
     (or emulators) do that, so this only waits for it. */
  for (ULONG t = 0; t < 100 && !(hda_rd16(h, HDA_CORBRP) & HDA_CORBRP_RST);
       t++)
    stall(10);
  hda_wr16(h, HDA_CORBRP, 0);
  for (ULONG t = 0; t < 100 && (hda_rd16(h, HDA_CORBRP) & HDA_CORBRP_RST);
       t++)
    stall(10);
  hda_wr16(h, HDA_CORBWP, 0);
  hda_wr8(h, HDA_CORBCTL, HDA_DMA_RUN);

  hda_wr8(h, HDA_RIRBSIZE,
          (UCHAR)ring_size(hda_rd8(h, HDA_RIRBSIZE) >> 4, &h->RirbEntries));
  hda_wr32(h, HDA_RIRBLBASE, h->RingsPhys + 1024);
  hda_wr32(h, HDA_RIRBUBASE, 0);
  hda_wr16(h, HDA_RIRBWP, HDA_RIRBWP_RST);
  hda_wr16(h, HDA_RINTCNT, 0xFF);
  h->RirbRead = 0;
  hda_wr8(h, HDA_RIRBCTL, HDA_DMA_RUN);
  stall(100);
  return h->Codecs ? STATUS_SUCCESS : STATUS_NO_SUCH_DEVICE;
}


/* The next unsolicited response, reading the RIRB for new ones: its
   32 bits and the address of the codec that sent it. */
BOOLEAN hda_unsolicited(HDA *h, ULONG *response, ULONG *codec) {
  KIRQL irql;
  BOOLEAN got = FALSE;
  KeAcquireSpinLock(&h->Lock, &irql);
  ULONG rp = hda_rd16(h, HDA_RIRBWP) & 0xFF;
  while (h->RirbRead != rp) {
    h->RirbRead = (h->RirbRead + 1) % h->RirbEntries;
    ULONG ex = h->Rirb[h->RirbRead * 2 + 1];
    if (ex & 0x10)
      unsol_put(h, h->Rirb[h->RirbRead * 2], ex);
  }
  hda_wr8(h, HDA_RIRBSTS, 0x05);
  if (h->UnsolHead != h->UnsolTail) {
    *response = h->Unsol[h->UnsolHead][0];
    *codec = h->Unsol[h->UnsolHead][1];
    h->UnsolHead = (h->UnsolHead + 1) % HDA_UNSOL_QUEUE;
    got = TRUE;
  }
  KeReleaseSpinLock(&h->Lock, irql);
  return got;
}

void hda_shutdown(HDA *h) {
  if (!h->Regs)
    return;
  for (ULONG i = 0; i < HDA_MAX_ENGINES; i++)
    if (h->Engines[i].Used)
      hda_engine_reset(h, &h->Engines[i]);
  hda_wr32(h, HDA_INTCTL, 0);
  hda_wr8(h, HDA_CORBCTL, 0);
  hda_wr8(h, HDA_RIRBCTL, 0);
  wait8(h, HDA_CORBCTL, HDA_DMA_RUN, 0, 1000);
  wait8(h, HDA_RIRBCTL, HDA_DMA_RUN, 0, 1000);
  hda_wr32(h, HDA_GCTL, hda_rd32(h, HDA_GCTL) & ~HDA_GCTL_CRST);
}

/* A free engine of the direction, with a stream tag no other engine of
   that direction has (tags are per direction, section 3.3.35). Output
   takes the bidirectional engines too. NULL when all are in use. */
HDA_ENGINE *hda_engine_alloc(HDA *h, BOOLEAN output) {
  KIRQL irql;
  HDA_ENGINE *got = NULL;
  ULONG first = output ? h->InputStreams : 0;
  ULONG end = output ? h->InputStreams + h->OutputStreams + h->BidiStreams
                     : h->InputStreams;
  KeAcquireSpinLock(&h->Lock, &irql);
  for (ULONG i = first; i < end && !got; i++)
    if (!h->Engines[i].Used)
      got = &h->Engines[i];
  if (got) {
    for (UCHAR tag = 1; tag < 16; tag++) {
      BOOLEAN taken = FALSE;
      for (ULONG i = first; i < end; i++)
        taken |= h->Engines[i].Used && h->Engines[i].Tag == tag;
      if (!taken) {
        got->Tag = tag;
        break;
      }
    }
    got->Used = TRUE;
    got->Output = output;
    got->Size = 0;
  }
  KeReleaseSpinLock(&h->Lock, irql);
  return got;
}

void hda_engine_free(HDA *h, HDA_ENGINE *e) {
  hda_engine_reset(h, e);
  e->Size = 0;
  e->Used = FALSE;
}

void hda_engine_run(HDA *h, HDA_ENGINE *e, BOOLEAN run) {
  ULONG sd = HDA_SD(e->Index);
  ULONG ctl = hda_rd32(h, sd + HDA_SD_CTL) & 0x00FFFFFF;
  if (run) {
    ctl |= HDA_SD_RUN;
    e->Fresh = FALSE;
  } else {
    ctl &= ~(ULONG)HDA_SD_RUN;
  }
  hda_wr32(h, sd + HDA_SD_CTL, ctl);
  if (!run)
    wait32(h, sd + HDA_SD_CTL, HDA_SD_RUN, 0, 1000);
}

/* Stop and reset the engine; its buffer and format must be set again. */
void hda_engine_reset(HDA *h, HDA_ENGINE *e) {
  ULONG sd = HDA_SD(e->Index);
  hda_engine_run(h, e, FALSE);
  hda_wr32(h, sd + HDA_SD_CTL, HDA_SD_SRST);
  wait32(h, sd + HDA_SD_CTL, HDA_SD_SRST, HDA_SD_SRST, 1000);
  hda_wr32(h, sd + HDA_SD_CTL, 0);
  wait32(h, sd + HDA_SD_CTL, HDA_SD_SRST, 0, 1000);
  hda_wr8(h, sd + HDA_SD_STS, HDA_SD_STS_ALL);
  hda_wr32(h, HDA_INTCTL, hda_rd32(h, HDA_INTCTL) & ~(1u << e->Index));
  e->Fresh = TRUE;
}

/* Set the engine up for a cyclic buffer at bufphys, bufsize bytes, in
   format fmt, in BDL entries of fragsize bytes, each with an interrupt
   when ioc. */
NTSTATUS hda_engine_setup(HDA *h, HDA_ENGINE *e, ULONG bufphys,
                          ULONG bufsize, ULONG fragsize, BOOLEAN ioc,
                          USHORT fmt) {
  ULONG n = 0, off;
  if (!fragsize || fragsize >= bufsize)
    fragsize = bufsize / 2;
  fragsize &= ~127u;
  if (!fragsize || (bufphys & 127))
    return STATUS_INVALID_PARAMETER;
  hda_engine_reset(h, e);
  for (off = 0; off < bufsize && n < HDA_MAX_BDL; off += fragsize, n++) {
    ULONG len = bufsize - off < fragsize ? bufsize - off : fragsize;
    e->Bdl[n].AddressLow = bufphys + off;
    e->Bdl[n].AddressHigh = 0;
    e->Bdl[n].Length = len;
    e->Bdl[n].Flags = ioc ? 1 : 0;
  }
  if (off < bufsize) /* more fragments than entries: the last one grows */
    e->Bdl[n - 1].Length += bufsize - off;
  hda_engine_program(h, e, e->BdlPhys, bufsize, n - 1, ioc, fmt);
  return STATUS_SUCCESS;
}

/* Program a reset engine with a BDL of lvi + 1 entries at bdlphys
   describing a cyclic buffer of cbl bytes. */
void hda_engine_program(HDA *h, HDA_ENGINE *e, ULONG bdlphys, ULONG cbl,
                        ULONG lvi, BOOLEAN ioc, USHORT fmt) {
  ULONG sd = HDA_SD(e->Index);
  e->Size = cbl;
  e->Fmt = fmt;
  hda_wr32(h, sd + HDA_SD_BDPL, bdlphys);
  hda_wr32(h, sd + HDA_SD_BDPU, 0);
  hda_wr32(h, sd + HDA_SD_CBL, cbl);
  hda_wr16(h, sd + HDA_SD_LVI, (USHORT)lvi);
  hda_wr16(h, sd + HDA_SD_FMT, fmt);
  /* Bidirectional engines set DIR (bit 19) for output. */
  hda_wr32(h, sd + HDA_SD_CTL,
           (ULONG)e->Tag << 20 | (ioc ? HDA_SD_IOCE : 0) |
               (e->Output && e->Index >= h->InputStreams + h->OutputStreams
                    ? 1u << 19
                    : 0));
  hda_wr8(h, sd + HDA_SD_STS, HDA_SD_STS_ALL);
  if (ioc)
    hda_wr32(h, HDA_INTCTL, hda_rd32(h, HDA_INTCTL) | HDA_INTCTL_GIE |
                                (1u << e->Index));
}

/* The position in the buffer, in bytes from its start. QEMU's controller
   keeps LPIB over a stream reset, which the specification says clears
   it, until the stream runs again; until then it is 0. */
ULONG hda_engine_position(HDA *h, HDA_ENGINE *e) {
  ULONG lpib = hda_rd32(h, HDA_SD(e->Index) + HDA_SD_LPIB);
  if (!e->Size || e->Fresh)
    return 0;
  return lpib % e->Size;
}

/* The interrupt: a bit for each engine that interrupted, their status
   (SDnSTS) into status[] if it is not NULL and cleared; 0 when the
   interrupt is not the controller's. */
ULONG hda_interrupt(HDA *h, UCHAR *status) {
  ULONG sts = hda_rd32(h, HDA_INTSTS), done = 0;
  if (sts == 0xFFFFFFFF || !(sts & 0x3FFFFFFF))
    return 0;
  for (ULONG i = 0; i < HDA_MAX_ENGINES; i++)
    if (sts & (1u << i)) {
      ULONG sd = HDA_SD(i);
      UCHAR s = hda_rd8(h, sd + HDA_SD_STS) & HDA_SD_STS_ALL;
      hda_wr8(h, sd + HDA_SD_STS, s);
      if (status)
        status[i] = s;
      done |= 1u << i;
    }
  return done;
}

/* The stream format (section 3.7.1) for PCM at a rate, sample size and
   channel count, or 0xFFFF (no PCM format) when it has none for them. */
USHORT hda_format(ULONG rate, ULONG bits, ULONG channels) {
  static const struct {
    ULONG rate;
    USHORT bits;
  } rates[] = {
      {48000, 0x0000}, {44100, 0x4000}, {96000, 0x0800}, {88200, 0x4800},
      {192000, 0x1800}, {176400, 0x5800}, {32000, 0x0A00}, {24000, 0x0100},
      {22050, 0x4100}, {16000, 0x0200}, {11025, 0x4300}, {8000, 0x0500},
  };
  static const UCHAR sizes[] = {8, 16, 20, 24, 32};
  USHORT fmt = 0xFFFF;
  for (ULONG i = 0; i < sizeof rates / sizeof rates[0]; i++)
    if (rates[i].rate == rate)
      fmt = rates[i].bits;
  if (fmt == 0xFFFF || channels < 1 || channels > 16)
    return 0xFFFF;
  for (ULONG i = 0; i < sizeof sizes; i++)
    if (sizes[i] == bits)
      return (USHORT)(fmt | i << 4 | (channels - 1));
  return 0xFFFF;
}
