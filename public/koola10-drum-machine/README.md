# KOOLA10 DRUM MACHINE — V1

A drum machine plugin for Windows. Eight pads, every one of them synthesised
while you play, and a 16-step sequencer that follows your DAW's tempo.

It is built with JUCE, CMake and Visual Studio, exactly like Koola10 Synth, and
it uses the same dark minimalist panel: hairline dividers, monochrome knobs,
per cent readouts everywhere except the filter cutoff, which stays in kHz.

This is a **separate plugin from Koola10 Synth**. It lives in the same repo but
it has its own folder, its own CMake project and its own VST3. Building one does
not build or change the other.


--------------------------------------------------------------------------
1. WHAT YOU GET
--------------------------------------------------------------------------

* Eight pads: Kick, Snare, Closed Hat, Open Hat, Clap, Rim, Tom, Crash.
* Three knobs on every pad: Tune, Decay, Level. All in per cent.
* One shared low-pass filter: Cutoff in kHz, Resonance in per cent.
* Swing in per cent, and a Master Level for the whole kit.
* A 16-step grid, one row per pad, that follows the DAW's tempo.
* Playable from the FL Studio piano roll as well as from the grid.
* The pattern and every knob are saved with your project.


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


--------------------------------------------------------------------------
3. WHERE IT GOES
--------------------------------------------------------------------------

The drum machine sits next to the synth inside your existing repo:

    C:\dev\koola10-wavetable-synth\
        plugin-juce\                 <- Koola10 Synth (already there)
        koola10-drum-machine\
            plugin-juce\             <- this plugin
                CMakeLists.txt
                Source\
                    DrumEngine.h
                    DrumEngine.cpp
                    Sequencer.h
                    Sequencer.cpp
                    PluginProcessor.h
                    PluginProcessor.cpp
                    PluginEditor.h
                    PluginEditor.cpp
            shared\
                parameter_schema.json
            README.md                <- this file

