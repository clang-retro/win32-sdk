# hdaudio: HD Audio for Windows 98, 98 SE and ME

Drivers for Intel High Definition Audio controllers, the ones in PCs from
2004 on and the ones QEMU emulates (`intel-hda` and `ich9-intel-hda`), for
Windows 98, 98 SE and ME, which have none. They are written in C from
Intel's public *High Definition Audio Specification* (revision 1.0a), with
no Microsoft code, and split the way XP's are:

* `hdaudbus.sys`, the bus driver, owns the controller. It finds the
  function groups on each codec and makes a device of each, named as XP
  names them (`HDAUDIO\FUNC_01&VEN_xxxx&DEV_xxxx&SUBSYS_xxxxxxxx&REV_xxxx`,
  `FUNC_01` for audio and `FUNC_02` for a modem). Each one's driver gets
  XP's HD Audio bus interface (`HDAUDIO_BUS_INTERFACE` and
  `HDAUDIO_BUS_INTERFACE_BDL` in `<hdaudio.h>`) by IRP_MN_QUERY_INTERFACE:
  codec verbs, DMA engines and their buffers, the link position and wall
  clock, and the codec's unsolicited responses.
* `hdaudio.sys`, the audio function driver (port class), takes the
  `FUNC_01` device and reaches the hardware only through that interface.
* a soft modem's driver takes the `FUNC_02` device the same way, so the
  modem half of a codec goes to its own driver, as on XP.
  `modem/hdamodem.sys` is a sample that shows the handoff.

What the audio driver does:

* plays 16-bit stereo PCM at 44.1 or 48 kHz (KMixer converts anything else)
* through the first analog output path of its function group: a line out
  pin first, then a speaker, then headphones, and every other connected
  output pin that reaches the same DAC (a laptop's speaker and headphone
  jack together, say)
* has a master volume and mute in the Volume Control, on the first
  amplifier on that path that has steps (usually the DAC's)

What it does not do yet: recording, S/PDIF, more than two channels, jack
sensing, and power management. As it uses only the bus interface, it
should also run on XP's `HDAUDBUS.SYS`; that was not tried.

## Files

```
hda.h, hda.c    the controller: reset, the CORB and RIRB, unsolicited
                responses, stream descriptors and their buffer
                descriptor lists
codec.c         the codec: its function groups, the widget graph, the
                output path and its amplifiers
hdaudbus.c      the bus driver: the controller's PnP, its children, and
                the bus interface
hdaudbus.inf    installs it on the PCI controller
hdaudio.c       the audio function driver and its two miniports: Wave
                (WaveCyclic) and Topology, as COM objects in C
hdaudio.inf     installs it on HDAUDIO\FUNC_01
modem/          hdamodem.c and .inf: the sample modem function driver
test/           tests without Windows (below)
```

## Building

With the toolchain package's `bin/` as `CFGDIR`:

```sh
cc="$CFGDIR/clang --config=$CFGDIR/nt4-ddk.cfg -march=i486 -O2"
$cc hda.c codec.c hdaudbus.c -o hdaudbus.sys
$cc codec.c hdaudio.c -lportcls -lksguid -o hdaudio.sys
$cc modem/hdamodem.c -o hdamodem.sys
python3 ../../tools/check-imports.py --wdm 98 hdaudbus.sys
```

`-march=i486` because Windows 98 needs a 486 anyway and the reference
counts use its `xadd`; `nt4-ddk.cfg` alone targets the 386.
`tests/ddk/run-tests.sh` builds all three at -O0, -O2 and -Os and checks
them against 98, 98 SE and ME.

## Installing

Copy `hdaudbus.sys`, `hdaudbus.inf`, `hdaudio.sys` and `hdaudio.inf` to a
floppy or a folder. In Device Manager open the PCI audio device (or the
"PCI Multimedia Audio Device" with the yellow mark), choose Update Driver,
and point it at that folder. Windows then finds the audio function group
and asks for its driver: point it at the same folder. It asks for its CD
for the KS and WDM audio files.

To try it in QEMU:

```sh
qemu-system-i386 -m 256 -hda win98se.img -device intel-hda \
  -device hda-duplex,audiodev=snd -audiodev pa,id=snd
```

(`-audiodev wav,id=snd,path=out.wav` records what it plays instead.)

## Writing a modem (or other) function driver

Bind to `HDAUDIO\FUNC_02` (or the full ID with your codec's vendor and
subsystem), and on IRP_MN_START_DEVICE send IRP_MN_QUERY_INTERFACE down
your stack for `GUID_HDAUDIO_BUS_INTERFACE_BDL` (or
`GUID_HDAUDIO_BUS_INTERFACE`), version 0x0100, as on XP.
`GetResourceInformation` gives your codec address and function group
node. Verbs go to your codec whatever address they carry.

One difference from XP: 98's WDM has no `MmGetPhysicalAddress`, so
`AllocateContiguousDmaBuffer` hands the BDL over with its first entry
already filled with the whole buffer's address and length. Read the
address from there before you write your own entries. A driver written
for XP overwrites it and loses nothing.

Unsolicited responses are polled every 20 ms and delivered at
DISPATCH_LEVEL to the routine `RegisterEventCallback` registered for their
tag. A BDL engine's interrupt routine is called from the controller's
interrupt, with the stream's status bits.

## Tests

`test/run-tests.sh` runs, with no Windows:

* `graphtest`, codec.c on the host, against a made-up codec shaped like a
  laptop's: two DACs behind mixers, a speaker and a headphone pin with a
  selector, an unconnected line out, an S/PDIF pin and input pins, with
  connection lists in the long form and with ranges, and a modem function
  group beside the audio one. It checks the function groups, the path,
  the pins, the amplifiers, and the verbs sent.
* `hdatest`, hda.c and codec.c on a bare PC in QEMU (a multiboot kernel),
  with each of QEMU's two controllers and four codec setups: it lists the
  function groups with the IDs the bus driver gives them, then plays a
  1 kHz tone at 48 kHz for two seconds, mutes it, turns it down, then
  stops and starts the stream again. The script checks the buffer
  interrupts (one each 10 ms) and the position, and `checkwav.py` checks
  the WAV file QEMU recorded: the tone's level and frequency, the silence
  and the lower volume.

The bus driver's PnP and the port class drivers need Windows and are not
tested yet beyond building and the import checks.
