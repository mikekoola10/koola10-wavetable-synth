#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <array>
#include <atomic>

/**
    The eight pads, in the order they appear on the panel and in the step grid.

    This namespace is the single place the pad list is defined: the processor,
    the sequencer and the editor all read it from here, so the panel, the
    parameter IDs and the MIDI map can never drift apart.
*/
namespace DrumPads
{
    enum PadIndex
    {
        kick = 0,
        snare,
        closedHat,
        openHat,
        clap,
        rim,
        tom,
        crash,
        count
    };

    inline constexpr int numPads = (int) count;

    /** Panel labels. */
    inline constexpr const char* names[numPads] =
    {
        "KICK", "SNARE", "CLOSED HAT", "OPEN HAT", "CLAP", "RIM", "TOM", "CRASH"
    };

    /** The names used inside the parameter IDs and in parameter_schema.json. */
    inline constexpr const char* shortNames[numPads] =
    {
        "kick", "snare", "hat_closed", "hat_open", "clap", "rim", "tom", "crash"
    };

    /** General MIDI drum notes, so the FL Studio piano roll works out of the box. */
    inline constexpr int midiNotes[numPads] = { 36, 38, 42, 46, 39, 37, 45, 49 };

    inline int midiNoteForPad (int pad) noexcept
    {
        return (pad >= 0 && pad < numPads) ? midiNotes[pad] : -1;
    }

    inline int padForMidiNote (int note) noexcept
    {
        for (int i = 0; i < numPads; ++i)
            if (midiNotes[i] == note)
                return i;

        return -1;
    }
}

//==============================================================================
/**
    Koola10 Drum Machine — the sound source.

    Every pad is synthesised: there are no samples to ship, load or lose. The
    engine knows nothing about VST3, parameters or the GUI, exactly like the
    SynthEngine in Koola10 Synth.

    Signal flow, in order:

        1. Eight one-shot voices      (DrumEngine::process, step 1)
        2. Shared low-pass filter     (DrumEngine::process, step 2)   juce::dsp
        3. Master level + safety clip (DrumEngine::process, step 3)

    Each pad owns exactly one voice, so hitting a pad twice retriggers it the
    way a real drum does, and eight pads can sound at once without any voice
    allocation to manage on the audio thread.
*/
class DrumEngine
{
public:
    DrumEngine();

    //==============================================================================
    // Lifecycle — never called from the audio thread mid-block.
    //==============================================================================

    void prepare (double newSampleRate, int maximumBlockSize);
    void reset();

    //==============================================================================
    // Pad controls (0 .. 1, read from the knobs once per block)
    //==============================================================================

    void setPadTune (int pad, float proportion);
    void setPadDecay (int pad, float proportion);
    void setPadLevel (int pad, float proportion);

    void setFilterCutoff (float cutoffHz);
    void setFilterResonance (float resonance);
    void setMasterLevel (float level);

    /** The shared low-pass defaults, used by prepare() and the editor. */
    static constexpr float defaultCutoffHz = 12000.0f;
    static constexpr float defaultMasterLevel = 0.75f;

    //==============================================================================
    // Triggering
    //==============================================================================

    /** Starts a pad. Audio thread only (sequencer hits and MIDI). */
    void triggerPad (int pad, float velocity);

    /** Asks for a pad hit from the GUI. Safe from any thread; the request is
        picked up at the start of the next render call. */
    void requestPadTrigger (int pad, float velocity = 1.0f);

    /** 0 .. 1 activity for the pad's panel light. Message thread. */
    float getPadActivity (int pad) const noexcept;

    //==============================================================================
    /** Renders one segment. Called once per hit boundary so hits sit exactly on
        their step instead of at the start of the block. */
    void process (juce::AudioBuffer<float>& buffer, int startSample, int numSamples);

private:
    //==============================================================================
    /** A small state-variable filter, used to shape each pad's noise component.
        One filter per voice, so a snare's noise and a hat's noise never share
        state. */
    struct VoiceFilter
    {
        float k = 1.0f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
        float ic1 = 0.0f, ic2 = 0.0f;

        void set (double sampleRate, float cutoffHz, float resonance) noexcept;

        void process (float input, float& low, float& band, float& high) noexcept
        {
            const float v3 = input - ic2;
            const float v1 = a1 * ic1 + a2 * v3;
            const float v2 = ic2 + a2 * ic1 + a3 * v3;

            ic1 = 2.0f * v1 - ic1;
            ic2 = 2.0f * v2 - ic2;

            band = v1;
            low  = v2;
            high = input - k * v1 - v2;
        }
    };

    /** White noise, one generator per voice. */
    struct Noise
    {
        juce::uint32 state = 0x9e3779b9u;

        void seed (juce::uint32 newSeed) noexcept
        {
            state = newSeed == 0 ? 0x9e3779b9u : newSeed;
        }

        float next() noexcept
        {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;

            return (float) (juce::int32) state * 4.6566129e-10f;   // -1 .. 1
        }
    };

    struct Voice
    {
        bool active = false;
        int pad = -1;
        float velocity = 1.0f;
        float level = 1.0f;

        double phase = 0.0, phaseB = 0.0, phaseC = 0.0;
        double inc = 0.0, incB = 0.0, incC = 0.0;

        float bodyAMix = 0.0f, bodyBMix = 0.0f, bodyCMix = 0.0f;
        float noiseHighMix = 0.0f, noiseBandMix = 0.0f;
        float clickMix = 0.0f;

        float pitchEnv = 0.0f, pitchStart = 0.0f, pitchCoeff = 0.99999f;
        float ampEnv = 0.0f, ampCoeff = 0.99999f;
        float noiseEnv = 0.0f, noiseCoeff = 0.99999f;
        float clickEnv = 0.0f, clickCoeff = 0.99999f;

        VoiceFilter filter;
        Noise noise;

        // Clap flam: a handful of extra noise re-hits after the first one.
        int burstsRemaining = 0;
        int burstCountdown = 0;
        int burstInterval = 0;
    };

    void startVoice (int pad, float velocity);
    float renderVoice (Voice& voice) noexcept;

    //==============================================================================
    double sampleRate = 44100.0;

    std::array<Voice, DrumPads::numPads> voices;

    // Written by the GUI, read once per block.
    std::array<std::atomic<float>, DrumPads::numPads> padTune;
    std::array<std::atomic<float>, DrumPads::numPads> padDecay;
    std::array<std::atomic<float>, DrumPads::numPads> padLevel;

    std::atomic<float> cutoffTarget    { defaultCutoffHz };
    std::atomic<float> resonanceTarget { 0.707f };
    std::atomic<float> masterTarget    { defaultMasterLevel };

    // GUI pad hits: a counter per pad, compared with the last one we handled.
    std::array<std::atomic<juce::uint32>, DrumPads::numPads> triggerRequest;
    std::array<juce::uint32, DrumPads::numPads> handledRequest {};

    std::atomic<float> padActivity[DrumPads::numPads];

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> cutoffSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> gainSmoother;

    juce::dsp::StateVariableTPTFilter<float> lowPassFilter;

    juce::uint32 noiseSeed = 1u;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumEngine)
};
