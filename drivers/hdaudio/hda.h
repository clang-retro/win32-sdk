/* The Intel High Definition Audio controller and its codecs, for
   hdaudio.sys. The registers, verbs and parameters are those of Intel's
   "High Definition Audio Specification", revision 1.0a (2010). */
#ifndef HDA_H
#define HDA_H

#ifdef HDA_BARE_METAL
#include "test/shim.h" /* test/hdatest.c: without Windows */
#else
#include <ntddk.h>
#endif

/* Controller registers (section 3.3), offsets from BAR 0. */
#define HDA_GCAP      0x00 /* 16: OSS 15:12, ISS 11:8, BSS 7:3, 64OK 0 */
#define HDA_VMIN      0x02
#define HDA_VMAJ      0x03
#define HDA_GCTL      0x08 /* 32: CRST 0 */
#define HDA_STATESTS  0x0E /* 16: codecs present, one bit each */
#define HDA_INTCTL    0x20 /* 32: GIE 31, CIE 30, SIE 29:0 */
#define HDA_INTSTS    0x24 /* 32: GIS 31, CIS 30, SIS 29:0 */
#define HDA_CORBLBASE 0x40
#define HDA_CORBUBASE 0x44
#define HDA_CORBWP    0x48 /* 16 */
#define HDA_CORBRP    0x4A /* 16: CORBRPRST 15 */
#define HDA_CORBCTL   0x4C /* 8: CORBRUN 1 */
#define HDA_CORBSIZE  0x4E /* 8: CORBSZCAP 7:4, CORBSIZE 1:0 */
#define HDA_RIRBLBASE 0x50
#define HDA_RIRBUBASE 0x54
#define HDA_RIRBWP    0x58 /* 16: RIRBWPRST 15 */
#define HDA_RINTCNT   0x5A /* 16 */
#define HDA_RIRBCTL   0x5C /* 8: RIRBDMAEN 1 */
#define HDA_RIRBSTS   0x5D /* 8: RINTFL 0, RIRBOIS 2 */
#define HDA_RIRBSIZE  0x5E /* 8 */

#define HDA_GCTL_CRST     0x00000001
#define HDA_INTCTL_GIE    0x80000000
#define HDA_CORBRP_RST    0x8000
#define HDA_RIRBWP_RST    0x8000
#define HDA_DMA_RUN       0x02 /* CORBCTL and RIRBCTL */

/* Stream descriptors (section 3.3.35): the input streams, then the output
   ones, then the bidirectional ones, 0x20 bytes each. */
#define HDA_SD(n)     (0x80 + (n) * 0x20)
#define HDA_SD_CTL    0x00 /* 24: SRST 0, RUN 1, IOCE 2, STRM 23:20 */
#define HDA_SD_STS    0x03 /* 8: BCIS 2, FIFOE 3, DESE 4 */
#define HDA_SD_LPIB   0x04
#define HDA_SD_CBL    0x08
#define HDA_SD_LVI    0x0C /* 16 */
#define HDA_SD_FMT    0x12 /* 16 */
#define HDA_SD_BDPL   0x18
#define HDA_SD_BDPU   0x1C

#define HDA_SD_SRST   0x01
#define HDA_SD_RUN    0x02
#define HDA_SD_IOCE   0x04
#define HDA_SD_STS_ALL 0x1C

/* A buffer descriptor list entry (section 3.6.3). */
typedef struct {
  ULONG AddressLow, AddressHigh, Length, Flags; /* Flags: IOC 0 */
} HDA_BDL_ENTRY;

/* Stream formats (section 3.7.1): 16-bit stereo PCM. */
#define HDA_FMT_44K1  0x4000
#define HDA_FMT_48K   0x0000
#define HDA_FMT_16BIT 0x0010
#define HDA_FMT_STEREO 0x0001

/* Codec verbs (section 7.3.3), with a 12-bit verb and 8-bit payload or a
   4-bit verb and 16-bit payload. */
