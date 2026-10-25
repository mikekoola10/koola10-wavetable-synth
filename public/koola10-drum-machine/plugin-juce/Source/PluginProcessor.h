#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "DrumEngine.h"
#include "Sequencer.h"

/**
    The parameter IDs. These strings are shared by the processor, the editor and
    shared/parameter_schema.json — change all three together.
*/
namespace ParamIDs
{
    /** Per-pad parameters: [0] Tune, [1] Decay, [2] Level. */
    inline constexpr const char* padIds[DrumPads::numPads][3] =
    {
        { "kick_tune",       "kick_decay",       "kick_level" },
        { "snare_tune",      "snare_decay",      "snare_level" },
        { "hat_closed_tune", "hat_closed_decay", "hat_closed_level" },
        { "hat_open_tune",   "hat_open_decay",   "hat_open_level" },
        { "clap_tune",       "clap_decay",       "clap_level" },
        { "rim_tune",        "rim_decay",        "rim_level" },
        { "tom_tune",        "tom_decay",        "tom_level" },
        { "crash_tune",      "crash_decay",      "crash_level" }
    };

    inline constexpr auto filterCutoff    = "filter_cutoff";
    inline constexpr auto filterResonance = "filter_resonance";
    inline constexpr auto swing           = "swing";
    inline constexpr auto masterLevel     = "master_level";
}

//==============================================================================
/**
    The VST3 wrapper. Its jobs:

      * tell the host this is a stereo instrument with MIDI in,
      * forward the knobs into the DrumEngine once per block,
      * turn MIDI notes and sequencer steps into pad hits at the exact sample
        they belong on,
      * save and restore the knobs and the 16-step pattern.

    All of the sound generation lives in DrumEngine.cpp, and all of the timing
    in Sequencer.cpp.
*/
class Koola10DrumMachineAudioProcessor : public juce::AudioProcessor
{
public:
    Koola10DrumMachineAudioProcessor();
    ~Koola10DrumMachineAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept { return parameters; }
    DrumEngine& getEngine() noexcept { return engine; }
    StepSequencer& getSequencer() noexcept { return sequencer; }

    //==============================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    /** A pad hit waiting to be placed on the sample it belongs to. */
    struct PendingHit
    {
        int sampleOffset = 0;
        int pad = 0;
        float velocity = 1.0f;
    };

    void pushParametersToEngine();

    DrumEngine engine;
    StepSequencer sequencer;
    juce::AudioProcessorValueTreeState parameters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Koola10DrumMachineAudioProcessor)
};
