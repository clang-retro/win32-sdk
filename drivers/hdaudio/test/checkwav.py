"""Check the WAV file hdatest's run wrote: a 1 kHz tone for the first
second, silence (muted) from 1.1 to 1.4 s, the tone again but quieter from
1.6 s. checkwav.py FILE [--no-volume] (when the codec has no amplifier the
last two parts are the tone at full level)."""
import math
import struct
import sys
import wave

w = wave.open(sys.argv[1])
rate, n = w.getframerate(), w.getnframes()
data = w.readframes(n)
left = struct.unpack("<%dh" % (len(data) // 2), data)[0::w.getnchannels()]
novol = "--no-volume" in sys.argv


def part(t0, t1):
    seg = left[int(t0 * rate):int(t1 * rate)]
    rms = math.sqrt(sum(x * x for x in seg) / max(len(seg), 1))
    up = sum(1 for a, b in zip(seg, seg[1:]) if a < 0 <= b)
    return rms, up / (t1 - t0)


fails = 0
for name, t0, t1, lo, hi in [
        ("tone", 0.1, 0.9, 10000, 12500),
        ("muted", 1.1, 1.4, -1, 10) if not novol else
        ("tone", 1.1, 1.4, 10000, 12500),
        ("quieter", 1.6, 1.9, 100, 9000) if not novol else
        ("tone", 1.6, 1.9, 10000, 12500)]:
    rms, freq = part(t0, t1)
    ok = lo < rms < hi and (name == "muted" or 980 <= freq <= 1020)
    fails += not ok
    print("%s %s %.1f-%.1f s: rms %.0f, %.0f Hz" %
          ("ok  " if ok else "FAIL", name, t0, t1, rms, freq))
sys.exit(1 if fails else 0)