#define HDA_VERB(cad, nid, verb, payload) \
  (((ULONG)(cad) << 28) | ((ULONG)(nid) << 20) | ((ULONG)(verb) << 8) | \
   (ULONG)(payload))
#define HDA_VERB4(cad, nid, verb, payload) \
  (((ULONG)(cad) << 28) | ((ULONG)(nid) << 20) | ((ULONG)(verb) << 16) | \
   (ULONG)(payload))

#define HDA_GET_PARAMETER     0xF00
#define HDA_GET_CONN_SELECT   0xF01
#define HDA_SET_CONN_SELECT   0x701
#define HDA_GET_CONN_LIST     0xF02
#define HDA_SET_POWER_STATE   0x705
#define HDA_SET_STREAM_ID     0x706
#define HDA_SET_PIN_CONTROL   0x707
#define HDA_SET_EAPD_BTL      0x70C
#define HDA_GET_CONFIG_DEFAULT 0xF1C
#define HDA_FUNCTION_RESET    0x7FF
#define HDA_SET_FORMAT        0x2 /* 4-bit verbs */
#define HDA_SET_AMP           0x3

/* Set Amplifier Gain/Mute's payload (section 7.3.3.7). */
#define HDA_AMP_OUT   0x8000
#define HDA_AMP_IN    0x4000
#define HDA_AMP_LEFT  0x2000
#define HDA_AMP_RIGHT 0x1000
#define HDA_AMP_MUTE  0x0080

/* Parameters (section 7.3.4). */
#define HDA_PAR_VENDOR_ID     0x00
#define HDA_PAR_NODE_COUNT    0x04
#define HDA_PAR_FG_TYPE       0x05
#define HDA_PAR_WIDGET_CAPS   0x09
#define HDA_PAR_PCM           0x0A
#define HDA_PAR_PIN_CAPS      0x0C
#define HDA_PAR_IN_AMP_CAPS   0x0D
#define HDA_PAR_CONN_LIST_LEN 0x0E
#define HDA_PAR_OUT_AMP_CAPS  0x12

#define HDA_FG_AUDIO 0x01

/* Audio Widget Capabilities. */
#define HDA_WCAP_TYPE(c)   (((c) >> 20) & 0xF)
#define HDA_WCAP_STEREO    0x001
#define HDA_WCAP_IN_AMP    0x002
#define HDA_WCAP_OUT_AMP   0x004
#define HDA_WCAP_AMP_OVRD  0x008
#define HDA_WCAP_CONN_LIST 0x100
#define HDA_WCAP_DIGITAL   0x200
#define HDA_WCAP_POWER     0x400

#define HDA_WIDGET_OUTPUT   0x0
#define HDA_WIDGET_INPUT    0x1
#define HDA_WIDGET_MIXER    0x2
#define HDA_WIDGET_SELECTOR 0x3
#define HDA_WIDGET_PIN      0x4

/* Pin Capabilities and Pin Widget Control. */
#define HDA_PINCAP_HP      0x00000008
#define HDA_PINCAP_OUT     0x00000010
#define HDA_PINCAP_EAPD    0x00010000
#define HDA_PINCTL_OUT     0x40
#define HDA_PINCTL_HP      0x80

/* Configuration Default (section 7.3.3.31). */
#define HDA_CFG_PORT(c)    (((c) >> 30) & 3) /* 1: no connection */
#define HDA_CFG_DEVICE(c)  (((c) >> 20) & 0xF)
#define HDA_DEV_LINE_OUT   0x0
#define HDA_DEV_SPEAKER    0x1
#define HDA_DEV_HP_OUT     0x2

/* Amplifier Capabilities. */
#define HDA_AMPCAP_OFFSET(c) ((c) & 0x7F)
#define HDA_AMPCAP_STEPS(c)  (((c) >> 8) & 0x7F)
#define HDA_AMPCAP_SIZE(c)   (((c) >> 16) & 0x7F) /* 0.25 dB units, - 1 */
#define HDA_AMPCAP_MUTE      0x80000000

