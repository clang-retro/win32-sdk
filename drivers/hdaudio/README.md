# hdaudio: HD Audio for Windows 98, 98 SE and ME

`hdaudio.sys` is a WDM audio driver (port class) for Intel High Definition
Audio controllers, the ones in PCs from 2004 on and the ones QEMU emulates
(`intel-hda` and `ich9-intel-hda`). Windows 98 and ME have no driver for
them. It is written in C from Intel's public *High Definition Audio
Specification* (revision 1.0a), with no Microsoft code.

What it does:

* plays 16-bit stereo PCM at 44.1 or 48 kHz (KMixer converts anything else)
* through the first analog output path of the first codec it finds: a line
  out pin first, then a speaker, then headphones, and every other
  connected output pin that reaches the same DAC (a laptop's speaker and
  headphone jack together, say)
* has a master volume and mute in the Volume Control, on the first
  amplifier on that path that has steps (usually the DAC's)

What it does not do yet: recording, S/PDIF, more than two channels, jack
sensing, and power management. It drives the controller itself, so on
Windows 2000 and XP, which have Microsoft's HD Audio bus driver, it is not
needed and was not tried.

## Files

```
hda.h, hda.c    the controller and the codec: reset, the CORB and RIRB,
                the widget graph and the output path, the output stream
                and its buffer descriptor list
hdaudio.c       the adapter driver and its two miniports: Wave
                (WaveCyclic) and Topology, as COM objects in C
hdaudio.inf     installs it on 98, 98 SE and ME
test/           tests without Windows (below)
```

## Building

With the toolchain package's `bin/` as `CFGDIR`:

```sh
$CFGDIR/clang --config=$CFGDIR/nt4-ddk.cfg -march=i486 -O2 \
  hda.c hdaudio.c -lportcls -lksguid -o hdaudio.sys
python3 ../../tools/check-imports.py --wdm 98 hdaudio.sys
```

`-march=i486` because Windows 98 needs a 486 anyway and the reference
counts use its `xadd`; `nt4-ddk.cfg` alone targets the 386.
`tests/ddk/run-tests.sh` builds it at -O0, -O2 and -Os and checks it
against 98, 98 SE and ME.

## Installing

Copy `hdaudio.sys` and `hdaudio.inf` to a floppy or a folder, then in
Device Manager open the PCI audio device (or the "PCI Multimedia Audio
Device" with the yellow mark), choose Update Driver, and point it at that
folder. Windows asks for its CD for the KS and WDM audio files it adds.

To try it in QEMU:

```sh
qemu-system-i386 -m 256 -hda win98se.img -device intel-hda \
  -device hda-duplex,audiodev=snd -audiodev pa,id=snd
```

(`-audiodev wav,id=snd,path=out.wav` records what it plays instead.)

## Tests

`test/run-tests.sh` runs, with no Windows:

* `graphtest`, hda.c's codec parsing and path choice on the host, against
  a made-up codec shaped like a laptop's: two DACs behind mixers, a
  speaker and a headphone pin with a selector, an unconnected line out, an
  S/PDIF pin and input pins, with connection lists in the long form and
  with ranges. It checks the path, the pins, the amplifiers, and the verbs
  sent.
* `hdatest`, hda.c on a bare PC in QEMU (a multiboot kernel), with each of
  QEMU's two controllers and four codec setups: it plays a 1 kHz tone at
  48 kHz for two seconds, mutes it, turns it down, then stops and starts
  the stream again. The script checks the buffer interrupts (one each
  10 ms) and the position, and `checkwav.py` checks the WAV file QEMU
  recorded: the tone's level and frequency, the silence and the lower
  volume.
