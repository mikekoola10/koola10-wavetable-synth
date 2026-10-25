/**
 * Shared descriptions of the Koola10 Sampler.
 *
 * These mirror shared/parameter_schema.json in the plugin folder, so the site
 * and the C++ parameter layout stay described in the same words.
 */

export type SamplerKnob = {
  label: string;
  /** Must match the parameter ID suffix in the schema file. */
  suffix: string;
  range: string;
  defaultValue: string;
  whatItDoes: string;
  whatYouHear: string;
};

export const SLOT_KNOBS: SamplerKnob[] = [
  {
    label: "Tune",
    suffix: "tune",
    range: "0 % to 100 %",
    defaultValue: "50 %",
    whatItDoes:
      "Pitches the whole slot. 0 % is two octaves down, 50 % is the file at the pitch it was recorded at, 100 % is two octaves up.",
    whatYouHear:
      "Lower is a fatter, slower sound; higher is thinner and faster. It moves the note the sample sits at, so the keyboard tracks from there.",
  },
  {
    label: "Level",
    suffix: "level",
    range: "0 % to 100 %",
    defaultValue: "80 %",
    whatItDoes:
      "How loud this slot is, before the shared filter and the master level.",
    whatYouHear: "The slot sitting under or over the others. It changes the balance, not the tone.",
  },
  {
    label: "Attack",
    suffix: "attack",
    range: "1 ms to 2 s",
    defaultValue: "2 %",
    whatItDoes: "Fade-in time, 1 ms at 0 % to about 2 s at 100 %.",
    whatYouHear:
      "A hard edge at the bottom of the knob, a soft swell at the top. The default is short enough to be inaudible and long enough to stop a click.",
  },
  {
    label: "Release",
    suffix: "release",
    range: "5 ms to 3 s",
    defaultValue: "25 %",
    whatItDoes:
      "Fade-out time after you let go of a note, about 5 ms at 0 % to about 3 s at 100 %. It applies to looped slots; one-shots play to their end.",
    whatYouHear:
      "A loop that stops dead, or one that fades away like a sustaining instrument.",
  },
  {
    label: "Start",
    suffix: "start",
    range: "0 % to 100 % of the file",
    defaultValue: "0 %",
    whatItDoes: "Where playback begins, as a percentage of the file.",
    whatYouHear:
      "Dead air at the head of a sample disappears, so the hit lands on the beat instead of after it.",
  },
  {
    label: "End",
    suffix: "end",
    range: "0 % to 100 % of the file",
    defaultValue: "100 %",
    whatItDoes:
      "Where playback ends. In Loop mode this is also the point it wraps back to Start.",
    whatYouHear:
      "The tail of a sample is cut off — the other half of making a loop that does not trail under the next bar.",
  },
];

export type PlayMode = {
  name: string;
  whatItDoes: string;
  whatYouHear: string;
};

export const PLAY_MODES: PlayMode[] = [
  {
    name: "One-Shot",
    whatItDoes: "Plays from Start to End and stops, whatever you do with the note.",
    whatYouHear:
      "A drum hit, a stab, a vocal chop: the sound finishes itself, so a short note is enough and a long one does not stretch it.",
  },
  {
    name: "Loop",
    whatItDoes:
      "Repeats between Start and End for as long as the note is held, then fades out over Release when you let go.",
    whatYouHear: "A steady tone or a groove you can hold, instead of a hit that runs out.",
  },
];

export type SharedControl = {
  id: string;
  label: string;
  min: string;
  max: string;
  defaultValue: string;
  whatItDoes: string;
  whatYouHear: string;
};

