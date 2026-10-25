#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <array>
#include <atomic>
#include <memory>
#include <vector>

/**
    The eight slots, in the order they appear on the panel.

    This namespace is the single place the slot list is defined: the processor,
    the engine and the editor all read it from here, so the panel, the
    parameter IDs and the saved state can never drift apart.
*/
namespace SamplerSlots
{
    inline constexpr int numSlots = 8;

    /** Panel labels. */
    inline constexpr const char* names[numSlots] =
    {
        "SLOT 1", "SLOT 2", "SLOT 3", "SLOT 4", "SLOT 5", "SLOT 6", "SLOT 7", "SLOT 8"
    };

    /** The names used inside the parameter IDs and in parameter_schema.json. */
    inline constexpr const char* shortNames[numSlots] =
    {
        "slot1", "slot2", "slot3", "slot4", "slot5", "slot6", "slot7", "slot8"
    };

    /** Sixteen voices: chords and fast runs do not choke each other. */
    inline constexpr int numVoices = 16;

    /**
        The note a sample sits at until Tune moves it. MIDI 60 is middle C:
        C3 in JUCE's octave numbering, C5 on FL Studio's piano roll.
    */
    inline constexpr int rootNote = 60;

    /** How far Tune reaches, in semitones, either way from the root. */
    inline constexpr double tuneRangeSemitones = 24.0;
}

//==============================================================================
/**
    One loaded file, decoded into memory and ready to play.

    Held behind a std::shared_ptr so a sample survives being replaced in its
    slot while a voice is still sounding it. The engine never copies the audio
    on the audio thread; it only reads.
*/
struct LoadedSample
{
    juce::AudioBuffer<float> buffer;
    double sourceSampleRate = 44100.0;
    juce::String name;          // file name, e.g. "diner_kick.wav"
    juce::String path;          // absolute path, for the saved state

    bool isStereo() const noexcept { return buffer.getNumChannels() > 1; }
    int getLength() const noexcept { return buffer.getNumSamples(); }
};

//==============================================================================
/**
    Koola10 Sampler — the sound source.

    The engine knows nothing about VST3, parameters or the GUI, exactly like
    DrumEngine and SynthEngine in their plugins. It is handed decoded audio and
    knob values, and it plays notes.

    Signal flow, in order:

        1. Sixteen sample voices      (SamplerEngine::process, step 1)
        2. Shared low-pass filter     (SamplerEngine::process, step 2)   juce::dsp
        3. Master level + safety clip (SamplerEngine::process, step 3)

    Pitching is a playback rate: the sample's own sample rate is divided into
    the host's, then multiplied by 2^((note - root + tune) / 12). No
    time-stretch, no formants: it pitch-shifts the way a tape machine does,
    which is what "tracks the keyboard like an instrument" means in v1.
*/
class SamplerEngine
{
public:
    SamplerEngine();

    //==============================================================================
    // Lifecycle — never called from the audio thread mid-block.
    //==============================================================================

    void prepare (double newSampleRate, int maximumBlockSize);
    void reset();

    //==============================================================================
    // Samples
    //
    // Loading happens on the message thread. The audio thread only ever reads
    // the raw pointer published by setSlotSample(), and a replaced sample is
    // kept alive until no voice is playing it any more
    // (see releaseRetiredSamples()).
    //==============================================================================

    void setSlotSample (int slot, std::shared_ptr<const LoadedSample> sample);
    void clearSlotSample (int slot);

    /** Message thread only: the sample currently in a slot, for the panel. */
    std::shared_ptr<const LoadedSample> getSlotSample (int slot) const;

    /** Message thread only: frees samples that were replaced and are no longer
        sounding. Call it from a timer; it is cheap and does nothing most times. */
    void releaseRetiredSamples();

    //==============================================================================
    // Slot controls (0 .. 1, read from the knobs once per block)
    //==============================================================================

    void setSlotTune (int slot, float proportion);
    void setSlotLevel (int slot, float proportion);
    void setSlotAttack (int slot, float proportion);
    void setSlotRelease (int slot, float proportion);
    void setSlotStart (int slot, float proportion);
    void setSlotEnd (int slot, float proportion);
    void setSlotLooping (int slot, bool shouldLoop);

    void setFilterCutoff (float cutoffHz);
    void setFilterResonance (float resonance);
    void setMasterLevel (float level);

    /** Which slot the keyboard plays. Set from the panel, saved with the state. */
    void setPlaySlot (int slot);
    int getPlaySlot() const noexcept { return playSlot.load(); }

    /** The shared low-pass defaults, used by prepare() and the editor. */
    static constexpr float defaultCutoffHz = 12000.0f;
    static constexpr float defaultMasterLevel = 0.8f;

    //==============================================================================
    // Playing
    //==============================================================================

    /** MIDI note on: starts the play slot's sample at that note. Audio thread. */
    void noteOn (int midiNote, float velocity);

