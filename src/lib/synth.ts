/**
 * Shared descriptions of the Koola10 Synth parameter set.
 *
 * These mirror shared/parameter_schema.json in the plugin repo, so the website
 * and the C++ parameter layout stay described in the same words.
 */

export type ParamGroup = "Oscillator" | "Filter" | "Amplitude" | "Output";

export type SynthParam = {
  /** Must match the parameter ID used in the C++ code. */
  id: string;
  label: string;
  group: ParamGroup;
  dspStage: 1 | 2 | 3 | 4;
  min: number;
  max: number;
  step: number;
  default: number;
  unit: string;
  whatItDoes: string;
  whatYouHear: string;
  /** "log" spreads low frequencies out so a cutoff slider feels musical. */
  curve?: "log";
  /** Formats a raw value for display. */
  format: (value: number) => string;
};

const percent = (value: number) => `${Math.round(value * 100)} %`;
const seconds = (value: number) =>
  value < 1 ? `${Math.round(value * 1000)} ms` : `${value.toFixed(2)} s`;
const hertz = (value: number) =>
  value >= 1000 ? `${(value / 1000).toFixed(2)} kHz` : `${Math.round(value)} Hz`;

export const PARAMETERS: SynthParam[] = [
  {
    id: "wavetable_position",
    label: "Wavetable Position",
    group: "Oscillator",
    dspStage: 1,
    min: 0,
    max: 1,
    step: 0.001,
    default: 0,
    unit: "%",
    whatItDoes:
      "Chooses where you are inside the loaded wavetable. 0 sits on the first frame, 1 on the last, and it slides smoothly between every frame in between.",
    whatYouHear:
      "It re-shapes the raw tone while a note plays. Low values sound round and hollow, high values sound bright and buzzy.",
    format: percent,
  },
  {
    id: "filter_cutoff",
    label: "Filter Cutoff",
    group: "Filter",
    dspStage: 2,
    min: 20,
    max: 20000,
    step: 1,
    default: 2500,
    unit: "Hz",
    curve: "log",
    whatItDoes:
      "Sets the frequency where the low-pass filter starts removing energy. Everything below passes through; everything above is rolled off.",
    whatYouHear:
      "Turn it down and the note goes dark and muffled. Turn it up and it opens out to full brightness.",
    format: hertz,
  },
  {
    id: "filter_resonance",
    label: "Filter Resonance",
    group: "Filter",
    dspStage: 2,
    min: 0.1,
    max: 10,
    step: 0.001,
    default: 0.707,
    unit: "Q",
    whatItDoes:
      "How much the filter boosts the frequencies right around the cutoff point. 0.707 is neutral in JUCE terms.",
    whatYouHear:
      "A whistle appears at the cutoff frequency and the note takes on a vocal, quacking character. Near the top the filter rings on its own.",
    format: (value) =>
      percent(Math.min(1, Math.max(0, (value - 0.707) / (10 - 0.707)))),
  },
  {
    id: "amp_attack",
    label: "Attack",
    group: "Amplitude",
    dspStage: 3,
    min: 0.001,
    max: 5,
    step: 0.001,
    default: 0.01,
    unit: "s",
    whatItDoes:
      "How long the note takes to reach full volume after you press a key.",
    whatYouHear:
      "Near zero is an instant click: a pluck or a percussive hit. A few hundred milliseconds is a swell, like a bowed string.",
    format: seconds,
  },
  {
    id: "amp_decay",
    label: "Decay",
    group: "Amplitude",
    dspStage: 3,
    min: 0.001,
    max: 5,
    step: 0.001,
    default: 0.3,
    unit: "s",
    whatItDoes:
      "How long it takes to fall from full volume down to the sustain level, after attack finishes.",
    whatYouHear:
      "Short decay gives a knock that drops away quickly. Long decay is a smoother slide into the held note.",
    format: seconds,
  },
  {
    id: "amp_sustain",
    label: "Sustain",
    group: "Amplitude",
    dspStage: 3,
    min: 0,
    max: 1,
    step: 0.001,
    default: 0.7,
    unit: "level",
    whatItDoes:
      "The level the note holds at while the key is still down, after attack and decay have finished.",
    whatYouHear:
      "Full means the note stays at full volume while held, like an organ. At zero the note dies away even though your finger is still on the key.",
    format: percent,
  },
  {
    id: "amp_release",
    label: "Release",
    group: "Amplitude",
    dspStage: 3,
    min: 0.001,
    max: 10,
    step: 0.001,
    default: 0.4,
    unit: "s",
    whatItDoes: "How long the note takes to fade to silence after you let go.",
    whatYouHear:
      "Very short and the note stops dead. Long release leaves a tail hanging in the air after your hand has moved on.",
    format: seconds,
  },
  {
    id: "output_gain",
    label: "Master Level",
    group: "Output",
    dspStage: 4,
    min: 0,
    max: 1,
    step: 0.01,
    default: 0.7,
    unit: "%",
    whatItDoes:
      "The final volume of the plugin as a percentage of full level, applied after everything else, including the envelope.",
    whatYouHear:
      "Louder or quieter, nothing else changes. Use it to match the synth against the other instruments in a project.",
    format: percent,
  },
];

export const PARAM_GROUPS: { group: ParamGroup; dspStage: number; blurb: string }[] = [
  {
    group: "Oscillator",
    dspStage: 1,
    blurb: "Reads the wavetable and blends between frames to make the raw tone.",
  },
  {
    group: "Filter",
    dspStage: 2,
    blurb: "A juce::dsp low-pass that sits directly after the oscillator.",
  },
  {
    group: "Amplitude",
    dspStage: 3,
    blurb: "A juce::ADSR envelope that shapes the volume of every note.",
  },
  {
    group: "Output",
    dspStage: 4,
    blurb: "The final gain stage, applied last so nothing downstream clips.",
  },
];

