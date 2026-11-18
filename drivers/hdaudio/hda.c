/* The Intel High Definition Audio controller and codec for hdaudio.sys:
   reset, the command rings (CORB and RIRB), the codec's widget graph and
   an output path through it, and one output stream with its buffer
   descriptor list. Written from Intel's "High Definition Audio
   Specification", revision 1.0a. */
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

/* Send a verb to the codec through the CORB and wait for its response in
   the RIRB. Unsolicited responses are skipped. 0xFFFFFFFF when the codec
   does not answer within 20 ms; after a few of those it is given up on. */
#ifndef HDA_FAKE_CODEC /* test/graphtest.c has its own */
ULONG hda_verb(HDA *h, ULONG verb) {
  KIRQL irql;
  ULONG resp = 0xFFFFFFFF;
  BOOLEAN got = FALSE;
  if (h->Timeouts >= 4)
    return resp;
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
        resp = r;
        got = TRUE;
      }
    }
    if (!got)
      stall(10);
  }
  /* Clear the response interrupt flag: some controllers stop taking
     commands once RINTCNT responses are unacknowledged. */
  hda_wr8(h, HDA_RIRBSTS, 0x05);
  KeReleaseSpinLock(&h->Lock, irql);
  if (!got)
    h->Timeouts++;
  return resp;
}
#endif

static ULONG param(HDA *h, UCHAR nid, ULONG p) {
  return hda_verb(h, HDA_VERB(h->Cad, nid, HDA_GET_PARAMETER, p));
}

static ULONG cmd(HDA *h, UCHAR nid, ULONG verb, ULONG payload) {
  return hda_verb(h, HDA_VERB(h->Cad, nid, verb, payload));
}

static HDA_WIDGET *node(HDA *h, ULONG nid) {
  if (nid < h->NodeStart || nid >= (ULONG)h->NodeStart + h->NodeCount)
    return NULL;
  return &h->Nodes[nid - h->NodeStart];
}

/* Set an amplifier: both channels, the gain of each, and the mute. */
static void set_amp(HDA *h, UCHAR nid, BOOLEAN out, UCHAR index, ULONG left,
                    ULONG right, BOOLEAN mute) {
  ULONG p = (out ? HDA_AMP_OUT : HDA_AMP_IN) | ((ULONG)index << 8) |
            (mute ? HDA_AMP_MUTE : 0);
  hda_verb(h, HDA_VERB4(h->Cad, nid, HDA_SET_AMP,
                        p | HDA_AMP_LEFT | (left & 0x7F)));
  hda_verb(h, HDA_VERB4(h->Cad, nid, HDA_SET_AMP,
                        p | HDA_AMP_RIGHT | (right & 0x7F)));
}

static ULONG zero_db(ULONG caps) {
  ULONG g = HDA_AMPCAP_OFFSET(caps);
  return g > HDA_AMPCAP_STEPS(caps) ? HDA_AMPCAP_STEPS(caps) : g;
}

static void read_connections(HDA *h, UCHAR nid, HDA_WIDGET *w) {
  ULONG len = param(h, nid, HDA_PAR_CONN_LIST_LEN);
  if (len == 0xFFFFFFFF)
    return;
  BOOLEAN lng = (len & 0x80) != 0;
  ULONG per = lng ? 2 : 4, bits = lng ? 16 : 8;
  ULONG range = 1u << (bits - 1), mask = (1u << bits) - 1;
  ULONG resp = 0, prev = 0;
  len &= 0x7F;
  for (ULONG i = 0; i < len; i++) {
    if (i % per == 0)
      resp = cmd(h, nid, HDA_GET_CONN_LIST, i);
    ULONG e = (resp >> ((i % per) * bits)) & mask;
    ULONG first = e & range ? prev + 1 : e & ~range;
    e &= ~range;
    for (ULONG x = first; x <= e && w->ConnCount < HDA_MAX_CONNS; x++)
      w->Conns[w->ConnCount++] = (UCHAR)x;
    prev = e;
  }
}

/* Depth-first search from a pin through mixers and selectors for an
   analog output converter (DAC), dac if it is not 0. The path goes into
   path, from the pin to the DAC. */
