/* hdatest: runs hda.c without Windows, on a PC that QEMU emulates with an
   intel-hda controller (run-tests.sh). It finds the controller on PCI,
   starts it and its codec, prints the output path it chose, and plays a
   1 kHz tone at 48 kHz for two seconds: the first full, the next half
   second muted and the last at a lower volume. It polls for the
   controller's buffer interrupts, prints how many came and how far the
   position went, and exits QEMU. run-tests.sh checks the output and the
   WAV file QEMU wrote. */
#include "../hda.h"

static inline void outb(USHORT p, UCHAR v) {
  __asm__ volatile("outb %0, %1" ::"a"(v), "Nd"(p));
}
static inline UCHAR inb(USHORT p) {
  UCHAR v;
  __asm__ volatile("inb %1, %0" : "=a"(v) : "Nd"(p));
  return v;
}
static inline void outl(USHORT p, ULONG v) {
  __asm__ volatile("outl %0, %1" ::"a"(v), "Nd"(p));
}
static inline ULONG inl(USHORT p) {
  ULONG v;
  __asm__ volatile("inl %1, %0" : "=a"(v) : "Nd"(p));
  return v;
}
static inline unsigned long long rdtsc(void) {
  unsigned long long v;
  __asm__ volatile("rdtsc" : "=A"(v));
  return v;
}

void *memset(void *d, int c, unsigned long n) {
  UCHAR *p = d;
  while (n--)
    *p++ = (UCHAR)c;
  return d;
}

/* 64-bit division for the TSC arithmetic, without libgcc. */
unsigned long long __udivdi3(unsigned long long n, unsigned long long d) {
  unsigned long long q = 0, bit = 1;
  if (!d)
    return 0;
  while (!(d >> 63) && d < n) {
    d <<= 1;
    bit <<= 1;
  }
  while (bit) {
    if (n >= d) {
      n -= d;
      q |= bit;
    }
    d >>= 1;
    bit >>= 1;
  }
  return q;
}

static void putc_(char c) {
  while (!(inb(0x3FD) & 0x20))
    ;
  outb(0x3F8, (UCHAR)c);
}
static void puts_(const char *s) {
  while (*s)
    putc_(*s++);
}
static void hex(ULONG v, int digits) {
  while (digits--)
    putc_("0123456789abcdef"[(v >> (digits * 4)) & 0xF]);
}
static void dec(ULONG v) {
  char b[12];
  int n = 0;
  do
    b[n++] = (char)('0' + v % 10);
  while (v /= 10);
  while (n)
    putc_(b[--n]);
}

static unsigned long long tsc_per_us;

void KeStallExecutionProcessor(ULONG us) {
  unsigned long long end = rdtsc() + us * tsc_per_us;
  while (rdtsc() < end)
    ;
}

/* TSC ticks in 10 ms of PIT channel 2. */
static void calibrate(void) {
  outb(0x61, (inb(0x61) & ~0x02) | 0x01);
  outb(0x43, 0xB0); /* channel 2, mode 0 */
  outb(0x42, 11932 & 0xFF);
  outb(0x42, 11932 >> 8);
  unsigned long long t0 = rdtsc();
  while (!(inb(0x61) & 0x20))
    ;
  tsc_per_us = (rdtsc() - t0) / 10000;
  if (!tsc_per_us)
    tsc_per_us = 1;
}

static ULONG pci_read(ULONG bus, ULONG dev, ULONG fn, ULONG reg) {
  outl(0xCF8, 0x80000000 | bus << 16 | dev << 11 | fn << 8 | reg);
  return inl(0xCFC);
}
static void pci_write(ULONG bus, ULONG dev, ULONG fn, ULONG reg, ULONG v) {
  outl(0xCF8, 0x80000000 | bus << 16 | dev << 11 | fn << 8 | reg);
  outl(0xCFC, v);
}

