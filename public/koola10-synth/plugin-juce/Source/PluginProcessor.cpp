#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "FactoryWaves.h"
#include "Presets.h"

//==============================================================================
namespace
{
    /** Frequencies feel natural when the knob is spaced logarithmically, so we
        bias the middle of the travel towards 1 kHz. */
    juce::NormalisableRange<float> makeFrequencyRange (float minimum, float maximum, float centreHz)
    {
        juce::NormalisableRange<float> range (minimum, maximum);
        range.setSkewForCentre (centreHz);
        return range;
    }

    float readParameter (const juce::AudioProcessorValueTreeState& state, const char* id)
    {
        if (auto* value = state.getRawParameterValue (id))
            return value->load();

        return 0.0f;
    }

    /** Every knob readout in v2 is a percentage of the knob's travel. */
    juce::String percentString (float proportion)
    {
        return juce::String (juce::roundToInt (juce::jlimit (0.0f, 1.0f, proportion) * 100.0f)) + " %";
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout
Koola10SynthAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // --- STAGE 1: oscillator ---------------------------------------------
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::wavetablePosition, 1 },
        "Wavetable Position",
        juce::NormalisableRange<float> (0.0f, 1.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")
            .withStringFromValueFunction ([] (float value, int)
            {
                return percentString (value);
            })));

    // --- STAGE 2: filter --------------------------------------------------
    // Opens around 2.5 kHz on first launch, which is musically open rather than
    // near-closed. The readout is always in kHz.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::filterCutoff, 1 },
        "Filter Cutoff",
        makeFrequencyRange (20.0f, 20000.0f, 1000.0f),
        2500.0f,
        juce::AudioParameterFloatAttributes().withLabel ("kHz")
            .withStringFromValueFunction ([] (float value, int)
            {
                return juce::String (value / 1000.0f, 2) + " kHz";
            })));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::filterResonance, 1 },
        "Filter Resonance",
        juce::NormalisableRange<float> (0.1f, 10.0f, 0.001f),
        0.707f,
        juce::AudioParameterFloatAttributes().withLabel ("%")
            .withStringFromValueFunction ([] (float value, int)
            {
                // 0.707 is "no resonance" in JUCE, so re-scale for the display.
                const float amount = juce::jlimit (0.0f, 1.0f, (value - 0.707f) / (10.0f - 0.707f));
                return percentString (amount);
            })));

    // --- STAGE 3: amplitude envelope --------------------------------------
    auto attackRange  = makeFrequencyRange (0.001f, 5.0f, 0.2f);
    auto decayRange   = makeFrequencyRange (0.001f, 5.0f, 0.3f);
    auto releaseRange = makeFrequencyRange (0.001f, 10.0f, 0.5f);

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::ampAttack, 1 },
        "Amp Attack",
        attackRange,
        0.010f,
        juce::AudioParameterFloatAttributes().withLabel ("%")
            .withStringFromValueFunction ([attackRange] (float value, int)
            {
                return percentString (attackRange.convertTo0to1 (value));
            })));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::ampDecay, 1 },
        "Amp Decay",
        decayRange,
        0.300f,
        juce::AudioParameterFloatAttributes().withLabel ("%")
            .withStringFromValueFunction ([decayRange] (float value, int)
            {
                return percentString (decayRange.convertTo0to1 (value));
            })));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::ampSustain, 1 },
        "Amp Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f),
        0.700f,
        juce::AudioParameterFloatAttributes().withLabel ("%")
            .withStringFromValueFunction ([] (float value, int)
            {
                return percentString (value);
            })));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::ampRelease, 1 },
        "Amp Release",
        releaseRange,
        0.400f,
        juce::AudioParameterFloatAttributes().withLabel ("%")
            .withStringFromValueFunction ([releaseRange] (float value, int)
            {
                return percentString (releaseRange.convertTo0to1 (value));
            })));

    // --- STAGE 4: output --------------------------------------------------
    // Master Level is a linear 0..1 level so it reads naturally as a
    // percentage. 0.7 is audible straight away without touching a knob.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::outputGain, 1 },
        "Master Level",
        juce::NormalisableRange<float> (0.0f, 1.0f),
        0.700f,
        juce::AudioParameterFloatAttributes().withLabel ("%")
            .withStringFromValueFunction ([] (float value, int)
            {
                return percentString (value);
            })));

    return layout;
}

