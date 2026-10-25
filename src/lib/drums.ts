/**
 * Shared descriptions of the Koola10 Drum Machine.
 *
 * These mirror shared/parameter_schema.json in the plugin folder, so the site
 * and the C++ parameter layout stay described in the same words.
 */

export type DrumPad = {
  index: number;
  /** Must match the parameter ID prefix and the schema file. */
  id: string;
  name: string;
  midiNote: number;
  /** Note name shown next to the MIDI number. */
  noteName: string;
  synthesis: string;
  whatItIs: string;
  tryThis: string;
};

export const PADS: DrumPad[] = [
  {
    index: 0,
    id: "kick",
    name: "Kick",
    midiNote: 36,
    noteName: "C2",
    synthesis: "Sine with a fast pitch drop, a 1.8 ms beater click, 0.14 s to 1.44 s tail",
    whatItIs:
      "808-style kick. The pitch envelope is what makes it fall instead of sweep.",
    tryThis:
      "Tune 50 %, Decay 55 % is a punchy kick that leaves room for a bass line. Tune 35 % is a long subby 808.",
  },
  {
    index: 1,
    id: "snare",
    name: "Snare",
    midiNote: 38,
    noteName: "D2",
    synthesis: "Two body tones (185 Hz and 1.78×) plus band-passed noise near 1.75 kHz",
    whatItIs: "A noise burst over a short tuned body, the classic snare recipe.",
    tryThis:
      "Decay 30 % is dry and tight. Decay 70 % puts a room behind the hit.",
  },
  {
    index: 2,
    id: "hat_closed",
    name: "Closed Hat",
    midiNote: 42,
    noteName: "F#2",
    synthesis: "High-passed noise at about 7.8 kHz, 18 ms to 118 ms",
    whatItIs:
      "The short tick. It also chokes the open hat, the way a real hi-hat pedal closes the cymbal.",
    tryThis:
      "Decay 25 % for tight sixteenths, 60 % when the loop should feel looser.",
  },
  {
    index: 3,
    id: "hat_open",
    name: "Open Hat",
    midiNote: 46,
    noteName: "A#2",
    synthesis: "The same noise source held open, 70 ms to 820 ms",
    whatItIs: "The wash of the closed hat sound, held.",
    tryThis:
      "Decay 80 % on the last off-eighth lifts the bar into the next one.",
  },
  {
    index: 4,
    id: "clap",
    name: "Clap",
    midiNote: 39,
    noteName: "D#2",
    synthesis: "Band-passed noise near 1.05 kHz, re-hit three times 10 ms apart",
    whatItIs:
      "The flam of a room full of hands, not a single burst. That is why it does not sound like a noise hit.",
    tryThis: "Layer it with the snare on the backbeat, or leave it there alone.",
  },
  {
    index: 5,
    id: "rim",
    name: "Rim",
    midiNote: 37,
    noteName: "C#2",
    synthesis: "Resonant sine near 1.75 kHz plus a filtered click",
    whatItIs: "A short resonant ping, good for syncopated ticks.",
    tryThis:
      "One per bar on an off-beat. It is the cheapest way to stop a loop sounding mechanical.",
  },
  {
    index: 6,
    id: "tom",
    name: "Tom",
    midiNote: 45,
    noteName: "A2",
    synthesis: "Sine near 118 Hz with a 1.5× pitch drop and a small noise attack",
    whatItIs: "A tunable tom, meant for fills.",
    tryThis: "Tune 65 % on the last two sixteenths of the bar.",
  },
  {
    index: 7,
    id: "crash",
    name: "Crash",
    midiNote: 49,
    noteName: "C#3",
    synthesis: "High-passed noise near 4.3 kHz plus three inharmonic partials",
    whatItIs:
      "A short crash wash. Deliberately not a cymbal library: it is a drum accent.",
    tryThis: "One on beat 1 only. Any more and it turns into noise.",
  },
];

export type PadKnob = {
  label: string;
  whatItDoes: string;
  whatYouHear: string;
};

export const PAD_KNOBS: PadKnob[] = [
  {
    label: "Tune",
    whatItDoes:
      "0 % is one octave down, 50 % is the pad's designed pitch, 100 % is one octave up. On kick, tom and snare it moves the fundamental; on hats, clap and crash it moves the noise filter.",
    whatYouHear:
      "Lower is heavier and darker, higher is tighter and brighter.",
  },
  {
    label: "Decay",
    whatItDoes:
      "How long the pad rings. Each pad has its own musical range (18 ms to 1.7 s across the kit), so 50 % does something different on a closed hat than on a crash.",
    whatYouHear:
      "Short is a tight knock, long is a tail that hangs under the next beat.",
  },
  {
    label: "Level",
    whatItDoes: "How loud this pad is in the kit. It changes the balance, not the tone.",
    whatYouHear: "The kit leaning towards the kick, or towards the hats.",
  },
];

export type GlobalControl = {
  id: string;
  label: string;
  min: string;
  max: string;
  defaultValue: string;
  whatItDoes: string;
  whatYouHear: string;
};