static HDA hda;
static HDA_CODEC codec;
static HDA_FUNCTION fns[8];
static UCHAR rings[HDA_RINGS_SIZE] __attribute__((aligned(4096)));
#define BUFSIZE (8 * 1920) /* 8 fragments of 10 ms */
static short buffer[BUFSIZE / 2] __attribute__((aligned(128)));
static const short sine[48] = {
    0,      2088,   4141,   6123,   8000,   9740,   11314,  12694,
    13856,  14782,  15455,  15863,  16000,  15863,  15455,  14782,
    13856,  12694,  11314,  9740,   8000,   6123,   4141,   2088,
    0,      -2088,  -4141,  -6123,  -8000,  -9740,  -11314, -12694,
    -13856, -14782, -15455, -15863, -16000, -15863, -15455, -14782,
    -13856, -12694, -11314, -9740,  -8000,  -6123,  -4141,  -2088};

static void quit(int code) {
  outb(0xF4, (UCHAR)code); /* isa-debug-exit: QEMU exits with code*2+1 */
}

static ULONG verb(PVOID context, ULONG v) { return hda_verb(context, v); }

static void fail(const char *what) {
  puts_(what);
  putc_('\n');
  quit(1);
}

int main(void) {
  ULONG bar = 0, irqs = 0, laps = 0, last = 0, nfns = 0;
  calibrate();
  for (ULONG dev = 0; dev < 32 && !bar; dev++)
    for (ULONG fn = 0; fn < 8 && !bar; fn++) {
      ULONG id = pci_read(0, dev, fn, 0);
      if (id == 0xFFFFFFFF)
        continue;
      if ((pci_read(0, dev, fn, 8) >> 16) == 0x0403) {
        bar = pci_read(0, dev, fn, 0x10) & ~0xFu;
        pci_write(0, dev, fn, 4, pci_read(0, dev, fn, 4) | 0x6);
        puts_("controller ");
        hex(id & 0xFFFF, 4), putc_(':'), hex(id >> 16, 4);
        puts_(" at ");
        hex(bar, 8);
        putc_('\n');
      }
    }
  if (!bar)
    return fail("no HDA controller"), 1;
  hda.Regs = (volatile UCHAR *)bar;
  hda.Rings = rings;
  hda.RingsPhys = (ULONG)rings;
  NTSTATUS st = hda_init(&hda);
  puts_("init ");
  hex((ULONG)st, 8);
  puts_(" version ");
  hex(hda.Version, 4);
  puts_(" streams in ");
  dec(hda.InputStreams);
  puts_(" out ");
  dec(hda.OutputStreams);
  puts_(" codecs ");
  hex(hda.Codecs, 4);
  putc_('\n');
  if (!NT_SUCCESS(st))
    return fail("init failed"), 1;

  /* The function groups, as the bus driver makes devices of them. */
  for (ULONG cad = 0; cad < 15; cad++)
    if (hda.Codecs & (1u << cad))
      nfns += codec_functions(verb, &hda, cad, fns + nfns, 8 - nfns);
  codec.Verb = verb;
  codec.VerbContext = &hda;
  for (ULONG i = 0; i < nfns; i++) {
    puts_("function codec ");
    dec(fns[i].Cad);
    puts_(" node ");
    dec(fns[i].Nid);
    puts_(" type ");
    hex(fns[i].Type, 2);
    puts_(" HDAUDIO\\FUNC_");
    hex(fns[i].Type, 2);
    puts_("&VEN_");
    hex(fns[i].Vendor >> 16, 4);
    puts_("&DEV_");
    hex(fns[i].Vendor & 0xFFFF, 4);
    puts_("&SUBSYS_");
    hex(fns[i].Subsystem, 8);
    puts_("&REV_");
    hex(fns[i].Revision >> 16, 4);
    putc_('\n');
    if ((fns[i].Type & 0xFF) == HDA_FG_AUDIO && !codec.AfgNid) {
      codec.Cad = fns[i].Cad;
      codec.AfgNid = fns[i].Nid;
    }
  }
  if (!codec.AfgNid)
    return fail("no audio function group"), 1;
  st = codec_probe(&codec);
  puts_("path");
  for (ULONG i = 0; i < codec.PathLength; i++)
    putc_(' '), dec(codec.Path[i].Nid);
  puts_("\ndac ");
  dec(codec.Dac);
  puts_(" pcm ");
  hex(codec.DacPcm, 8);
  puts_(" pins");
  for (ULONG i = 0; i < codec.PinCount; i++)
    putc_(' '), dec(codec.Pins[i]);
  puts_("\nvolume ");
  dec(codec.VolNid);
  puts_(codec.VolIsOutput ? " out " : " in ");
  hex(codec.VolCaps, 8);
  puts_(" mute ");
  dec(codec.MuteNid);
  putc_('\n');
  if (!NT_SUCCESS(st))
    return fail("no output path"), 1;

  for (ULONG i = 0; i < BUFSIZE / 4; i++)
    buffer[2 * i] = buffer[2 * i + 1] = sine[i % 48];
  ULONG steps = HDA_AMPCAP_STEPS(codec.VolCaps);
  ULONG zero = HDA_AMPCAP_OFFSET(codec.VolCaps);
  if (zero > steps)
    zero = steps;
  codec_set_volume(&codec, zero, zero, FALSE);
  USHORT fmt = hda_format(48000, 16, 2);
  HDA_ENGINE *e = hda_engine_alloc(&hda, TRUE);
  HDA_ENGINE *e2 = hda_engine_alloc(&hda, TRUE);
  if (!e || !e2 || e->Tag == e2->Tag || fmt != 0x0011)
    return fail("engine or format"), 1;
  hda_engine_free(&hda, e2);
  st = hda_engine_setup(&hda, e, (ULONG)buffer, BUFSIZE, 1920, TRUE, fmt);
  if (!NT_SUCCESS(st))
    return fail("stream setup failed"), 1;
  codec_stream(&codec, e->Tag, fmt);
  hda_engine_run(&hda, e, TRUE);
  unsigned long long t0 = rdtsc(), us = 0;
  int phase = 0;
  while ((us = (rdtsc() - t0) / tsc_per_us) < 2000000) {
    if (hda_interrupt(&hda, NULL) & (1u << e->Index))
      irqs++;
    ULONG pos = hda_engine_position(&hda, e);
    if (pos < last)
      laps++;
    last = pos;
    if (phase == 0 && us >= 1000000) {
      codec_set_volume(&codec, zero, zero, TRUE);
      phase = 1;
    } else if (phase == 1 && us >= 1500000) {
      codec_set_volume(&codec, zero / 2, zero / 2, FALSE);
      phase = 2;
    }
  }
  hda_engine_reset(&hda, e);
  puts_("interrupts ");
  dec(irqs);
  puts_(" laps ");
  dec(laps);
  puts_(" position after reset ");
  dec(hda_engine_position(&hda, e));
  /* Again from the start, as after KSSTATE_STOP: the position must start
     at 0 again and the interrupts come again. */
  hda_engine_setup(&hda, e, (ULONG)buffer, BUFSIZE, 1920, TRUE, fmt);
  puts_("\nrestart position ");
  dec(hda_engine_position(&hda, e));
  hda_engine_run(&hda, e, TRUE);
  KeStallExecutionProcessor(15000);
  puts_(" after 15 ms ");
  dec(hda_engine_position(&hda, e));
  irqs = 0;
  t0 = rdtsc();
  while ((rdtsc() - t0) / tsc_per_us < 200000)
    if (hda_interrupt(&hda, NULL) & (1u << e->Index))
      irqs++;
  hda_engine_free(&hda, e);
  codec_stream(&codec, 0, 0);
  puts_(" interrupts ");
  dec(irqs);
  puts_("\ndone\n");
  hda_shutdown(&hda);
  quit(0);
  return 0;
}