//==============================================================================
Koola10SynthAudioProcessor::Koola10SynthAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "KOOLA10_SYNTH_STATE", createParameterLayout())
{
    // Make a sound on first launch without touching a knob: load a bright
    // factory wave and leave the (already musical) parameter defaults alone.
    loadFactoryWaveByName ("Classic Saw");
}

Koola10SynthAudioProcessor::~Koola10SynthAudioProcessor() = default;

//==============================================================================
void Koola10SynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    pushParametersToEngine();
}

void Koola10SynthAudioProcessor::releaseResources()
{
    engine.reset();
}

bool Koola10SynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Instrument: no audio input, stereo output only.
    if (! layouts.getMainInputChannelSet().isDisabled())
        return false;

    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

//==============================================================================
void Koola10SynthAudioProcessor::pushParametersToEngine()
{
    engine.setWavetablePosition (readParameter (parameters, ParamIDs::wavetablePosition));
    engine.setFilterCutoff      (readParameter (parameters, ParamIDs::filterCutoff));
    engine.setFilterResonance   (readParameter (parameters, ParamIDs::filterResonance));
    engine.setEnvelope          (readParameter (parameters, ParamIDs::ampAttack),
                                 readParameter (parameters, ParamIDs::ampDecay),
                                 readParameter (parameters, ParamIDs::ampSustain),
                                 readParameter (parameters, ParamIDs::ampRelease));

    // Master Level is a linear level; the engine works in decibels.
    const float level = juce::jlimit (0.0f, 1.0f, readParameter (parameters, ParamIDs::outputGain));
    engine.setMasterGainDecibels (juce::Decibels::gainToDecibels (level, -60.0f));
}

void Koola10SynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    pushParametersToEngine();

    // Turn the host's note events into engine calls, in the order they arrived.
    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();

        if (message.isNoteOn())
            engine.noteOn (message.getNoteNumber(), message.getFloatVelocity());
        else if (message.isNoteOff())
            engine.noteOff();
        else if (message.isAllNotesOff() || message.isAllSoundOff())
            engine.allNotesOff();
    }

    midiMessages.clear();

    engine.process (buffer, 0, numSamples);
}

//==============================================================================
juce::AudioProcessorEditor* Koola10SynthAudioProcessor::createEditor()
{
    return new Koola10SynthAudioProcessorEditor (*this);
}

//==============================================================================
void Koola10SynthAudioProcessor::setParameterValue (const char* parameterID, float value)
{
    if (auto* parameter = parameters.getParameter (parameterID))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
}

void Koola10SynthAudioProcessor::rememberUserWave (const juce::File& file)
{
    const auto path = file.getFullPathName();

    for (const auto& existing : userWaveFiles)
        if (existing.getFullPathName() == path)
            return;

    if (userWaveFiles.size() >= maxUserWaves)
        userWaveFiles.remove (0);

    userWaveFiles.add (file);
}

//==============================================================================
bool Koola10SynthAudioProcessor::loadWavetableFromFile (const juce::File& file, bool rememberInUserList)
{
    if (! engine.loadWavetableFile (file))
        return false;

    currentWavetableFile = file;
    currentFactoryWaveName.clear();
    currentFactoryWaveIndex = -1;

    if (rememberInUserList)
        rememberUserWave (file);

    return true;
}

bool Koola10SynthAudioProcessor::loadFactoryWave (int index)
{
    if (! FactoryWaves::loadIntoEngine (engine, index))
        return false;

    currentFactoryWaveIndex = index;
    currentFactoryWaveName = FactoryWaves::getWave (index).name;
    currentWavetableFile = juce::File();

    return true;
}