export const GLOBAL_CONTROLS: GlobalControl[] = [
  {
    id: "filter_cutoff",
    label: "Filter Cutoff",
    min: "20 Hz",
    max: "20 kHz",
    defaultValue: "12.00 kHz",
    whatItDoes:
      "Sets where the shared low-pass starts rolling off the whole kit. One filter, placed after all eight pads.",
    whatYouHear:
      "Down goes dark and muffled, up opens to full brightness. This is the knob that keeps the kick clear of a bass line.",
  },
  {
    id: "filter_resonance",
    label: "Filter Resonance",
    min: "0 %",
    max: "100 %",
    defaultValue: "0 %",
    whatItDoes:
      "How much the filter emphasises the frequencies at the cutoff. 0.707 in JUCE terms is neutral.",
    whatYouHear:
      "A whistle appears at the cutoff and the kit turns squelchy.",
  },
  {
    id: "swing",
    label: "Swing",
    min: "0 %",
    max: "100 %",
    defaultValue: "0 %",
    whatItDoes:
      "Delays every off-beat sixteenth. At 100 % an off-beat lands two thirds of the way through its pair, the classic triplet shuffle.",
    whatYouHear: "Straight at 0 %, nudging at 30 %, a full shuffle near 100 %.",
  },
  {
    id: "master_level",
    label: "Master Level",
    min: "0 %",
    max: "100 %",
    defaultValue: "75 %",
    whatItDoes: "The final volume of the whole kit, applied after the filter.",
    whatYouHear: "Louder or quieter, nothing else changes.",
  },
];

export const SEQUENCER_FACTS: { title: string; body: string }[] = [
  {
    title: "16 steps, one row per pad",
    body: "Click a cell to turn it on. White means on, the column under the play head is highlighted. CLEAR empties the grid.",
  },
  {
    title: "Tempo follows the DAW",
    body: "There is no BPM control in the plugin, so it can never fight FL Studio's tempo. The panel shows the tempo it is following.",
  },
  {
    title: "Sample-accurate hits",
    body: "Each hit is rendered at its exact sample inside the block, not at the start of it, so the groove sits tight against the rest of the project.",
  },
  {
    title: "Saved with the project",
    body: "The grid and every knob are stored in the plugin state, so reopening the project brings the whole beat back.",
  },
];

export const FACTORY_PATTERN: { pad: string; steps: string }[] = [
  { pad: "Kick", steps: "1, 5, 9, 13" },
  { pad: "Snare", steps: "5, 13" },
  { pad: "Clap", steps: "13" },
  { pad: "Closed hat", steps: "1, 3, 5, 7, 9, 11, 13" },
  { pad: "Open hat", steps: "15" },
  { pad: "Rim", steps: "8" },
  { pad: "Tom", steps: "12, 16" },
  { pad: "Crash", steps: "1" },
];

export const DRUM_DOWNLOAD_BASE = "/koola10-drum-machine";

export const DRUM_DOWNLOAD_FILES = [
  { path: "README.md", label: "README.md", note: "Beginner build guide, pad guide, code map" },
  {
    path: "plugin-juce/CMakeLists.txt",
    label: "plugin-juce/CMakeLists.txt",
    note: "Build recipe: VST3 + Standalone, JUCE, C++17",
  },
  {
    path: "plugin-juce/Source/DrumEngine.h",
    label: "plugin-juce/Source/DrumEngine.h",
    note: "Pad list, MIDI map, engine declaration",
  },
  {
    path: "plugin-juce/Source/DrumEngine.cpp",
    label: "plugin-juce/Source/DrumEngine.cpp",
    note: "The eight synthesised pads, filter and output",
  },
  {
    path: "plugin-juce/Source/Sequencer.h",
    label: "plugin-juce/Source/Sequencer.h",
    note: "The 16-step grid and its clock",
  },
  {
    path: "plugin-juce/Source/Sequencer.cpp",
    label: "plugin-juce/Source/Sequencer.cpp",
    note: "Pattern storage, swing, host tempo tracking",
  },
  {
    path: "plugin-juce/Source/PluginProcessor.h",
    label: "plugin-juce/Source/PluginProcessor.h",
    note: "Parameter IDs and the VST3 wrapper",
  },
  {
    path: "plugin-juce/Source/PluginProcessor.cpp",
    label: "plugin-juce/Source/PluginProcessor.cpp",
    note: "Parameter layout, MIDI pads, state save",
  },
  {
    path: "plugin-juce/Source/PluginEditor.h",
    label: "plugin-juce/Source/PluginEditor.h",
    note: "Pad, step and knob components",
  },
  {
    path: "plugin-juce/Source/PluginEditor.cpp",
    label: "plugin-juce/Source/PluginEditor.cpp",
    note: "Look-and-feel, layout, painting",
  },
  {
    path: "shared/parameter_schema.json",
    label: "shared/parameter_schema.json",
    note: "Every parameter and pad in one machine-readable file",
  },
];

export function drumDownloadUrl(path: string) {
  return `${DRUM_DOWNLOAD_BASE}/${path}`;
}

export const DRUM_ZIP = "/koola10-drum-machine-v1.zip";