export const SHARED_CONTROLS: SharedControl[] = [
  {
    id: "filter_cutoff",
    label: "Filter Cutoff",
    min: "20 Hz",
    max: "20 kHz",
    defaultValue: "12.00 kHz",
    whatItDoes:
      "Sets where the shared low-pass starts rolling off all eight slots. One filter, placed after everything else.",
    whatYouHear:
      "Down goes dark and muffled, up opens to full brightness. The first thing to reach for when a bright sample needs to sit under a mix.",
  },
  {
    id: "filter_resonance",
    label: "Filter Resonance",
    min: "0 %",
    max: "100 %",
    defaultValue: "0 %",
    whatItDoes:
      "How much the filter emphasises the frequencies at the cutoff. 0.707 in JUCE terms is neutral.",
    whatYouHear: "A whistle appears at the cutoff and the sound turns squelchy.",
  },
  {
    id: "master_level",
    label: "Master Level",
    min: "0 %",
    max: "100 %",
    defaultValue: "80 %",
    whatItDoes: "The final volume of the whole plugin, applied after the filter.",
    whatYouHear: "Louder or quieter, nothing else changes.",
  },
];

export const SAMPLER_FACTS: { title: string; body: string }[] = [
  {
    title: "Eight slots, one sample each",
    body: "WAV or MP3, loaded from the panel. Every slot has the same six knobs in the same order, so there is nothing new to learn from slot to slot.",
  },
  {
    title: "Sixteen voices",
    body: "Chords and fast runs do not choke each other. If a seventeenth note arrives, the oldest sounding voice is the one that makes way.",
  },
  {
    title: "Played from the keyboard",
    body: "The slot marked KEYS answers the piano roll across the whole range, pitched from C3. Note velocity sets the level, so a soft note is a soft hit.",
  },
  {
    title: "Saved with the project",
    body: "Slot paths, the play mode, every knob and which slot has the keys travel in the plugin state. Reopening the project brings the instrument back.",
  },
];

export const SAMPLER_ROUTING: { key: string; value: string }[] = [
  { key: "Root note", value: "C3 · MIDI 60 · FL Studio shows C5" },
  { key: "One octave up", value: "MIDI 72" },
  { key: "One octave down", value: "MIDI 48" },
  { key: "Tune range", value: "two octaves either way, in per cent" },
  { key: "Voices", value: "16, oldest voice reused" },
  { key: "File formats", value: "WAV, MP3" },
  { key: "Longest sample", value: "two minutes per slot" },
  { key: "Missing file", value: "name shown in red, slot silent, no crash" },
];

export const SAMPLER_DOWNLOAD_BASE = "/koola10-sampler";

export const SAMPLER_DOWNLOAD_FILES = [
  { path: "README.md", label: "README.md", note: "Beginner build guide, slot guide, code map" },
  {
    path: "plugin-juce/CMakeLists.txt",
    label: "plugin-juce/CMakeLists.txt",
    note: "Build recipe: VST3 + Standalone, JUCE, MP3 switched on",
  },
  {
    path: "plugin-juce/Source/SamplerEngine.h",
    label: "plugin-juce/Source/SamplerEngine.h",
    note: "Slot list, voice declaration, engine",
  },
  {
    path: "plugin-juce/Source/SamplerEngine.cpp",
    label: "plugin-juce/Source/SamplerEngine.cpp",
    note: "Pitch tracking, trim, loop, filter, output",
  },
  {
    path: "plugin-juce/Source/PluginProcessor.h",
    label: "plugin-juce/Source/PluginProcessor.h",
    note: "Parameter IDs and the VST3 wrapper",
  },
  {
    path: "plugin-juce/Source/PluginProcessor.cpp",
    label: "plugin-juce/Source/PluginProcessor.cpp",
    note: "Parameter layout, WAV/MP3 decoding, state save",
  },
  {
    path: "plugin-juce/Source/PluginEditor.h",
    label: "plugin-juce/Source/PluginEditor.h",
    note: "Slot card and waveform components",
  },
  {
    path: "plugin-juce/Source/PluginEditor.cpp",
    label: "plugin-juce/Source/PluginEditor.cpp",
    note: "Look-and-feel, cards, waveform, layout",
  },
  {
    path: "shared/parameter_schema.json",
    label: "shared/parameter_schema.json",
    note: "Every parameter and slot in one machine-readable file",
  },
];

export function samplerDownloadUrl(path: string) {
  return `${SAMPLER_DOWNLOAD_BASE}/${path}`;
}

export const SAMPLER_ZIP = "/koola10-sampler-v1.zip";