static BOOLEAN find_dac(HDA *h, UCHAR nid, ULONG depth, UCHAR dac,
                        HDA_PATH_NODE *path, ULONG *length) {
  HDA_WIDGET *w = node(h, nid);
  if (!w)
    return FALSE;
  ULONG type = HDA_WCAP_TYPE(w->Caps);
  path[depth].Nid = nid;
  path[depth].Index = 0;
  if (depth > 0 && type == HDA_WIDGET_OUTPUT) {
    if ((w->Caps & HDA_WCAP_DIGITAL) || (dac && nid != dac))
      return FALSE;
    *length = depth + 1;
    return TRUE;
  }
  if (depth + 1 >= HDA_MAX_PATH)
    return FALSE;
  if (depth > 0 && type != HDA_WIDGET_MIXER && type != HDA_WIDGET_SELECTOR)
    return FALSE;
  for (ULONG i = 0; i < w->ConnCount; i++) {
    path[depth].Index = (UCHAR)i;
    if (find_dac(h, w->Conns[i], depth + 1, dac, path, length))
      return TRUE;
  }
  return FALSE;
}

/* Power up and open each widget on the path: select its input, unmute
   the amplifiers along it at 0 dB, and enable the pin's output. */
static void open_path(HDA *h, const HDA_PATH_NODE *path, ULONG length) {
  for (ULONG k = 0; k < length; k++) {
    UCHAR nid = path[k].Nid, in = path[k].Index;
    HDA_WIDGET *w = node(h, nid);
    ULONG type = HDA_WCAP_TYPE(w->Caps);
    if (w->Caps & HDA_WCAP_POWER)
      cmd(h, nid, HDA_SET_POWER_STATE, 0);
    if (k + 1 < length) {
      if (w->ConnCount > 1 && type != HDA_WIDGET_MIXER)
        cmd(h, nid, HDA_SET_CONN_SELECT, in);
      if (w->InAmp) {
        ULONG g = zero_db(w->InAmp);
        if (type == HDA_WIDGET_MIXER)
          for (ULONG j = 0; j < w->ConnCount; j++)
            set_amp(h, nid, FALSE, (UCHAR)j, g, g, j != in);
        else
          set_amp(h, nid, FALSE, in, g, g, FALSE);
      }
    }
    if (w->OutAmp) {
      ULONG g = zero_db(w->OutAmp);
      set_amp(h, nid, TRUE, 0, g, g, FALSE);
    }
    if (type == HDA_WIDGET_PIN) {
      ULONG ctl = HDA_PINCTL_OUT;
      if ((w->PinCaps & HDA_PINCAP_HP) &&
          HDA_CFG_DEVICE(w->Config) == HDA_DEV_HP_OUT)
        ctl |= HDA_PINCTL_HP;
      cmd(h, nid, HDA_SET_PIN_CONTROL, ctl);
      if (w->PinCaps & HDA_PINCAP_EAPD)
        cmd(h, nid, HDA_SET_EAPD_BTL, 0x02);
    }
  }
}

static BOOLEAN output_pin(HDA_WIDGET *w, BOOLEAN connected) {
  return HDA_WCAP_TYPE(w->Caps) == HDA_WIDGET_PIN &&
         (w->PinCaps & HDA_PINCAP_OUT) && !(w->Caps & HDA_WCAP_DIGITAL) &&
         (!connected || HDA_CFG_PORT(w->Config) != 1);
}

/* The first output pin that reaches a DAC, line out before speaker before
   headphones before the rest, and pins the configuration says are not
   connected last; then every other connected output pin that reaches
   the same DAC. */
static BOOLEAN choose_path(HDA *h) {
  static const ULONG order[] = {HDA_DEV_LINE_OUT, HDA_DEV_SPEAKER,
                                HDA_DEV_HP_OUT, 0xFF};
  HDA_PATH_NODE other[HDA_MAX_PATH];
  ULONG len;
  h->PathLength = 0;
  for (ULONG pass = 0; pass < 5 && !h->PathLength; pass++)
    for (ULONG i = 0; i < h->NodeCount && !h->PathLength; i++) {
      HDA_WIDGET *w = &h->Nodes[i];
      if (!output_pin(w, pass < 4))
        continue;
      if (pass < 3 && HDA_CFG_DEVICE(w->Config) != order[pass])
        continue;
      if (find_dac(h, (UCHAR)(h->NodeStart + i), 0, 0, h->Path, &len))
        h->PathLength = len;
    }
  if (!h->PathLength)
    return FALSE;
  h->Dac = h->Path[h->PathLength - 1].Nid;
  h->Pins[0] = h->Path[0].Nid;
  h->PinCount = 1;
  open_path(h, h->Path, h->PathLength);
  for (ULONG i = 0; i < h->NodeCount && h->PinCount < HDA_MAX_PINS; i++) {
    UCHAR nid = (UCHAR)(h->NodeStart + i);
    if (nid == h->Pins[0] || !output_pin(&h->Nodes[i], TRUE))
      continue;
    if (find_dac(h, nid, 0, h->Dac, other, &len)) {
      open_path(h, other, len);
      h->Pins[h->PinCount++] = nid;
    }
  }
  return TRUE;
}

