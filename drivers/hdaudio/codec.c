/* An audio function group of an HD Audio codec, for hdaudio.sys (through
   the bus driver's TransferCodecVerbs) and the tests: its widget graph,
   an analog output path through it and the amplifiers for a volume
   control; and the function groups of a codec, for hdaudbus.sys. Written
   from Intel's "High Definition Audio Specification", revision 1.0a. */
#include "hda.h"

static void stall_ms(ULONG ms) {
  while (ms--)
    for (int i = 0; i < 20; i++)
      KeStallExecutionProcessor(50);
}

static ULONG verb(HDA_CODEC *h, ULONG v) {
  return h->Verb(h->VerbContext, v);
}

static ULONG param(HDA_CODEC *h, UCHAR nid, ULONG p) {
  return verb(h, HDA_VERB(h->Cad, nid, HDA_GET_PARAMETER, p));
}

static ULONG cmd(HDA_CODEC *h, UCHAR nid, ULONG v, ULONG payload) {
  return verb(h, HDA_VERB(h->Cad, nid, v, payload));
}

static HDA_WIDGET *node(HDA_CODEC *h, ULONG nid) {
  if (nid < h->NodeStart || nid >= (ULONG)h->NodeStart + h->NodeCount)
    return NULL;
  return &h->Nodes[nid - h->NodeStart];
}

/* Set an amplifier: both channels, the gain of each, and the mute. */
static void set_amp(HDA_CODEC *h, UCHAR nid, BOOLEAN out, UCHAR index, ULONG left,
                    ULONG right, BOOLEAN mute) {
  ULONG p = (out ? HDA_AMP_OUT : HDA_AMP_IN) | ((ULONG)index << 8) |
            (mute ? HDA_AMP_MUTE : 0);
  verb(h, HDA_VERB4(h->Cad, nid, HDA_SET_AMP,
                        p | HDA_AMP_LEFT | (left & 0x7F)));
  verb(h, HDA_VERB4(h->Cad, nid, HDA_SET_AMP,
                        p | HDA_AMP_RIGHT | (right & 0x7F)));
}

static ULONG zero_db(ULONG caps) {
  ULONG g = HDA_AMPCAP_OFFSET(caps);
  return g > HDA_AMPCAP_STEPS(caps) ? HDA_AMPCAP_STEPS(caps) : g;
}

static void read_connections(HDA_CODEC *h, UCHAR nid, HDA_WIDGET *w) {
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
static BOOLEAN find_dac(HDA_CODEC *h, UCHAR nid, ULONG depth, UCHAR dac,
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
static void open_path(HDA_CODEC *h, const HDA_PATH_NODE *path, ULONG length) {
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
static BOOLEAN choose_path(HDA_CODEC *h) {
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
static void choose_amps(HDA_CODEC *h) {
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

/* The function groups of the codec at cad: up to max of them into out.
   Their count, 0 when the codec does not answer. */
ULONG codec_functions(HDA_VERB_FN fn, PVOID context, ULONG cad,
                      HDA_FUNCTION *out, ULONG max) {
#define P(nid, p) fn(context, HDA_VERB(cad, nid, HDA_GET_PARAMETER, p))
  ULONG vendor = P(0, HDA_PAR_VENDOR_ID), count = 0;
  if (vendor == 0xFFFFFFFF || vendor == 0)
    return 0;
  ULONG rev = P(0, HDA_PAR_REVISION_ID);
  ULONG n = P(0, HDA_PAR_NODE_COUNT), start = (n >> 16) & 0xFF;
  for (ULONG fg = start; fg < start + (n & 0xFF) && count < max; fg++) {
    HDA_FUNCTION *f = &out[count++];
    f->Cad = (UCHAR)cad;
    f->Nid = (UCHAR)fg;
    f->Type = (UCHAR)P(fg, HDA_PAR_FG_TYPE);
    f->Vendor = vendor;
    f->Revision = rev;
    f->Subsystem = fn(context, HDA_VERB(cad, fg, HDA_GET_SUBSYSTEM_ID, 0));
    if (f->Subsystem == 0xFFFFFFFF)
      f->Subsystem = 0;
  }
  return count;
#undef P
}

/* Read the graph of the audio function group at h->AfgNid of codec
   h->Cad, choose an output path and open it. */
NTSTATUS codec_probe(HDA_CODEC *h) {
  if ((param(h, h->AfgNid, HDA_PAR_FG_TYPE) & 0xFF) != HDA_FG_AUDIO)
    return STATUS_NO_SUCH_DEVICE;
  cmd(h, h->AfgNid, HDA_SET_POWER_STATE, 0);
  stall_ms(10);
  h->AfgOutAmp = param(h, h->AfgNid, HDA_PAR_OUT_AMP_CAPS);
  h->AfgInAmp = param(h, h->AfgNid, HDA_PAR_IN_AMP_CAPS);
  ULONG n = param(h, h->AfgNid, HDA_PAR_NODE_COUNT);
  if (n == 0xFFFFFFFF)
    return STATUS_NO_SUCH_DEVICE;
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

/* Gains are the amplifier's steps, 0 to HDA_AMPCAP_STEPS(VolCaps). */
void codec_set_volume(HDA_CODEC *h, ULONG left, ULONG right, BOOLEAN mute) {
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

/* Point the DAC at stream tag (0: none) in format fmt. */
void codec_stream(HDA_CODEC *h, UCHAR tag, USHORT fmt) {
  if (tag)
    verb(h, HDA_VERB4(h->Cad, h->Dac, HDA_SET_FORMAT, fmt));
  cmd(h, h->Dac, HDA_SET_STREAM_ID, (ULONG)tag << 4);
}
