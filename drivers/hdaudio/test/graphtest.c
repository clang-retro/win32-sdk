/* graphtest: hda.c's codec parsing and path choice against a made-up
   codec shaped like the ones in PCs, which QEMU's codecs are not: two
   DACs behind mixers, a speaker and a headphone pin that can take either,
   a line out pin the configuration says is not connected, an S/PDIF pin,
   input pins, and connection lists in the long form and with ranges; and
   a modem function group beside the audio one.
   Built and run on the host by run-tests.sh. */
#include "../codec.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void KeStallExecutionProcessor(ULONG us) { (void)us; }

#define OUT_AMP_CAPS (0x80000000u | 0x05 << 16 | 0x3F << 8 | 0x3F)
#define MUTE_ONLY 0x80000000u

typedef struct {
  ULONG caps, pincaps, config, outamp, inamp;
  int nconn, longform;
  ULONG conn[8]; /* raw entries, range bits included */
} FAKE;

static FAKE nodes[0x30];

#define W(type, extra) ((ULONG)(type) << 20 | (extra))
#define PINCFG(port, dev) ((ULONG)(port) << 30 | (ULONG)(dev) << 20)

static void setup(void) {
  /* DACs */
  nodes[0x02] = (FAKE){.caps = W(0, HDA_WCAP_STEREO | HDA_WCAP_OUT_AMP |
                                        HDA_WCAP_AMP_OVRD | HDA_WCAP_POWER),
                       .outamp = OUT_AMP_CAPS};
  nodes[0x03] = nodes[0x02];
  nodes[0x06] = (FAKE){.caps = W(0, HDA_WCAP_DIGITAL)};
  /* mixers: DAC and the analog loopback */
  nodes[0x0c] = (FAKE){.caps = W(2, HDA_WCAP_IN_AMP | HDA_WCAP_AMP_OVRD |
                                        HDA_WCAP_CONN_LIST),
                       .inamp = MUTE_ONLY, .nconn = 2,
                       .conn = {0x02, 0x0b}};
  nodes[0x0d] = nodes[0x0c];
  nodes[0x0d].conn[0] = 0x03;
  /* loopback mixer from the input pins: 0x18, then 0x19 to 0x1a as a
     range, in the long form */
  nodes[0x0b] = (FAKE){.caps = W(2, HDA_WCAP_CONN_LIST), .nconn = 2,
                       .longform = 1, .conn = {0x18, 0x8000 | 0x1a}};
  /* the selector in front of the headphones */
  nodes[0x0f] = (FAKE){.caps = W(3, HDA_WCAP_CONN_LIST), .nconn = 2,
                       .conn = {0x0d, 0x0c}};
  /* pins */
  nodes[0x14] = (FAKE){.caps = W(4, HDA_WCAP_OUT_AMP | HDA_WCAP_AMP_OVRD |
                                        HDA_WCAP_CONN_LIST),
                       .pincaps = HDA_PINCAP_OUT | HDA_PINCAP_EAPD,
                       .config = PINCFG(2, HDA_DEV_SPEAKER),
                       .outamp = MUTE_ONLY, .nconn = 2,
                       .conn = {0x0c, 0x0d}};
  nodes[0x15] = (FAKE){.caps = W(4, HDA_WCAP_CONN_LIST),
                       .pincaps = HDA_PINCAP_OUT | HDA_PINCAP_HP,
                       .config = PINCFG(0, HDA_DEV_HP_OUT), .nconn = 1,
                       .conn = {0x0f}};
  nodes[0x1b] = (FAKE){.caps = W(4, HDA_WCAP_CONN_LIST),
                       .pincaps = HDA_PINCAP_OUT,
                       .config = PINCFG(1, HDA_DEV_LINE_OUT), .nconn = 2,
                       .conn = {0x0c, 0x0d}};
  nodes[0x1e] = (FAKE){.caps = W(4, HDA_WCAP_DIGITAL | HDA_WCAP_CONN_LIST),
                       .pincaps = HDA_PINCAP_OUT,
                       .config = PINCFG(0, HDA_DEV_LINE_OUT), .nconn = 1,
                       .conn = {0x06}};
  nodes[0x18] = (FAKE){.caps = W(4, 0), .pincaps = 0x20,
                       .config = PINCFG(0, 0xA)};
  nodes[0x19] = nodes[0x1a] = nodes[0x18];
}

/* Every verb that set something, for the checks. */
static char log_[8192];

