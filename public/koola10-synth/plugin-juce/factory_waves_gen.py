"""
Koola10 Synth — factory wavetable generator.

Writes the 24 built-in waves into factory_waves/<Category>/<Name>.wav:

    Sub       : 4 waves
    Bass      : 5 waves
    Lead      : 5 waves
    Pad-Keys  : 5 waves
    FX        : 5 waves

Every wave is one 2048-sample single-cycle shape, mono 16-bit PCM at 44.1 kHz.
The plugin embeds the resulting .wav files with juce_add_binary_data, so once
they exist here you never need the Python again to build the plugin.

Run it from this directory:

    python3 factory_waves_gen.py
"""

import math, wave, struct, os

N, SR = 2048, 44100
OUT = "factory_waves"


def write(cat, name, samples):
    os.makedirs(os.path.join(OUT, cat), exist_ok=True)
    peak = max(abs(s) for s in samples) or 1.0
    norm = [s / peak * 0.89 for s in samples]
    with wave.open(os.path.join(OUT, cat, name + ".wav"), "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes(struct.pack("<%dh" % N, *[int(s * 32767) for s in norm]))


def harm(weights):
    return [sum(a * math.sin(2 * math.pi * h * i / N) for h, a in weights.items()) for i in range(N)]


def saw_stack(n_harm=24):
    return harm({h: 1.0 / h for h in range(1, n_harm + 1)})


def pulse(width=0.5):
    return [1.0 if (i / N) < width else -1.0 for i in range(N)]


write("Sub", "Sub Sine", harm({1: 1.0}))
write("Sub", "Fat 808", harm({1: 1.0, 2: 0.35, 3: 0.15}))
write("Sub", "Deep Triangle", [2 * abs(2 * (i / N % 1)) - 1 for i in range(N)])
write("Sub", "Sub Pulse", pulse(0.25))
write("Bass", "Reese", [math.sin(2 * math.pi * i / N) + 0.6 * math.sin(2 * math.pi * 2 * i / N + 0.7) + 0.4 * math.sin(2 * math.pi * 3 * i / N + 1.9) for i in range(N)])
write("Bass", "Hollow Bass", harm({1: 1.0, 2: 0.15, 3: 0.5, 5: 0.2}))
write("Bass", "Acid Saw", saw_stack(16))
write("Bass", "Wobble Fifth", harm({1: 1.0, 2: 0.4, 3: 0.55, 4: 0.2}))
write("Bass", "Growl Lite", harm({1: 1.0, 2: 0.5, 3: 0.4, 4: 0.3, 5: 0.25, 6: 0.2, 7: 0.15}))
write("Lead", "Classic Saw", saw_stack(24))
write("Lead", "Square 50", pulse(0.5))
write("Lead", "Pulse 25", pulse(0.25))
write("Lead", "Bright Pluck", harm({h: 1.0 / (h ** 1.5) for h in range(1, 20)}))
write("Lead", "Nasty Lead", harm({1: 1.0, 2: 0.7, 3: 0.6, 4: 0.5, 5: 0.45, 6: 0.4, 8: 0.3}))
write("Pad-Keys", "Soft Saw", harm({h: 1.0 / (h ** 2) for h in range(1, 10)}))
write("Pad-Keys", "Airy", harm({1: 0.7, 2: 0.3, 8: 0.15, 12: 0.1, 16: 0.08}))
write("Pad-Keys", "E-Piano", harm({1: 1.0, 2: 0.4, 4: 0.18, 8: 0.06}))
write("Pad-Keys", "Mellow Keys", harm({1: 1.0, 2: 0.25, 3: 0.12}))
write("Pad-Keys", "Glass", harm({1: 0.8, 2: 0.3, 5: 0.25, 9: 0.15, 13: 0.1}))
write("FX", "Vocal Ah", harm({1: 0.9, 2: 0.5, 3: 0.7, 4: 0.3, 5: 0.5, 6: 0.2}))
write("FX", "Bellish", harm({1: 0.8, 2: 0.35, 3: 0.2, 5: 0.4, 7: 0.25}))
write("FX", "Digital Crush", [round(math.sin(2 * math.pi * i / N) * 4) / 4 for i in range(N)])
import random
random.seed(7)
write("FX", "Static Cycle", [random.uniform(-1, 1) for _ in range(N)])
write("FX", "Sweep Up", [math.sin(2 * math.pi * (i / N + 0.5 * (i / N) ** 2)) for i in range(N)])
print("done")
