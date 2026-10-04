#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>

#include "ModSource.h"

#include <atomic>
#include <vector>

/**
    Koola10 Synth — DSP engine.

    This class is the "sound source" part of the plugin and knows nothing about
    VST3, parameters, or the GUI. It is deliberately a plain C++ object so it can
    also be used by an offline renderer.

    Signal flow inside this file, in order:

        1. Wavetable oscillator   (SynthEngine::process, step 1)
        2. Low-pass filter        (SynthEngine::process, step 2)   juce::dsp
        3. ADSR amplitude envelope(SynthEngine::process, step 3)   juce::ADSR
        4. Output gain            (SynthEngine::process, step 4)

    Wavetables (v2)
    ---------------
    A loaded .wav is sliced into frames of 2048 samples; a short final frame is
    zero-padded. Files of 2048 samples or fewer load as a single frame, which is
    exactly the v1 behaviour. The Wavetable Position knob walks through the
    frames and linearly crossfades between the two neighbouring frames, so files
    of any length (including a 74-second beat) render a smooth, evolving tone
    instead of a single warped cycle.

    The engine is monophonic: one note at a time. See README.md ("What is not in
    v2").
*/
class SynthEngine
{
public:
    SynthEngine();

    //==============================================================================
    // Lifecycle — called by the plugin processor, never from the audio thread in
    // the middle of a block.
    //==============================================================================

    void prepare (double newSampleRate, int maximumBlockSize);
    void reset();

    //==============================================================================
    // Wavetable loading
    //==============================================================================

    /** The sample count of every wavetable frame. */
    static constexpr int frameSize = 2048;

    /** The largest table we will hold, so a huge file cannot exhaust memory. */
    static constexpr int maxFrames = 8192;

    /** Loads a .wav file from disk and slices it into 2048-sample frames.
        Returns false if the file could not be read. */
    bool loadWavetableFile (const juce::File& file);

    /** Loads a .wav that already lives in memory (used for the 24 factory waves
        baked into the binary with juce_add_binary_data). Returns false if the
        bytes are not a readable .wav. */
    bool loadWavetableFromMemory (const void* data, size_t numBytes,
                                  const juce::String& displayName);

    /** Name of the currently loaded table, for the GUI. */
    juce::String getWavetableName() const;

    int getNumFrames() const;
    int getFrameLength() const;

    /** Fills `dest` with `numPoints` samples of the post-crossfade frame the
        Position knob is currently pointing at, so the editor can draw it. */
    void fillDisplayPoints (float* dest, int numPoints) const;

    //==============================================================================
    // Parameters (plain setters, safe to call from the message thread)
    //==============================================================================

    void setWavetablePosition (float newPosition);   // 0 .. 1
    void setFilterCutoff (float cutoffHz);
    void setFilterResonance (float resonance);
    void setEnvelope (float attackSeconds, float decaySeconds,
                      float sustainLevel, float releaseSeconds);
    void setMasterGainDecibels (float decibels);

    //==============================================================================
    // Modulation (the v3 hook)
    //
    // v2 routes nothing, but every parameter a future LFO or step sequencer
    // needs to move already flows through here rather than being welded to its
    // knob. Pass nullptr to disconnect a target.
    //==============================================================================

    void setModulation (ModTarget target, ModSource* source, float depth);

    //==============================================================================
    // Note events
    //==============================================================================

    void noteOn (int midiNoteNumber, float velocity);
    void noteOff();
    void allNotesOff();

    bool isVoiceActive() const noexcept { return adsr.isActive(); }

    //==============================================================================
    /** Renders one block. `buffer` already contains the correct number of
        channels; the engine overwrites them. */
    void process (juce::AudioBuffer<float>& buffer, int startSample, int numSamples);

private:
    //==============================================================================
    static constexpr int maxInputSamples = 1 << 24;

    void buildDefaultTable();
    bool installFromReader (juce::AudioFormatReader& reader, const juce::String& displayName);
    bool installFromMono (std::vector<float> mono, const juce::String& displayName);
    float sampleAt (int frame, double normalisedPhase) const noexcept;
    void clearBlock (juce::AudioBuffer<float>& buffer, int startSample, int numSamples) const;

    double sampleRate = 44100.0;

    // --- Wavetable data --------------------------------------------------------
    // `table` is a flat mono buffer of numFrames * frameLength samples. Every
    // access is guarded by tableLock.
    std::vector<float> table;
    int numFrames = 0;
    int frameLength = 0;
    juce::String wavetableName { "Sine (built in)" };

    // Guards a wavetable swap against the audio thread reading the old one.
    mutable juce::SpinLock tableLock;

    // --- Parameter targets (written from the GUI, read on the audio thread) ---
    std::atomic<float> positionSmootherTarget  { 0.0f };
    std::atomic<float> cutoffSmootherTarget    { 2500.0f };
    std::atomic<float> gainDecibelsTarget      { -3.0f };
    std::atomic<float> resonanceTarget         { 0.707f };

    std::atomic<float> pendingAttack  { 0.01f };
    std::atomic<float> pendingDecay   { 0.30f };
    std::atomic<float> pendingSustain { 0.70f };
    std::atomic<float> pendingRelease { 0.40f };
    std::atomic<bool>  envelopeNeedsUpdate { false };

    // --- Modulation routings (lock-free; set from the message thread) ---------
    std::atomic<ModSource*> positionModSource { nullptr };
    std::atomic<float>      positionModDepth  { 0.0f };
    std::atomic<ModSource*> pitchModSource    { nullptr };
    std::atomic<float>      pitchModDepth     { 0.0f };
    std::atomic<ModSource*> amplitudeModSource { nullptr };
    std::atomic<float>      amplitudeModDepth  { 0.0f };

    // --- Smoothed parameter targets -------------------------------------------
    // These remove the "zipper noise" you would otherwise hear when a knob is
    // dragged while a note is sounding.
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> positionSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> cutoffSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> gainSmoother;

    // --- STAGE 2: low-pass filter ---------------------------------------------
    juce::dsp::StateVariableTPTFilter<float> lowPassFilter;

    // --- STAGE 3: amplitude envelope ------------------------------------------
    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParameters { 0.01f, 0.30f, 0.70f, 0.40f };

    // --- Oscillator playback state --------------------------------------------
    double phase = 0.0;            // 0 .. 1, one full cycle of the wave
    double phaseIncrement = 0.0;   // how far phase moves per sample
    float velocityGain = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthEngine)
};
