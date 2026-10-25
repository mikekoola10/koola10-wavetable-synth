# KOOLA10 SAMPLER — V1

A sample player plugin for Windows. Eight slots, WAV and MP3, and a keyboard
that plays whatever you load across the whole piano roll.

It is built with JUCE, CMake and Visual Studio, exactly like Koola10 Synth and
the drum machine, and it uses the same dark minimalist panel: hairline dividers,
monochrome knobs, per cent readouts everywhere except the filter cutoff, which
stays in kHz.

This is a **separate plugin from Koola10 Synth and the drum machine**. It lives
in the same repo but it has its own folder, its own CMake project and its own
VST3. Building one does not build or change the others.


--------------------------------------------------------------------------
1. WHAT YOU GET
--------------------------------------------------------------------------

* Eight slots, one sample each. WAV or MP3.
* Six knobs on every slot: Tune, Level, Attack, Release, Start, End.
* One play mode per slot: One-Shot or Loop.
* Sixteen voices, so chords and fast runs do not choke each other.
* Your samples play across the keyboard, pitched from the root note.
* One shared low-pass filter: Cutoff in kHz, Resonance in per cent.
* A master level.
* Every slot path and every knob is saved with your project.


--------------------------------------------------------------------------
2. WHAT YOU NEED
--------------------------------------------------------------------------

* Windows 10 or 11, 64-bit.
* Visual Studio 2022 (Community is fine). During setup, tick
  **"Desktop development with C++"**. That one box brings the compiler
  and CMake with it.
* An internet connection for the first build, so CMake can download JUCE.
* About 2 GB of free disk space for the build folder.
* FL Studio (or any DAW that loads VST3) if you want to use it in a project.
  The Standalone app needs no DAW at all.

About MP3: JUCE ships its MP3 decoder switched off, and `CMakeLists.txt` in this
folder switches it on with `JUCE_USE_MP3AUDIOFORMAT=1`. That is the whole
change, and it is what lets the plugin open your archive tracks directly.
JUCE's own source carries a note that the MP3 patent situation is the user's
responsibility; for playing back your own files on your own machine there is
nothing to do. If you would rather not have the decoder at all, delete that line
from `CMakeLists.txt` and rebuild: the plugin then loads WAV only, and the LOAD
window will not show MP3s.


--------------------------------------------------------------------------
3. WHERE IT GOES
--------------------------------------------------------------------------

The sampler sits next to the synth and the drum machine inside your existing
repo:

    C:\dev\koola10-wavetable-synth\
        plugin-juce\                 <- Koola10 Synth (already there)
        koola10-drum-machine\        <- the drum machine (already there)
        koola10-sampler\
            plugin-juce\             <- this plugin
                CMakeLists.txt
                Source\
                    SamplerEngine.h
                    SamplerEngine.cpp
                    PluginProcessor.h
                    PluginProcessor.cpp
                    PluginEditor.h
                    PluginEditor.cpp
            shared\
                parameter_schema.json
            README.md                <- this file

