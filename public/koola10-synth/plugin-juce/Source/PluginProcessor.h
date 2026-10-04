#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "SynthEngine.h"

/**
    The parameter IDs. These strings are shared by the plugin processor, the
    editor, and shared/parameter_schema.json — keep all three in step.
*/
namespace ParamIDs
{
    inline constexpr auto wavetablePosition = "wavetable_position";
    inline constexpr auto filterCutoff      = "filter_cutoff";
    inline constexpr auto filterResonance   = "filter_resonance";
    inline constexpr auto ampAttack         = "amp_attack";
    inline constexpr auto ampDecay          = "amp_decay";
    inline constexpr auto ampSustain        = "amp_sustain";
    inline constexpr auto ampRelease        = "amp_release";
    inline constexpr auto outputGain        = "output_gain";   // "Master Level", a 0..1 level
}

//==============================================================================
/**
    The VST3 wrapper. Its only jobs are:

      * tell the host what the plugin is (a stereo instrument with MIDI in),
      * forward the host's knobs/faders into the SynthEngine once per block,
      * hand MIDI note events to the SynthEngine,
      * load factory waves and apply factory presets,
      * save and restore the knob positions and the selected wave.

    All of the actual sound generation lives in SynthEngine.cpp.
*/
class Koola10SynthAudioProcessor : public juce::AudioProcessor
{
public:
    Koola10SynthAudioProcessor();
    ~Koola10SynthAudioProcessor() override;

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
    double getTailLengthSeconds() const override { return 8.0; }

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
    SynthEngine& getEngine() noexcept { return engine; }

    //==============================================================================
    // Waves
    //==============================================================================

    /** Called by the editor's "Load .wav" button and by the browser's User list. */
    bool loadWavetableFromFile (const juce::File& file, bool rememberInUserList = true);

    /** Loads one of the 24 embedded factory waves. */
    bool loadFactoryWave (int index);
    bool loadFactoryWaveByName (const juce::String& name);

    /** Index into the factory list, or -1 when a user file is loaded. */
    int getCurrentFactoryWaveIndex() const noexcept { return currentFactoryWaveIndex; }

    const juce::File& getCurrentWavetableFile() const noexcept { return currentWavetableFile; }

    /** WAV files the user has loaded this session, for the browser's User group. */
    const juce::Array<juce::File>& getUserWaveFiles() const noexcept { return userWaveFiles; }

    //==============================================================================
    // Presets
    //==============================================================================

    /** Applies one of the built-in presets (wave + every knob). */
    void applyFactoryPreset (int index);

    int getCurrentPresetIndex() const noexcept { return currentPresetIndex; }
    juce::String getCurrentPresetName() const { return currentPresetName; }

    /** Writes the full APVTS state (plus the selected wave) to a .koola10preset file. */
    bool savePresetToFile (const juce::File& file);

    /** Restores a .koola10preset file saved by savePresetToFile(). */
    bool loadPresetFromFile (const juce::File& file);

    //==============================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    // Pushes the current knob positions into the engine. Called once per block.
    void pushParametersToEngine();

    void setParameterValue (const char* parameterID, float value);
    void applyStateTree (juce::ValueTree state);
    void rememberUserWave (const juce::File& file);

    static constexpr int maxUserWaves = 64;

    SynthEngine engine;
    juce::AudioProcessorValueTreeState parameters;

    // Wave selection
    juce::File currentWavetableFile;
    juce::String currentFactoryWaveName;
    int currentFactoryWaveIndex = -1;
    juce::Array<juce::File> userWaveFiles;

    // Preset selection
    juce::String currentPresetName;
    int currentPresetIndex = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Koola10SynthAudioProcessor)
};