    /** MIDI note off: releases looped voices on that note. One-shots ignore it,
        because a one-shot plays to the end by definition. Audio thread. */
    void noteOff (int midiNote);

    /** Asks for a slot to be auditioned at the root note from the GUI. Safe from
        any thread; the request is picked up at the start of the next block. */
    void requestAudition (int slot);

    /** 0 .. 1 activity for a slot's panel light. Message thread. */
    float getSlotActivity (int slot) const noexcept;

    /** How many of the sixteen voices are sounding. Message thread. */
    int getActiveVoiceCount() const noexcept { return activeVoiceCount.load(); }

    //==============================================================================
    // Rendering
    //
    // A block is rendered in one or more segments so a note that arrives
    // halfway through the block starts exactly where it arrived, rather than
    // at the start of the block. The processor calls beginBlock() once, then
    // render() for each stretch between MIDI events.
    //==============================================================================

    /** Once per block, before any render call. Takes the block size of the
        buffer that is about to be filled. */
    void beginBlock (int numBlockSamples);

    /** Renders [startSample, startSample + numSamples) additively into a buffer
        the caller has already cleared. */
    void render (juce::AudioBuffer<float>& buffer, int startSample, int numSamples);

    /** Renders a whole block on its own, for callers that have no MIDI to place. */
    void process (juce::AudioBuffer<float>& buffer);

private:
    //==============================================================================
    struct Voice
    {
        bool active = false;
        int slot = -1;
        int note = SamplerSlots::rootNote;

        const LoadedSample* sample = nullptr;

        double position = 0.0, startPosition = 0.0, endPosition = 0.0, rate = 1.0;
        bool looping = false;

        float gain = 1.0f;
        float env = 0.0f;
        float attackStep = 1.0f;
        float releaseStep = 0.0f;
        bool attacking = false;
        bool releasing = false;

        juce::uint64 age = 0;

        JUCE_DECLARE_NON_COPYABLE (Voice)
    };

    Voice* findFreeVoice();
    void startVoice (int slot, int note, float velocity);
    void stopVoice (Voice& voice) noexcept;
    void renderVoice (Voice& voice, float& left, float& right) noexcept;

    void handleAuditionRequests();
    void pushParametersToDsp();

    //==============================================================================
    double sampleRate = 44100.0;

    std::array<Voice, SamplerSlots::numVoices> voices;
    juce::uint64 voiceAge = 0;

    // The current sample per slot, owned here (message thread), and the raw
    // pointer the audio thread is allowed to use.
    std::array<std::shared_ptr<const LoadedSample>, SamplerSlots::numSlots> slotSamples;
    std::array<std::atomic<const LoadedSample*>, SamplerSlots::numSlots> published;

    struct Retired
    {
        std::shared_ptr<const LoadedSample> sample;
        int slot = 0;
        juce::uint64 retiredAtBlock = 0;
    };

    std::vector<Retired> retired;

    // Bumped once per rendered block, so the message thread can tell how much
    // audio has passed since a sample was replaced.
    std::atomic<juce::uint64> blockCounter { 0 };

    // Live voices per slot, so a replaced sample is never freed underneath a
    // voice that is still sounding it.
    std::array<std::atomic<int>, SamplerSlots::numSlots> voicesUsingSlot;

    // Written by the GUI, read once per block.
    std::array<std::atomic<float>, SamplerSlots::numSlots> slotTune;
    std::array<std::atomic<float>, SamplerSlots::numSlots> slotLevel;
    std::array<std::atomic<float>, SamplerSlots::numSlots> slotAttack;
    std::array<std::atomic<float>, SamplerSlots::numSlots> slotRelease;
    std::array<std::atomic<float>, SamplerSlots::numSlots> slotStart;
    std::array<std::atomic<float>, SamplerSlots::numSlots> slotEnd;
    std::array<std::atomic<bool>, SamplerSlots::numSlots> slotLooping;

    std::atomic<float> cutoffTarget    { defaultCutoffHz };
    std::atomic<float> resonanceTarget { 0.707f };
    std::atomic<float> masterTarget    { defaultMasterLevel };

    std::atomic<int> playSlot { 0 };

    // GUI auditions: a counter per slot, compared with the last one handled.
    std::array<std::atomic<juce::uint32>, SamplerSlots::numSlots> auditionRequest;
    std::array<juce::uint32, SamplerSlots::numSlots> handledAudition {};

    std::array<std::atomic<float>, SamplerSlots::numSlots> slotActivity;
    std::atomic<int> activeVoiceCount { 0 };

    // Master level is ramped across the block, so turning the knob does not
    // step. The ramp is computed per segment from these three values.
    float lastGain = defaultMasterLevel;
    float blockGainStart = defaultMasterLevel;
    float blockGainTarget = defaultMasterLevel;
    int blockNumSamples = 0;

    std::array<juce::dsp::StateVariableTPTFilter<float>, 2> lowPassFilters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerEngine)
};
