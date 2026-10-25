#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "SamplerEngine.h"

/**
    The parameter IDs. These strings are shared by the processor, the editor and
    shared/parameter_schema.json — change all three together.
*/
namespace ParamIDs
{
    /**
        Per-slot parameters, in panel order:
        [0] Tune, [1] Level, [2] Attack, [3] Release, [4] Start, [5] End.
    */
    inline constexpr const char* slotIds[SamplerSlots::numSlots][6] =
    {
        { "slot1_tune", "slot1_level", "slot1_attack", "slot1_release", "slot1_start", "slot1_end" },
        { "slot2_tune", "slot2_level", "slot2_attack", "slot2_release", "slot2_start", "slot2_end" },
        { "slot3_tune", "slot3_level", "slot3_attack", "slot3_release", "slot3_start", "slot3_end" },
        { "slot4_tune", "slot4_level", "slot4_attack", "slot4_release", "slot4_start", "slot4_end" },
        { "slot5_tune", "slot5_level", "slot5_attack", "slot5_release", "slot5_start", "slot5_end" },
        { "slot6_tune", "slot6_level", "slot6_attack", "slot6_release", "slot6_start", "slot6_end" },
        { "slot7_tune", "slot7_level", "slot7_attack", "slot7_release", "slot7_start", "slot7_end" },
        { "slot8_tune", "slot8_level", "slot8_attack", "slot8_release", "slot8_start", "slot8_end" }
    };

    /** One-Shot (0) or Loop (1), per slot. */
    inline constexpr const char* slotModes[SamplerSlots::numSlots] =
    {
        "slot1_mode", "slot2_mode", "slot3_mode", "slot4_mode",
        "slot5_mode", "slot6_mode", "slot7_mode", "slot8_mode"
    };

    inline constexpr auto filterCutoff    = "filter_cutoff";
    inline constexpr auto filterResonance = "filter_resonance";
    inline constexpr auto masterLevel     = "master_level";
}

//==============================================================================
/**
    The VST3 wrapper. Its jobs:

      * tell the host this is a stereo instrument with MIDI in,
      * forward the knobs into the SamplerEngine once per block,
      * decode WAV and MP3 files into the eight slots and remember their paths,
      * turn MIDI notes into voices at the exact sample they arrived on,
      * save and restore the knobs, the slot paths and which slot the keyboard
        plays.

    All of the sample playback lives in SamplerEngine.cpp.
*/
class Koola10SamplerAudioProcessor : public juce::AudioProcessor,
                                     private juce::Timer
{
public:
    Koola10SamplerAudioProcessor();
    ~Koola10SamplerAudioProcessor() override;

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
    double getTailLengthSeconds() const override { return 5.0; }

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
    /**
        What the panel needs to know about one slot. `missing` means the state
        remembered a path that is not there any more: the slot keeps showing its
        name and stays silent.
    */
    struct SlotInfo
    {
        juce::String name;
        juce::String path;
        bool loaded = false;
        bool missing = false;
    };

    SlotInfo getSlotInfo (int slot) const;

    /** The audio in a slot, for the panel's waveform. Message thread. */
    std::shared_ptr<const LoadedSample> getSlotSample (int slot) const;

    /** Message thread. Decodes a WAV or MP3 into a slot. Returns false, and puts
        the slot into its missing state, if the file cannot be read. */
    bool loadSlot (int slot, const juce::File& file);

    /** Message thread. Used by the state restore. */
    bool loadSlotFromPath (int slot, const juce::String& path, const juce::String& knownName);

    void clearSlot (int slot);

    /** Which slot the keyboard plays. Lives in the plugin state. */
    void setPlaySlot (int slot);
    int getPlaySlot() const noexcept { return engine.getPlaySlot(); }

    juce::AudioFormatManager& getFormatManager() noexcept { return formats; }

    //==============================================================================
    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept { return parameters; }
    SamplerEngine& getEngine() noexcept { return engine; }

    //==============================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    /** Message thread only. Publishes a decoded sample and its panel info. */
    void publishSlot (int slot, std::shared_ptr<const LoadedSample> sample,
                      const juce::String& name, const juce::String& path,
                      bool missing);

    void pushParametersToEngine();
    void timerCallback() override;

    SamplerEngine engine;
    juce::AudioProcessorValueTreeState parameters;
    juce::AudioFormatManager formats;

    // Slot paths and names are read by the host's save thread as well as by the
    // panel, so they are guarded.
    mutable juce::CriticalSection slotLock;
    std::array<SlotInfo, SamplerSlots::numSlots> slotInfos;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Koola10SamplerAudioProcessor)
};