static ULONG fake(ULONG verb) {
  ULONG nid = verb >> 20 & 0x7F, v12 = verb >> 8 & 0xFFF, p8 = verb & 0xFF;
  ULONG v4 = verb >> 16 & 0xF;
  FAKE *f = &nodes[nid < 0x30 ? nid : 0];
  char line[64];
  if (v12 == HDA_GET_PARAMETER) {
    switch (p8) {
    case HDA_PAR_VENDOR_ID:
      return nid == 0 ? 0x10ec0123 : 0;
    case HDA_PAR_NODE_COUNT:
      return nid == 0 ? 0x20 << 16 | 2 : nid == 0x20 ? 0x02 << 16 | 0x1d : 0;
    case HDA_PAR_FG_TYPE:
      return nid == 0x20 ? HDA_FG_AUDIO
             : nid == 0x21 ? 0x100 | HDA_FG_MODEM
                           : 0;
    case HDA_PAR_WIDGET_CAPS:
      return f->caps ? f->caps : W(0xF, 0);
    case HDA_PAR_PCM:
      return nid == 0x20 ? 0x000E0560 : 0;
    case HDA_PAR_PIN_CAPS:
      return f->pincaps;
    case HDA_PAR_OUT_AMP_CAPS:
      return nid == 0x20 ? 0 : f->outamp;
    case HDA_PAR_IN_AMP_CAPS:
      return nid == 0x20 ? 0 : f->inamp;
    case HDA_PAR_CONN_LIST_LEN:
      return (ULONG)f->nconn | (f->longform ? 0x80 : 0);
    }
    return 0;
  }
  if (v12 == HDA_GET_CONFIG_DEFAULT)
    return f->config;
  if (v12 == HDA_GET_SUBSYSTEM_ID)
    return 0x1234abcd;
  if (v12 == HDA_GET_CONN_LIST) {
    ULONG r = 0, per = f->longform ? 2 : 4, bits = f->longform ? 16 : 8;
    for (ULONG i = 0; i < per && p8 + i < (ULONG)f->nconn; i++)
      r |= f->conn[p8 + i] << (i * bits);
    return r;
  }
  if (v4 == HDA_SET_AMP)
    snprintf(line, sizeof line, "amp %02x %04x\n", nid, verb & 0xFFFF);
  else if (v12 == HDA_SET_CONN_SELECT)
    snprintf(line, sizeof line, "select %02x %u\n", nid, p8);
  else if (v12 == HDA_SET_PIN_CONTROL)
    snprintf(line, sizeof line, "pinctl %02x %02x\n", nid, p8);
  else if (v12 == HDA_SET_EAPD_BTL)
    snprintf(line, sizeof line, "eapd %02x %02x\n", nid, p8);
  else if (v12 == HDA_SET_POWER_STATE)
    snprintf(line, sizeof line, "power %02x %u\n", nid, p8);
  else
    snprintf(line, sizeof line, "verb %08x\n", verb);
  strcat(log_, line);
  return 0;
}

static ULONG fake_verb(PVOID context, ULONG verb) {
  (void)context;
  return fake(verb);
}

static int fails;
static void expect(int ok, const char *what) {
  printf("%s %s\n", ok ? "ok  " : "FAIL", what);
  fails += !ok;
}

int main(void) {
  static HDA_CODEC h;
  HDA_FUNCTION fns[4];
  setup();
  ULONG n = codec_functions(fake_verb, NULL, 0, fns, 4);
  expect(n == 2 && fns[0].Nid == 0x20 && fns[0].Type == HDA_FG_AUDIO &&
             fns[1].Nid == 0x21 && fns[1].Type == HDA_FG_MODEM &&
             fns[1].Vendor == 0x10ec0123 && fns[1].Subsystem == 0x1234abcd,
         "function groups: audio at 20, modem at 21");
  h.Verb = fake_verb;
  h.Cad = 0;
  h.AfgNid = 0x20;
  NTSTATUS st = codec_probe(&h);
  expect(st == STATUS_SUCCESS, "codec_probe");
  expect(codec_probe(&(HDA_CODEC){.Verb = fake_verb, .AfgNid = 0x21}) ==
             STATUS_NO_SUCH_DEVICE,
         "codec_probe refuses the modem function group");
  printf("path:");
  for (ULONG i = 0; i < h.PathLength; i++)
    printf(" %02x/%u", h.Path[i].Nid, h.Path[i].Index);
  printf("\npins:");
  for (ULONG i = 0; i < h.PinCount; i++)
    printf(" %02x", h.Pins[i]);
  printf("\nvolume %02x %s, mute %02x %s, pcm %08x\n", h.VolNid,
         h.VolIsOutput ? "out" : "in", h.MuteNid,
         h.MuteIsOutput ? "out" : "in", h.DacPcm);
  printf("%s", log_);
  HDA_WIDGET *lb = node(&h, 0x0b);
  expect(lb->ConnCount == 3 && lb->Conns[0] == 0x18 && lb->Conns[1] == 0x19 &&
             lb->Conns[2] == 0x1a,
         "long-form connection list with a range");
  expect(h.PathLength == 3 && h.Path[0].Nid == 0x14 && h.Path[0].Index == 0 &&
             h.Path[1].Nid == 0x0c && h.Path[1].Index == 0 &&
             h.Path[2].Nid == 0x02,
         "speaker (line out is not connected) -> mixer 0c -> DAC 02");
  expect(h.Dac == 0x02, "DAC 02, not the S/PDIF one");
  expect(h.PinCount == 2 && h.Pins[1] == 0x15,
         "headphones through the selector to the same DAC");
  expect(h.VolNid == 0x02 && h.VolIsOutput && h.MuteNid == 0x02,
         "volume and mute on the DAC's output amplifier");
  expect(h.DacPcm == 0x000E0560, "the PCM formats from the function group");
  expect(strstr(log_, "pinctl 14 40\n") != NULL, "speaker pin enabled");
  expect(strstr(log_, "eapd 14 02\n") != NULL, "speaker amplifier (EAPD) on");
  expect(strstr(log_, "pinctl 15 c0\n") != NULL,
         "headphone pin enabled with its amplifier");
  expect(strstr(log_, "select 0f 1\n") != NULL,
         "selector 0f set to mixer 0c");
  expect(strstr(log_, "select 14 0\n") != NULL, "speaker pin set to mixer 0c");
  expect(strstr(log_, "amp 0c 6000\n") && strstr(log_, "amp 0c 6180\n"),
         "mixer 0c: DAC input unmuted, loopback input muted");
  expect(strstr(log_, "amp 02 a03f\n") && strstr(log_, "amp 02 903f\n"),
         "DAC amplifier at 0 dB, unmuted");
  expect(!strstr(log_, "pinctl 1b") && !strstr(log_, "pinctl 1e"),
         "unconnected line out and S/PDIF pins left alone");
  return fails ? 1 : 0;
}