/* The amplifiers for the volume and the mute: the first on the path from
   the DAC towards the pin that has steps, and that can mute. */
static void choose_amps(HDA *h) {
  BOOLEAN vol = FALSE, mute = FALSE;
  for (LONG k = (LONG)h->PathLength - 1; k >= 0; k--) {
    HDA_WIDGET *w = node(h, h->Path[k].Nid);
    for (int out = 0; out < 2; out++) {
      ULONG caps = out ? w->OutAmp : w->InAmp;
      UCHAR index = out ? 0 : h->Path[k].Index;
      if (!caps || (!out && (ULONG)k + 1 >= h->PathLength))
        continue;
      if (!vol && HDA_AMPCAP_STEPS(caps)) {
        vol = TRUE;
        h->VolNid = h->Path[k].Nid;
        h->VolIndex = index;
        h->VolIsOutput = (UCHAR)out;
        h->VolCaps = caps;
      }
      if (!mute && (caps & HDA_AMPCAP_MUTE)) {
        mute = TRUE;
        h->MuteNid = h->Path[k].Nid;
        h->MuteIndex = index;
        h->MuteIsOutput = (UCHAR)out;
      }
    }
  }
}

static NTSTATUS probe_codec(HDA *h, ULONG cad) {
  h->Cad = cad;
  h->Vendor = param(h, 0, HDA_PAR_VENDOR_ID);
  if (h->Vendor == 0xFFFFFFFF || h->Vendor == 0)
    return STATUS_NO_SUCH_DEVICE;
  ULONG n = param(h, 0, HDA_PAR_NODE_COUNT);
  h->AfgNid = 0;
  for (ULONG fg = (n >> 16) & 0xFF; fg < ((n >> 16) & 0xFF) + (n & 0xFF);
       fg++)
    if ((param(h, (UCHAR)fg, HDA_PAR_FG_TYPE) & 0xFF) == HDA_FG_AUDIO) {
      h->AfgNid = (UCHAR)fg;
      break;
    }
  if (!h->AfgNid)
    return STATUS_NO_SUCH_DEVICE;
  cmd(h, h->AfgNid, HDA_SET_POWER_STATE, 0);
  stall(10000);
  h->AfgOutAmp = param(h, h->AfgNid, HDA_PAR_OUT_AMP_CAPS);
  h->AfgInAmp = param(h, h->AfgNid, HDA_PAR_IN_AMP_CAPS);
  n = param(h, h->AfgNid, HDA_PAR_NODE_COUNT);
  h->NodeStart = (UCHAR)(n >> 16);
  h->NodeCount = (UCHAR)((n & 0xFF) > HDA_MAX_NODES ? HDA_MAX_NODES : n);
  for (ULONG i = 0; i < h->NodeCount; i++) {
    HDA_WIDGET *w = &h->Nodes[i];
    UCHAR nid = (UCHAR)(h->NodeStart + i);
    RtlZeroMemory(w, sizeof *w);
    w->Caps = param(h, nid, HDA_PAR_WIDGET_CAPS);
    if (w->Caps == 0xFFFFFFFF) {
      w->Caps = 0x00F00000; /* vendor-defined: ignored */
      continue;
    }
    if (HDA_WCAP_TYPE(w->Caps) == HDA_WIDGET_PIN) {
      w->PinCaps = param(h, nid, HDA_PAR_PIN_CAPS);
      w->Config = cmd(h, nid, HDA_GET_CONFIG_DEFAULT, 0);
    }
    if (w->Caps & HDA_WCAP_OUT_AMP)
      w->OutAmp = w->Caps & HDA_WCAP_AMP_OVRD
                      ? param(h, nid, HDA_PAR_OUT_AMP_CAPS)
                      : h->AfgOutAmp;
    if (w->Caps & HDA_WCAP_IN_AMP)
      w->InAmp = w->Caps & HDA_WCAP_AMP_OVRD
                     ? param(h, nid, HDA_PAR_IN_AMP_CAPS)
                     : h->AfgInAmp;
    if (w->Caps & HDA_WCAP_CONN_LIST)
      read_connections(h, nid, w);
  }
  if (!choose_path(h))
    return STATUS_NO_SUCH_DEVICE;
  choose_amps(h);
  h->DacPcm = param(h, h->Dac, HDA_PAR_PCM);
  if (!h->DacPcm || h->DacPcm == 0xFFFFFFFF)
    h->DacPcm = param(h, h->AfgNid, HDA_PAR_PCM);
  return STATUS_SUCCESS;
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

/* Reset the controller, start its rings and find a codec with an output
   path. h->Regs, h->Rings and h->RingsPhys are set. */
NTSTATUS hda_init(HDA *h) {
  KeInitializeSpinLock(&h->Lock);
  USHORT gcap = hda_rd16(h, HDA_GCAP);
  if (gcap == 0xFFFF)
    return STATUS_DEVICE_NOT_CONNECTED;
  h->InputStreams = (gcap >> 8) & 0xF;
  h->OutputStreams = (gcap >> 12) & 0xF;
  if (!h->OutputStreams)
    return STATUS_DEVICE_CONFIGURATION_ERROR;
  h->StreamIndex = h->InputStreams; /* the first output stream */
  h->StreamTag = 1;

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
  USHORT codecs = hda_rd16(h, HDA_STATESTS);
  hda_wr16(h, HDA_STATESTS, codecs);

  /* The CORB at the start of the page, the RIRB 1 KB in and the BDL
     3 KB in (section 4.4). */
  h->Corb = (volatile ULONG *)h->Rings;
  h->Rirb = (volatile ULONG *)((UCHAR *)h->Rings + 1024);
  h->Bdl = (HDA_BDL_ENTRY *)((UCHAR *)h->Rings + 3072);
  h->BdlPhys = h->RingsPhys + 3072;
  RtlZeroMemory(h->Rings, 4096);

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

  for (ULONG cad = 0; cad < 15; cad++)
    if ((codecs & (1u << cad)) && NT_SUCCESS(probe_codec(h, cad)))
      return STATUS_SUCCESS;
  return STATUS_NO_SUCH_DEVICE;
}

void hda_shutdown(HDA *h) {
  if (!h->Regs)
    return;
  hda_stream_reset(h);
  hda_wr32(h, HDA_INTCTL, 0);
  hda_wr8(h, HDA_CORBCTL, 0);
  hda_wr8(h, HDA_RIRBCTL, 0);
  wait8(h, HDA_CORBCTL, HDA_DMA_RUN, 0, 1000);
  wait8(h, HDA_RIRBCTL, HDA_DMA_RUN, 0, 1000);
  hda_wr32(h, HDA_GCTL, hda_rd32(h, HDA_GCTL) & ~HDA_GCTL_CRST);
}

void hda_stream_run(HDA *h, BOOLEAN run) {
  ULONG sd = HDA_SD(h->StreamIndex);
  ULONG ctl = hda_rd32(h, sd + HDA_SD_CTL) & 0x00FFFFFF;
  if (run) {
    ctl |= HDA_SD_RUN | HDA_SD_IOCE;
    h->StreamFresh = FALSE;
  }
  else
    ctl &= ~(ULONG)HDA_SD_RUN;
  hda_wr32(h, sd + HDA_SD_CTL, ctl);
  if (!run)
    wait32(h, sd + HDA_SD_CTL, HDA_SD_RUN, 0, 1000);
}

void hda_stream_reset(HDA *h) {
  ULONG sd = HDA_SD(h->StreamIndex);
  hda_stream_run(h, FALSE);
  hda_wr32(h, sd + HDA_SD_CTL, HDA_SD_SRST);
  wait32(h, sd + HDA_SD_CTL, HDA_SD_SRST, HDA_SD_SRST, 1000);
  hda_wr32(h, sd + HDA_SD_CTL, 0);
  wait32(h, sd + HDA_SD_CTL, HDA_SD_SRST, 0, 1000);
  hda_wr8(h, sd + HDA_SD_STS, HDA_SD_STS_ALL);
}

/* Set up the output stream for a cyclic buffer at bufphys, bufsize bytes,
   with an interrupt every fragsize bytes, in format fmt. */
NTSTATUS hda_stream_start(HDA *h, ULONG bufphys, ULONG bufsize,
                          ULONG fragsize, USHORT fmt) {
  ULONG sd = HDA_SD(h->StreamIndex), n = 0, off;
  if (!fragsize || fragsize >= bufsize)
    fragsize = bufsize / 2;
  fragsize &= ~127u;
  if (!fragsize)
    return STATUS_INVALID_PARAMETER;
  hda_stream_reset(h);
  for (off = 0; off < bufsize && n < HDA_MAX_BDL; off += fragsize, n++) {
    ULONG len = bufsize - off < fragsize ? bufsize - off : fragsize;
    h->Bdl[n].AddressLow = bufphys + off;
    h->Bdl[n].AddressHigh = 0;
    h->Bdl[n].Length = len;
    h->Bdl[n].Flags = 1; /* IOC */
  }
  if (off < bufsize) /* more fragments than entries: the last one grows */
    h->Bdl[n - 1].Length += bufsize - off;
  hda_wr32(h, sd + HDA_SD_BDPL, h->BdlPhys);
  hda_wr32(h, sd + HDA_SD_BDPU, 0);
  hda_wr32(h, sd + HDA_SD_CBL, bufsize);
  hda_wr16(h, sd + HDA_SD_LVI, (USHORT)(n - 1));
  hda_wr16(h, sd + HDA_SD_FMT, fmt);
  hda_wr32(h, sd + HDA_SD_CTL, h->StreamTag << 20);
  hda_wr8(h, sd + HDA_SD_STS, HDA_SD_STS_ALL);
  h->StreamSize = bufsize;
  h->StreamFresh = TRUE;
  hda_verb(h, HDA_VERB4(h->Cad, h->Dac, HDA_SET_FORMAT, fmt));
  cmd(h, h->Dac, HDA_SET_STREAM_ID, h->StreamTag << 4);
  hda_wr32(h, HDA_INTCTL, hda_rd32(h, HDA_INTCTL) | HDA_INTCTL_GIE |
                              (1u << h->StreamIndex));
  return STATUS_SUCCESS;
}

/* The position in the buffer, in bytes from its start. */
ULONG hda_stream_position(HDA *h) {
  ULONG lpib = hda_rd32(h, HDA_SD(h->StreamIndex) + HDA_SD_LPIB);
  if (!h->StreamSize || h->StreamFresh)
    return 0;
  return lpib % h->StreamSize;
}

/* The interrupt: TRUE when a buffer of the output stream completed. The
   status bits are cleared. */
BOOLEAN hda_interrupt(HDA *h) {
  ULONG sts = hda_rd32(h, HDA_INTSTS);
  if (sts == 0xFFFFFFFF || !(sts & (1u << h->StreamIndex)))
    return FALSE;
  ULONG sd = HDA_SD(h->StreamIndex);
  UCHAR s = hda_rd8(h, sd + HDA_SD_STS);
  hda_wr8(h, sd + HDA_SD_STS, s & HDA_SD_STS_ALL);
  return TRUE;
}

/* Gains are the amplifier's steps, 0 to HDA_AMPCAP_STEPS(VolCaps). */
void hda_set_volume(HDA *h, ULONG left, ULONG right, BOOLEAN mute) {
  BOOLEAN same = h->MuteNid && h->MuteNid == h->VolNid &&
                 h->MuteIndex == h->VolIndex &&
                 h->MuteIsOutput == h->VolIsOutput;
  if (h->VolNid)
    set_amp(h, h->VolNid, h->VolIsOutput, h->VolIndex, left, right,
            same && mute);
  if (h->MuteNid && !same) {
    HDA_WIDGET *w = node(h, h->MuteNid);
    ULONG g = zero_db(h->MuteIsOutput ? w->OutAmp : w->InAmp);
    set_amp(h, h->MuteNid, h->MuteIsOutput, h->MuteIndex, g, g, mute);
  }
}