bool Koola10SynthAudioProcessor::loadFactoryWaveByName (const juce::String& name)
{
    const int index = FactoryWaves::indexOfName (name);
    return index >= 0 && loadFactoryWave (index);
}

//==============================================================================
void Koola10SynthAudioProcessor::applyFactoryPreset (int index)
{
    if (! juce::isPositiveAndBelow (index, Presets::getNumFactoryPresets()))
        return;

    const auto& preset = Presets::getFactoryPreset (index);

    setParameterValue (ParamIDs::wavetablePosition, preset.wavetablePosition);
    setParameterValue (ParamIDs::filterCutoff,      preset.filterCutoff);
    setParameterValue (ParamIDs::filterResonance,   preset.filterResonance);
    setParameterValue (ParamIDs::ampAttack,         preset.attack);
    setParameterValue (ParamIDs::ampDecay,          preset.decay);
    setParameterValue (ParamIDs::ampSustain,        preset.sustain);
    setParameterValue (ParamIDs::ampRelease,        preset.release);
    setParameterValue (ParamIDs::outputGain,        preset.masterLevel);

    loadFactoryWaveByName (preset.waveName);

    currentPresetIndex = index;
    currentPresetName = preset.name;

    pushParametersToEngine();
}

//==============================================================================
void Koola10SynthAudioProcessor::applyStateTree (juce::ValueTree state)
{
    const juce::String factoryWave = state.getProperty ("factoryWave", juce::String());
    const juce::String filePath    = state.getProperty ("wavetableFile", juce::String());
    const juce::String presetName  = state.getProperty ("presetName", juce::String());

    state.removeProperty ("factoryWave", nullptr);
    state.removeProperty ("wavetableFile", nullptr);
    state.removeProperty ("presetName", nullptr);

    parameters.replaceState (state);

    currentPresetName = presetName;
    currentPresetIndex = Presets::indexOfName (presetName);

    if (factoryWave.isNotEmpty())
    {
        loadFactoryWaveByName (factoryWave);
    }
    else if (filePath.isNotEmpty())
    {
        const juce::File file (filePath);

        if (file.existsAsFile())
            loadWavetableFromFile (file, false);
    }

    pushParametersToEngine();
}

//==============================================================================
void Koola10SynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();

    // Remember the selected wave so the next session finds it again.
    state.setProperty ("wavetableFile", currentWavetableFile.getFullPathName(), nullptr);
    state.setProperty ("factoryWave", currentFactoryWaveName, nullptr);
    state.setProperty ("presetName", currentPresetName, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void Koola10SynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml == nullptr || ! xml->hasTagName (parameters.state.getType()))
        return;

    applyStateTree (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
bool Koola10SynthAudioProcessor::savePresetToFile (const juce::File& file)
{
    auto state = parameters.copyState();
    state.setProperty ("wavetableFile", currentWavetableFile.getFullPathName(), nullptr);
    state.setProperty ("factoryWave", currentFactoryWaveName, nullptr);
    state.setProperty ("presetName", currentPresetName.isNotEmpty()
                                        ? currentPresetName
                                        : file.getFileNameWithoutExtension(), nullptr);

    juce::ValueTree root ("KOOLA10_PRESET");
    root.setProperty ("version", 1, nullptr);
    root.appendChild (state, nullptr);

    if (auto xml = root.createXml())
        return xml->writeTo (file);

    return false;
}

bool Koola10SynthAudioProcessor::loadPresetFromFile (const juce::File& file)
{
    std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse (file));

    if (xml == nullptr)
        return false;

    const auto root = juce::ValueTree::fromXml (*xml);

    // Accept both a wrapped .koola10preset file and a bare APVTS state tree.
    juce::ValueTree state = root.hasType (parameters.state.getType())
                              ? root
                              : root.getChildWithName (parameters.state.getType());

    if (! state.isValid())
        return false;

    applyStateTree (state);
    return true;
}

//==============================================================================
// This creates the plugin instance that the host loads.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Koola10SynthAudioProcessor();
}