#define HDA_MAX_NODES 128
#define HDA_MAX_CONNS 16
#define HDA_MAX_PATH  8
#define HDA_MAX_PINS  4
#define HDA_MAX_BDL   32

typedef struct {
  UCHAR Nid, Index; /* the widget, and the connection it takes input from */
} HDA_PATH_NODE;

typedef struct {
  ULONG Caps, PinCaps, Config, OutAmp, InAmp;
  UCHAR ConnCount, Conns[HDA_MAX_CONNS];
} HDA_WIDGET;

/* One controller, one codec on it and the output path in that codec:
   a DAC and the pins it drives. */
typedef struct {
  volatile UCHAR *Regs;
  ULONG RegsLength;
  KSPIN_LOCK Lock;
  /* The rings, in one page from Rings: CORB, RIRB, BDL. */
  PVOID Rings;
  ULONG RingsPhys;
  volatile ULONG *Corb;
  volatile ULONG *Rirb;
  HDA_BDL_ENTRY *Bdl;
  ULONG BdlPhys;
  ULONG CorbEntries, RirbEntries, RirbRead, Timeouts;
  ULONG InputStreams, OutputStreams;
  /* The codec. */
  ULONG Cad, Vendor;
  UCHAR AfgNid, NodeStart, NodeCount;
  ULONG AfgOutAmp, AfgInAmp;
  HDA_WIDGET Nodes[HDA_MAX_NODES];
  /* The output path from the first pin: Path[0] is the pin, the last
     node the DAC. */
  HDA_PATH_NODE Path[HDA_MAX_PATH];
  ULONG PathLength;
  UCHAR Dac;
  ULONG DacPcm;
  /* The other pins that reach the DAC (speaker and headphones, say). */
  UCHAR Pins[HDA_MAX_PINS];
  ULONG PinCount;
  /* The amplifier for the volume control, if the path has one. */
  UCHAR VolNid, VolIndex, VolIsOutput;
  ULONG VolCaps;
  UCHAR MuteNid, MuteIndex, MuteIsOutput;
  /* Playback: the output stream descriptor and its stream tag. */
  ULONG StreamIndex, StreamTag;
  /* The buffer's size, and whether the stream has not run since it was
     set up: QEMU's controller keeps LPIB over a stream reset, which the
     specification says clears it, until the stream runs again. */
  ULONG StreamSize;
  BOOLEAN StreamFresh;
} HDA;

static inline ULONG hda_rd32(HDA *h, ULONG r) {
  return *(volatile ULONG *)(h->Regs + r);
}
static inline USHORT hda_rd16(HDA *h, ULONG r) {
  return *(volatile USHORT *)(h->Regs + r);
}
static inline UCHAR hda_rd8(HDA *h, ULONG r) {
  return *(volatile UCHAR *)(h->Regs + r);
}
static inline void hda_wr32(HDA *h, ULONG r, ULONG v) {
  *(volatile ULONG *)(h->Regs + r) = v;
}
static inline void hda_wr16(HDA *h, ULONG r, USHORT v) {
  *(volatile USHORT *)(h->Regs + r) = v;
}
static inline void hda_wr8(HDA *h, ULONG r, UCHAR v) {
  *(volatile UCHAR *)(h->Regs + r) = v;
}

/* hda.c */
NTSTATUS hda_init(HDA *h);
void hda_shutdown(HDA *h);
ULONG hda_verb(HDA *h, ULONG verb);
NTSTATUS hda_stream_start(HDA *h, ULONG bufphys, ULONG bufsize,
                          ULONG fragsize, USHORT fmt);
void hda_stream_run(HDA *h, BOOLEAN run);
void hda_stream_reset(HDA *h);
ULONG hda_stream_position(HDA *h);
BOOLEAN hda_interrupt(HDA *h);
void hda_set_volume(HDA *h, ULONG left, ULONG right, BOOLEAN mute);

#endif