Nothing in `plugin-juce\` (the synth) or `koola10-drum-machine\` needs to move
or change.


--------------------------------------------------------------------------
4. HOW TO BUILD IT
--------------------------------------------------------------------------

1. Unzip so the folder lands at
   `C:\dev\koola10-wavetable-synth\koola10-sampler\` — the name of the folder in
   the archive.

2. Open Visual Studio. Choose **File -> Open -> Folder** and pick
   `C:\dev\koola10-wavetable-synth\koola10-sampler\plugin-juce`
   (the folder that contains `CMakeLists.txt`, not the one above it).

3. Visual Studio sees `CMakeLists.txt` and starts configuring on its own.
   The first time, it downloads JUCE, which takes a few minutes.

4. **Build -> Build All** (Ctrl+Shift+B). Choose **Release** for making music.
   Debug works too, and is only for stepping through code.

5. When it finishes, CMake has copied the plugin to:

       C:\Program Files\Common Files\VST3\Koola10 Sampler.vst3

   and built a Standalone app next to it in the build folder. Search the
   build folder for `Koola10 Sampler.exe`.

If you would rather use a terminal, from `koola10-sampler\plugin-juce`:

    cmake -B build -G "Visual Studio 17 2022" -A x64
    cmake --build build --config Release

(Use the generator name your Visual Studio reports. Visual Studio 2026
reports `"Visual Studio 18 2026"`.)


--------------------------------------------------------------------------
5. FIRST LAUNCH: WHAT YOU SHOULD SEE
--------------------------------------------------------------------------

Open the Standalone app. All eight slots are empty on purpose — this plugin
plays **your** sounds, it does not ship a sample library of its own.

Press **LOAD** on SLOT 1, pick a WAV or an MP3, and click the waveform: you will
hear it at the root note. The card with **KEYS** on it is the one the keyboard
plays, so notes from C3 upwards play that sound up and down the keyboard.

The panel reads, left to right:

    SLOT 1 ... SLOT 8     two columns of four cards
    SHARED                filter cutoff, resonance, master level

Every card has the file name, a waveform strip, its six knobs, the play mode,
and the KEYS button.


--------------------------------------------------------------------------
6. THE EIGHT SLOTS
--------------------------------------------------------------------------

A slot is one sample plus its six knobs. Nothing else.

    LOAD    Opens a file browser for WAV and MP3. Loading a new file into a
            slot replaces what was there.

    KEYS    Hands the keyboard to this slot. There is exactly one at a time.
            The card you picked is framed brighter, and the right-hand panel
            says which slot it is.

    waveform  Click it to hear the slot at the root note. The bright part is
            what will play: everything outside START and END is dimmed, so you
            can see your trim without opening another editor.

    ONE-SHOT / LOOP   The play mode, described in section 9.

A slot can hold up to two minutes of audio. That is far more than a one-shot or
a loop needs, and it stops a full-length track from eating a gigabyte of RAM
across eight slots.


--------------------------------------------------------------------------
7. THE SIX KNOBS ON EVERY SLOT
--------------------------------------------------------------------------

Every slot has the same six knobs in the same order, and every one of them reads
out as a percentage.

    TUNE      0 % is two octaves down, 50 % is the file exactly as recorded,
              100 % is two octaves up. This moves the note the whole slot sits
              at, so the keyboard tracks from the new pitch.

    LEVEL     How loud this slot is. It changes the balance, not the tone.

    ATTACK    Fade-in time, about 1 ms at 0 % up to about 2 s at 100 %.
              The default (2 %) is short enough to be inaudible and long enough
              to stop a click at the head of the sample.

    RELEASE   Fade-out time after you let go of a note, about 5 ms at 0 % up to
              about 3 s at 100 %. It only applies to LOOP slots; a one-shot
              always plays to its end.

    START     Where playback begins, as a percentage of the file. This is how
              you cut dead air off the head so the hit lands on the beat.

    END       Where playback ends, as a percentage of the file. In LOOP mode
              this is also the point it jumps back to START.

The label above each knob is the plain-English name. There is no hidden second
page of controls.


--------------------------------------------------------------------------
8. THE SHARED CONTROLS
--------------------------------------------------------------------------

Bottom right of the panel, in the SHARED column:

    FILTER CUTOFF   Always shown in kHz, e.g. `12.00 kHz`. One low-pass filter
                    for all eight slots, placed after everything else. Open by
                    default. Turn it down to pull a bright sample back in the
                    mix, or to take the top off a whole layer.

    RESONANCE       In per cent. 0 % is neutral. Push it up and a whistle
                    appears at the cutoff.

    MASTER LEVEL    In per cent. The final volume of the plugin.

Underneath them the panel shows how many of the sixteen voices are sounding,
the root note, and which slot the keyboard is playing.


--------------------------------------------------------------------------
9. ONE-SHOT AND LOOP
--------------------------------------------------------------------------

Each slot is in one of two modes, chosen with the box on the card.

    ONE-SHOT   Plays from START to END and stops, whatever you do with the
               note. This is the mode for a drum hit, a stab, a vocal chop: the
               sound finishes itself, so a short note in the piano roll is
               enough and a long one does not stretch it.

    LOOP       Repeats between START and END for as long as the note is held,
               then fades out over RELEASE when you let go. This is the mode
               for a steady tone or a groove: hold the note and it sustains.

To make a loop that does not click at the seam, set START and END by ear on the
waveform until the bright part begins and ends on the same point of the
waveform, then hold a note. If the seam still ticks, trim END a fraction later
or move START a fraction earlier: the click is the sample restarting at the
wrong phase, and a percent or two is usually enough.

The pitch follows the note, like a tape machine: playing two octaves up makes
the sound shorter and brighter, two octaves down makes it longer and darker.
That is deliberate — time-stretching is not in v1 (see section 15).


--------------------------------------------------------------------------
10. PLAYING FROM MIDI
--------------------------------------------------------------------------

The keyboard plays the slot marked KEYS, across the whole range:

    MIDI 60 (C3, which FL Studio calls C5)   the sample at its recorded pitch
    every semitone above                    pitched up, an octave up at 72
    every semitone below                    pitched down, an octave down at 48

Note velocity sets the slot's level, so a soft note in the piano roll is a soft
hit. Sixteen notes can sound at once; if a seventeenth arrives, the oldest one
is the one that makes way.

Notes that arrive halfway through a block start halfway through the block, not
at the edge, so the piano roll sits tight against the drums.

To play a different sound, press **KEYS** on another slot. That choice is saved
with the project, so the right slot comes back when you reopen it.


--------------------------------------------------------------------------
11. USING IT IN FL STUDIO
--------------------------------------------------------------------------

1. In the Channel Rack, add a channel and pick **Koola10 Sampler** from the
   VST3 list. If it is not there, rescan VST3 plugins (Options -> Manage
   plugins), then look under Installed -> Generators.

2. Press LOAD on a slot and pick a file from your own library.

3. Press KEYS on that slot, then draw notes in the piano roll.

4. To use two sounds in one project, either add the plugin twice (one instance
   per sound, each with its own slot), or load both into slots and switch KEYS
   when you write the part. A second instance is usually the simpler choice.

5. Mixing it with Koola10 Synth's bass: the shared low-pass is the first thing
   to reach for. Closing it to 6 or 8 kHz takes the fizz off a bright sample
   without touching its level.

Rewriting a slot's sound: LOAD the same file again and change the knobs. The
knobs belong to the slot, not the file, so a slot keeps its settings until you
change them.


--------------------------------------------------------------------------
12. HOW TO CHECK IT BY EAR
--------------------------------------------------------------------------

This is the acceptance list, written the way you would actually listen:

* Load one of your own beat one-shots and play it across two octaves of the
  piano roll. It should track the pitch cleanly and still sound like your
  sound, not a chipmunk. If it turns glassy at the top of the range, that is
  the sample itself running out of top end, not the tuning.

* Trim a loop by ear with START and END, set the slot to LOOP, and hold a note.
  It should repeat without a click at the seam. If there is a tick, nudge END
  and START as described in section 9.

* Every knob should do what its label says, on its own, in per cent.
  FILTER CUTOFF is the one exception: it reads in kHz on purpose.

* Close and reopen the project. All eight slots should come back with their
  samples and settings intact.

* Move one of the sample files somewhere else, reopen the project, and the slot
  should show its name in red and stay silent. Nothing should crash. Put the
  file back and it plays again.


--------------------------------------------------------------------------
13. CODE MAP
--------------------------------------------------------------------------

    plugin-juce\CMakeLists.txt          Build recipe: VST3 + Standalone, JUCE, C++17, MP3 on
    plugin-juce\Source\SamplerEngine.h    Slot list, voice declaration, engine
    plugin-juce\Source\SamplerEngine.cpp  Pitch tracking, voices, trim, loop, filter, output
    plugin-juce\Source\PluginProcessor.h  Parameter IDs and the VST3 wrapper
    plugin-juce\Source\PluginProcessor.cpp  Parameter layout, file decoding, MIDI, state
    plugin-juce\Source\PluginEditor.h     Panel components
    plugin-juce\Source\PluginEditor.cpp   Look-and-feel, cards, waveform, layout
    shared\parameter_schema.json        Every parameter in one machine-readable file

Inside `SamplerEngine.cpp` the audio travels through three marked stages, in this
order:

    STAGE 1 — Sixteen sample voices, summed
    STAGE 2 — Shared low-pass filter (juce::dsp), one per output channel
    STAGE 3 — Master level, then a plain clamp

`SamplerSlots` (in `SamplerEngine.h`) is the single source of truth for the slot
order, the panel labels, the parameter ID names and the voice count. Change it
there and the panel, the IDs and the schema all agree with it.

Pitching is a playback rate, not a time-stretch: `2^((note - 60 + tune) / 12)`,
multiplied by the file's sample rate divided by the host's. Reading between
samples uses a four-point curve, which is what keeps a pitched-up one-shot from
going dull.

A sample that is replaced while a voice is still playing it is not freed
immediately: the engine keeps it until two blocks have passed and the slot is
silent, then drops it. That is why loading a new file never glitches the note
that is already sounding.


--------------------------------------------------------------------------
14. TROUBLESHOOTING
--------------------------------------------------------------------------

The plugin does not appear in FL Studio
    Confirm `Koola10 Sampler.vst3` is in
    `C:\Program Files\Common Files\VST3\`, then rescan VST3 plugins in
    FL Studio. It is an instrument (a generator), so look under Generators,
    not Effects.

LOAD shows no files, or will not open my MP3
    The browser only shows `*.wav` and `*.mp3`. If MP3s are missing, check that
    `JUCE_USE_MP3AUDIOFORMAT=1` is still in `CMakeLists.txt`, delete the
    `build\` folder and build again.

The slot name is red
    The file the project remembers is not at that path any more. Press LOAD and
    pick it again. The knobs keep their settings, so only the file needs
    pointing at.

No sound, but the waveform plays when I click it
    The piano roll plays the slot marked KEYS. If that is a different slot from
    the one you loaded, press KEYS on the loaded card. Then check Master Level,
    the slot's Level, and Filter Cutoff: with the cutoff all the way down
    almost nothing gets through.

A loop clicks at the seam
    That is the loop point, not a bug. Nudge END a percent or two later (or
    START earlier) until the bright part begins and ends on the same part of
    the waveform. Section 9 explains it.

A one-shot stops when I stop holding the note
    It should not: one-shots always play to their end. Check the mode box on
    that card. If it says LOOP, the note-off is fading it out, which is what
    LOOP does.

Chords choke each other
    There are sixteen voices. Nine-note chords with a long release can run out;
    shorten RELEASE on that slot, or add a second instance of the plugin.

Nothing I loaded is saved when I reopen the project
    The slot paths and the knobs are saved by the host when it saves the
    project. Make sure you saved the FL Studio project itself, not just the
    plugin window. Note that paths are saved as absolute paths: moving a folder
    of samples means the slots need pointing at it again.

Building fails on the first configure
    Almost always the JUCE download. Check the internet connection and that
    `git` is on your PATH, then delete the `build\` folder and try again.

"Debug Assertion Failed!" in a Debug build (debug_heap.cpp,
is_block_type_valid)
    This is a Debug-CRT heap check in the JUCE/C runtime layer, not in the
    sampler code, and it can appear in JUCE's own tools too. It comes from
    either a `build\` folder left over from an older Visual Studio install, or a
    JUCE older than 8.0.12 with Visual Studio 2026. `CMakeLists.txt` already
    fetches 8.0.12 and defines `_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR` for MSVC.
    If you still see it, delete `build\` and configure again:

        rmdir /s /q build
        cmake -B build -G "Visual Studio 17 2022" -A x64
        cmake --build build --config Debug

    Release builds are unaffected, and Release is the one to make music with.


--------------------------------------------------------------------------
15. WHAT IS NOT IN V1 (AND WHY)
--------------------------------------------------------------------------

* No auto-slicing or chopping. Trimming happens with START and END; finding the
  slices for you is a later sampler update.
* No time-stretch. The pitch follows the note, and tempo-matching is the DAW's
  job in this version.
* No built-in sample library. Your beats become the library by loading them,
  not by bundling them.
* No recording or resampling into the plugin. A later update.
* No sequencing. The drum machine already does that; this plugin plays.
* No vocal processing of any kind. Out of scope by decision, not by accident.
* No AI anything. Out of scope, and it would need a faster machine than a
  Celeron.

If one of those becomes the next thing you need, it is a new build with its own
brief, not a patch to this one.