export type PatchValues = {
  wavetablePosition: number;
  filterCutoff: number;
  filterResonance: number;
  attack: number;
  decay: number;
  sustain: number;
  release: number;
  outputGain: number;
};

/** Maps a parameter ID from the schema to the matching patch field. */
export const PATCH_KEY: Record<string, keyof PatchValues> = {
  wavetable_position: "wavetablePosition",
  filter_cutoff: "filterCutoff",
  filter_resonance: "filterResonance",
  amp_attack: "attack",
  amp_decay: "decay",
  amp_sustain: "sustain",
  amp_release: "release",
  output_gain: "outputGain",
};

export const DEFAULT_PATCH: PatchValues = {
  wavetablePosition: 0,
  filterCutoff: 2500,
  filterResonance: 0.707,
  attack: 0.01,
  decay: 0.3,
  sustain: 0.7,
  release: 0.4,
  outputGain: 0.7,
};

/** The four DSP stages, in the order the audio travels through them. */
export const SIGNAL_CHAIN = [
  {
    stage: 1,
    name: "Wavetable oscillator",
    file: "plugin-juce/Source/SynthEngine.cpp",
    marker: "STAGE 1 — Wavetable oscillator",
    api: "std::vector<float> table + linear interpolation",
    detail:
      "Slices a loaded .wav into fixed 2048-sample frames (the last one zero-padded), then reads the two frames nearest the Position knob and linearly crossfades between them, sample by sample.",
  },
  {
    stage: 2,
    name: "Low-pass filter",
    file: "plugin-juce/Source/SynthEngine.cpp",
    marker: "STAGE 2 — Low-pass filter (juce::dsp)",
    api: "juce::dsp::StateVariableTPTFilter<float>",
    detail:
      "Processes the oscillator's buffer in place, so it is physically downstream of stage 1. Cutoff is smoothed across each block so knob moves sweep instead of stepping.",
  },
  {
    stage: 3,
    name: "ADSR amplitude envelope",
    file: "plugin-juce/Source/SynthEngine.cpp",
    marker: "STAGE 3 — ADSR amplitude envelope",
    api: "juce::ADSR",
    detail:
      "applyEnvelopeToBuffer() multiplies every sample by how loud the note should be at that instant. Note on/off reaches it from the MIDI events handled in PluginProcessor.cpp.",
  },
  {
    stage: 4,
    name: "Output gain",
    file: "plugin-juce/Source/SynthEngine.cpp",
    marker: "STAGE 4 — Output gain",
    api: "juce::SmoothedValue + applyGainRamp",
    detail:
      "One smoothed multiply by the Master Level knob. Nothing after it, so the rendered VST3 output is plain stereo PCM.",
  },
];

/** Files offered for download, in the order they should be read. */
export const DOWNLOAD_FILES = [
  {
    path: "README.md",
    label: "README.md",
    note: "Beginner build guide and code map",
  },
  {
    path: "plugin-juce/CMakeLists.txt",
    label: "plugin-juce/CMakeLists.txt",
    note: "Build recipe: VST3 + Standalone, juce::dsp linked",
  },
  {
    path: "plugin-juce/Source/SynthEngine.h",
    label: "plugin-juce/Source/SynthEngine.h",
    note: "Declares the four DSP stages",
  },
  {
    path: "plugin-juce/Source/SynthEngine.cpp",
    label: "plugin-juce/Source/SynthEngine.cpp",
    note: "The oscillator, filter and envelope",
  },
  {
    path: "plugin-juce/Source/PluginProcessor.h",
    label: "plugin-juce/Source/PluginProcessor.h",
    note: "Parameter IDs and the VST3 wrapper",
  },
  {
    path: "plugin-juce/Source/PluginProcessor.cpp",
    label: "plugin-juce/Source/PluginProcessor.cpp",
    note: "Parameter layout, MIDI routing, state save",
  },
  {
    path: "plugin-juce/Source/PluginEditor.h",
    label: "plugin-juce/Source/PluginEditor.h",
    note: "Minimal dark-theme GUI declarations",
  },
  {
    path: "plugin-juce/Source/PluginEditor.cpp",
    label: "plugin-juce/Source/PluginEditor.cpp",
    note: "Custom look-and-feel and labelled knobs",
  },
  {
    path: "plugin-juce/Source/ModSource.h",
    label: "plugin-juce/Source/ModSource.h",
    note: "Modulation-source abstraction reserved for v3",
  },
  {
    path: "plugin-juce/Source/FactoryWaves.h",
    label: "plugin-juce/Source/FactoryWaves.h",
    note: "Declares the 24 embedded factory waves",
  },
  {
    path: "plugin-juce/Source/FactoryWaves.cpp",
    label: "plugin-juce/Source/FactoryWaves.cpp",
    note: "The factory wave registry",
  },
  {
    path: "plugin-juce/Source/Presets.h",
    label: "plugin-juce/Source/Presets.h",
    note: "Factory preset declarations",
  },
  {
    path: "plugin-juce/Source/Presets.cpp",
    label: "plugin-juce/Source/Presets.cpp",
    note: "The 8 factory presets",
  },
  {
    path: "shared/parameter_schema.json",
    label: "shared/parameter_schema.json",
    note: "Unified Parameter Schema entries",
  },
];

export const DOWNLOAD_BASE = "/koola10-synth";

export function downloadUrl(path: string) {
  return `${DOWNLOAD_BASE}/${path}`;
}
