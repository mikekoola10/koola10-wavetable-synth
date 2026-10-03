#include "PluginProcessor.h"
#include "PluginEditor.h"

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
        juce::AudioParameterFloatAttributes().withLabel ("pos")
            .withStringFromValueFunction ([] (float value, int)
            {
                return juce::String (juce::roundToInt (value * 100.0f)) + " %";
            })));

    // --- STAGE 2: filter --------------------------------------------------
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::filterCutoff, 1 },
        "Filter Cutoff",
        makeFrequencyRange (20.0f, 20000.0f, 1000.0f),
        2000.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")
            .withStringFromValueFunction ([] (float value, int)
            {
                return value >= 1000.0f ? juce::String (value / 1000.0f, 2) + " kHz"
                                        : juce::String (value, 0) + " Hz";
            })));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::filterResonance, 1 },
        "Filter Resonance",
        juce::NormalisableRange<float> (0.1f, 10.0f, 0.001f),
        0.707f,
        juce::AudioParameterFloatAttributes().withLabel ("Q")
            .withStringFromValueFunction ([] (float value, int)
            {
                // 0.707 is "no resonance" in JUCE, so re-scale for the display.
                const float percent = juce::jlimit (0.0f, 1.0f, (value - 0.707f) / (10.0f - 0.707f));
                return juce::String (juce::roundToInt (percent * 100.0f)) + " %";
            })));

    // --- STAGE 3: amplitude envelope --------------------------------------
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::ampAttack, 1 },
        "Amp Attack",
        makeFrequencyRange (0.001f, 5.0f, 0.2f),
        0.010f,
        juce::AudioParameterFloatAttributes().withLabel ("s")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::ampDecay, 1 },
        "Amp Decay",
        makeFrequencyRange (0.001f, 5.0f, 0.3f),
        0.300f,
        juce::AudioParameterFloatAttributes().withLabel ("s")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::ampSustain, 1 },
        "Amp Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f),
        0.700f,
        juce::AudioParameterFloatAttributes().withLabel ("level")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::ampRelease, 1 },
        "Amp Release",
        makeFrequencyRange (0.001f, 10.0f, 0.5f),
        0.400f,
        juce::AudioParameterFloatAttributes().withLabel ("s")));

    // --- STAGE 4: output --------------------------------------------------
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::outputGain, 1 },
        "Master Level",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    return layout;
}

//==============================================================================
Koola10SynthAudioProcessor::Koola10SynthAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "KOOLA10_SYNTH_STATE", createParameterLayout())
{
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
    engine.setMasterGainDecibels (readParameter (parameters, ParamIDs::outputGain));
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
bool Koola10SynthAudioProcessor::loadWavetableFromFile (const juce::File& file)
{
    if (! engine.loadWavetableFile (file))
        return false;

    currentWavetableFile = file;
    return true;
}

//==============================================================================
void Koola10SynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();

    // Remember which .wav the user loaded so the next session finds it again.
    state.setProperty ("wavetableFile", currentWavetableFile.getFullPathName(), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void Koola10SynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml == nullptr || ! xml->hasTagName (parameters.state.getType()))
        return;

    auto state = juce::ValueTree::fromXml (*xml);

    const juce::String storedPath = state.getProperty ("wavetableFile", juce::String());
    state.removeProperty ("wavetableFile", nullptr);

    parameters.replaceState (state);

    if (storedPath.isNotEmpty())
    {
        const juce::File wavetableFile (storedPath);

        if (wavetableFile.existsAsFile())
            loadWavetableFromFile (wavetableFile);
    }
}

//==============================================================================
// This creates the plugin instance that the host loads.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Koola10SynthAudioProcessor();
}