Nothing in `plugin-juce\` (the synth) needs to move or change.


--------------------------------------------------------------------------
4. HOW TO BUILD IT
--------------------------------------------------------------------------

1. Unzip so the folder lands at
   `C:\dev\koola10-wavetable-synth\koola10-drum-machine\` — the name of the
   folder in the archive.

2. Open Visual Studio. Choose **File -> Open -> Folder** and pick
   `C:\dev\koola10-wavetable-synth\koola10-drum-machine\plugin-juce`
   (the folder that contains `CMakeLists.txt`, not the one above it).

3. Visual Studio sees `CMakeLists.txt` and starts configuring on its own.
   The first time, it downloads JUCE, which takes a few minutes.

4. **Build -> Build All** (Ctrl+Shift+B). Choose **Release** for making music.
   Debug works too, and is only for stepping through code.

5. When it finishes, CMake has copied the plugin to:

       C:\Program Files\Common Files\VST3\Koola10 Drum Machine.vst3

   and built a Standalone app next to it in the build folder. Search the
   build folder for `Koola10 Drum Machine.exe`.

If you would rather use a terminal, from `koola10-drum-machine\plugin-juce`:

    cmake -B build -G "Visual Studio 17 2022" -A x64
    cmake --build build --config Release

(Use the generator name your Visual Studio reports. Visual Studio 2026
reports `"Visual Studio 18 2026"`.)


--------------------------------------------------------------------------
5. FIRST LAUNCH: WHAT YOU SHOULD HEAR
--------------------------------------------------------------------------

Open the Standalone app. It starts with a pattern already drawn and plays it
straight away at 130 BPM, the tempo of the Diner beat:

    Kick        beats 1, 2, 3, 4
    Snare       beats 2 and 4
    Clap        beat 4, layered with the snare
    Closed hat  every off-eighth
    Open hat    the last off-eighth of the bar
    Rim         the "and" of beat 2
    Tom         end of beat 3, and the last sixteenth
    Crash       beat 1

That is the whole point of the shipped pattern: you should be able to press
play in your DAW and hear a finished drum bed, not a demo.

In the Standalone there is no DAW transport, so the sequencer runs on its own
clock and the panel says **INTERNAL CLOCK**. In FL Studio it says **HOST CLOCK**
and the grid only moves while the transport is playing.


--------------------------------------------------------------------------
6. THE EIGHT PADS
--------------------------------------------------------------------------

Every pad is generated from scratch, sample by sample. There is nothing to
load and nothing to lose.

    1. KICK        A sine wave whose pitch falls fast, plus a 1.8 ms click.
                   This is the signature sound. Its pitch drop is what makes
                   it fall instead of sweep.
                   Try: Decay 55 %, Tune 50 % = a punchy kick that leaves room
                   for the bass. Tune 35 % = a long subby 808.

    2. SNARE       Two short body tones with a band-passed noise burst on top,
                   around 1.75 kHz.
                   Try: Decay 30 % = a dry backbeat. Decay 70 % = a snare with
                   room around it.

    3. CLOSED HAT  High-passed noise, 18 to 118 ms.
                   It also chokes the open hat, the way a real hi-hat pedal
                   closes the cymbal. That is deliberate, not a bug.
                   Try: Decay 25 % for tight sixteenths, 60 % for a looser feel.

    4. OPEN HAT    The same noise source, held open for 70 to 820 ms.
                   Try: Decay 80 % and put it on the last off-eighth to lift
                   the bar into the next one.

    5. CLAP        Band-passed noise around 1.05 kHz, re-hit three times about
                   10 ms apart, then a short tail. That flam is what makes a
                   clap sound like a room full of hands.
                   Try: layered with the snare on beat 4.

    6. RIM         A resonant ping near 1.75 kHz plus a click.
                   Try: one per bar on an off-beat. It is the cheapest way to
                   make a loop sound less mechanical.

    7. TOM         A sine near 118 Hz with a 1.5x pitch drop.
                   Try: Tune 65 % on the last two sixteenths of the bar as a
                   fill.

    8. CRASH       High-passed noise plus three inharmonic partials, 0.22 to
                   1.72 s. Short on purpose: it is a drum accent, not a cymbal
                   library.
                   Try: one on beat 1 only.


--------------------------------------------------------------------------
7. THE THREE KNOBS ON EVERY PAD
--------------------------------------------------------------------------

Every pad has the same three knobs in the same order, and every one of them
reads out as a percentage:

    TUNE    0 % is one octave down, 50 % is the pad's designed pitch,
            100 % is one octave up. On the kick, tom and snare this moves the
            fundamental. On the hats, clap and crash it moves the noise filter.

    DECAY   How long the pad rings. Each pad has its own sensible range
            (18 ms to 1.7 s across the kit), so 50 % means something different
            on a closed hat than on a crash. Sections 6 and 9 list the ranges.

    LEVEL   How loud this pad is in the kit. It does not change the tone, only
            the balance against the other seven pads.

The label above each knob is the plain-English name. There is no hidden
second page of controls.


--------------------------------------------------------------------------
8. THE SHARED CONTROLS
--------------------------------------------------------------------------

Bottom right of the panel, in the GLOBAL block:

    FILTER CUTOFF   Always shown in kHz, e.g. `12.00 kHz`. One low-pass filter
                    for the whole kit, placed after all eight pads. Open by
                    default. Turn it down to pull the kit back in the mix, or
                    to keep the kick out of the way of a bass line.

    RESONANCE       In per cent. 0 % is neutral. Push it up and a whistle
                    appears at the cutoff.

    SWING           In per cent. It delays every off-beat sixteenth. At 0 % the
                    grid is straight; near 100 % the off-beats land two thirds
                    of the way through their pair, which is the classic
                    triplet shuffle.

    MASTER LEVEL    In per cent. The final volume of the whole kit.


--------------------------------------------------------------------------
9. THE SEQUENCER
--------------------------------------------------------------------------

* Sixteen steps. One row per pad. One column per sixteenth note.
* Click a cell to turn it on or off. White means on.
* The column under the play head is highlighted while the transport runs.
* **CLEAR** empties the whole grid so you can start from nothing.
* Pattern length is 16 steps in v1. There is no song mode and no pattern
  chaining; that belongs in a later build, not this one.
* **Tempo follows the DAW.** There is no BPM control in the plugin, so it can
  never fight FL Studio's tempo. The panel shows the tempo it is following.
* Hits are placed at their exact sample inside the block, not at the start of
  it, so the groove sits tight against everything else in the project.

Swing only moves the off-beat sixteenths. The downbeats stay exactly where they
are, which is why the kick does not drift when you turn swing up.

The grid is saved with the project. Close FL Studio, reopen it, and the pattern
comes back. So do the knobs.


--------------------------------------------------------------------------
10. PLAYING THE PADS FROM MIDI
--------------------------------------------------------------------------

The pads answer the General MIDI drum notes, so the FL Studio piano roll works
without any mapping:

    Kick        36  (C2)
    Snare       38  (D2)
    Closed hat  42  (F#2)
    Open hat    46  (A#2)
    Clap        39  (D#2)
    Rim         37  (C#2)
    Tom         45  (A2)
    Crash       49  (C#3)

Note velocity sets the pad's level, so a soft note in the piano roll is a soft
hit. Notes that are not in the list are ignored.


--------------------------------------------------------------------------
11. USING IT IN FL STUDIO
--------------------------------------------------------------------------

1. In the Channel Rack, add a channel and pick **Koola10 Drum Machine** from
   the VST3 list. If it is not there, rescan VST3 plugins (Options -> Manage
   plugins), then look under Installed -> Generators.

2. Press play. The grid starts moving and the shipped pattern plays in time
   with the project. Change the project tempo and the pattern follows.

3. Two ways to write a beat:
   * draw steps in the plugin's own grid, or
   * draw notes in the FL Studio piano roll on the notes in section 10.

   Both work at the same time. Use whichever is quicker for the part.

4. Mixing it with Koola10 Synth's bass: put the bass low-pass around 200 to
   300 Hz and the drum machine's shared cutoff around 8 to 12 kHz. The kick
   then keeps its punch and the bass keeps its weight. Reach for the filter
   knobs before you reach for the pad levels.


--------------------------------------------------------------------------
12. HOW TO CHECK IT BY EAR
--------------------------------------------------------------------------

This is the acceptance list, written the way you would actually listen:

* A four-bar loop at about 130 BPM with kick, snare, hats and clap should sound
  like a finished drum bed, not a demo. If it does not, the usual fixes are:
  snare Decay too long, closed hat Level too high, or the shared cutoff closed
  too far.

* The kick should still punch with the synth's bass underneath it. If the two
  turn to mud, turn the drum machine's Filter Cutoff down a little (or the
  bass's filter down), do not turn the kick down. The kick's weight is its
  whole job.

* Every knob should do what its label says, on its own, in per cent.
  FILTER CUTOFF is the one exception: it reads in kHz on purpose.

* Close and reopen the project. The pattern and every knob should come back.


--------------------------------------------------------------------------
13. CODE MAP
--------------------------------------------------------------------------

    plugin-juce\CMakeLists.txt          Build recipe: VST3 + Standalone, JUCE, C++17
    plugin-juce\Source\DrumEngine.h     Pad list, MIDI map, engine declaration
    plugin-juce\Source\DrumEngine.cpp   The eight synthesised pads, filter, output
    plugin-juce\Source\Sequencer.h      16-step grid and the clock
    plugin-juce\Source\Sequencer.cpp    Pattern storage, swing, host tempo
    plugin-juce\Source\PluginProcessor.h  Parameter IDs and the VST3 wrapper
    plugin-juce\Source\PluginProcessor.cpp  Parameter layout, MIDI, hits, state
    plugin-juce\Source\PluginEditor.h   Panel components
    plugin-juce\Source\PluginEditor.cpp Look-and-feel, layout, painting
    shared\parameter_schema.json        Every parameter in one machine-readable file

Inside `DrumEngine.cpp` the audio travels through three marked stages, in this
order:

    STAGE 1 — Eight one-shot voices, summed to mono
    STAGE 2 — Shared low-pass filter (juce::dsp)
    STAGE 3 — Master level, then a plain clamp

`DrumPads` (in `DrumEngine.h`) is the single source of truth for the pad order,
the panel labels, the parameter ID names and the MIDI notes. Change a pad name
there and the browser, the grid and the schema all agree with it.


--------------------------------------------------------------------------
14. TROUBLESHOOTING
--------------------------------------------------------------------------

The plugin does not appear in FL Studio
    Confirm `Koola10 Drum Machine.vst3` is in
    `C:\Program Files\Common Files\VST3\`, then rescan VST3 plugins in
    FL Studio. It is an instrument (a generator), so look under Generators,
    not Effects.

The grid does not move in the DAW
    The sequencer follows the transport. Press play. If the panel says
    STOPPED, the host is not running. In the Standalone it says INTERNAL
    CLOCK and runs on its own; that is expected.

No sound, but the grid is moving
    Check Master Level, then the pad's Level. Then check Filter Cutoff: with
    the cutoff all the way down, almost nothing gets through.

The open hat stops when the closed hat plays
    That is the choke, on purpose, the same as a real hi-hat pedal. Give the
    open hat a step where the closed hat is silent.

Nothing I clicked is saved when I reopen the project
    The pattern and the knobs are saved by the host when it saves the project.
    Make sure you saved the FL Studio project itself, not just the plugin
    window.

Building fails on the first configure
    Almost always the JUCE download. Check the internet connection and that
    `git` is on your PATH, then delete the `build\` folder and try again.

"Debug Assertion Failed!" in a Debug build (debug_heap.cpp,
is_block_type_valid)
    This is a Debug-CRT heap check in the JUCE/C runtime layer, not in the drum
    code, and it can appear in JUCE's own tools too. It comes from either a
    `build\` folder left over from an older Visual Studio install, or a JUCE
    older than 8.0.12 with Visual Studio 2026. `CMakeLists.txt` already fetches
    8.0.12 and defines `_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR` for MSVC. If you
    still see it, delete `build\` and configure again:

        rmdir /s /q build
        cmake -B build -G "Visual Studio 17 2022" -A x64
        cmake --build build --config Debug

    Release builds are unaffected, and Release is the one to make music with.


--------------------------------------------------------------------------
15. WHAT IS NOT IN V1 (AND WHY)
--------------------------------------------------------------------------

* No sample loading or slicing. That is the sampler's job, next in the rack,
  and it would change what this plugin is.
* No FM, keys or bass sounds. Those are separate instruments.
* No vocal processing of any kind. Out of scope by decision, not by accident.
* No song mode, pattern chaining or multiple patterns. Sixteen steps, one
  pattern, saved with the project.
* No per-step velocity or accent lanes. Velocity comes from the piano roll
  instead.
* No internal BPM. The tempo is the DAW's, always, so the two can never drift.

If one of those becomes the next thing you need, it is a new build with its own
brief, not a patch to this one.
