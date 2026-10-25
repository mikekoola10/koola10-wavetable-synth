#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <array>

//==============================================================================
namespace
{
    /** Every knob readout on this plugin is a percentage, except the cutoff. */
    juce::String percentString (float proportion)
    {
        return juce::String (juce::roundToInt (juce::jlimit (0.0f, 1.0f, proportion) * 100.0f)) + " %";
    }

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

    /** Full parameter names, for the host's automation list. */
    constexpr const char* padParameterNames[DrumPads::numPads][3] =
    {
        { "Kick Tune",       "Kick Decay",       "Kick Level" },
        { "Snare Tune",      "Snare Decay",      "Snare Level" },
        { "Closed Hat Tune", "Closed Hat Decay", "Closed Hat Level" },
        { "Open Hat Tune",   "Open Hat Decay",   "Open Hat Level" },
        { "Clap Tune",       "Clap Decay",       "Clap Level" },
        { "Rim Tune",        "Rim Decay",        "Rim Level" },
        { "Tom Tune",        "Tom Decay",        "Tom Level" },
        { "Crash Tune",      "Crash Decay",      "Crash Level" }
    };

    /** Defaults: a balanced kit that is loud enough to hear on first launch. */
    constexpr float padDefaults[DrumPads::numPads][3] =
    {
        { 0.50f, 0.55f, 0.85f },   // kick
        { 0.50f, 0.45f, 0.80f },   // snare
        { 0.50f, 0.40f, 0.65f },   // closed hat
        { 0.50f, 0.45f, 0.60f },   // open hat
        { 0.50f, 0.45f, 0.70f },   // clap
        { 0.50f, 0.40f, 0.60f },   // rim
        { 0.50f, 0.45f, 0.70f },   // tom
        { 0.50f, 0.40f, 0.55f }    // crash
    };
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
Koola10DrumMachineAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // --- The 24 pad knobs: Tune, Decay and Level on every pad ---------------
    for (int pad = 0; pad < DrumPads::numPads; ++pad)
    {
        for (int kind = 0; kind < 3; ++kind)
        {
            layout.add (std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID { ParamIDs::padIds[pad][kind], 1 },
                padParameterNames[pad][kind],
                juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
                padDefaults[pad][kind],
                juce::AudioParameterFloatAttributes().withLabel ("%")
                    .withStringFromValueFunction ([] (float value, int)
                    {
                        return percentString (value);
                    })));
        }
    }

    // --- Shared filter -----------------------------------------------------
    // Open by default (12 kHz), so the kit sounds like a kit rather than a
    // muffled loop until the user reaches for the knob. Always shown in kHz.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::filterCutoff, 1 },
        "Filter Cutoff",
        makeFrequencyRange (20.0f, 20000.0f, 1000.0f),
        12000.0f,
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
                const float amount = juce::jlimit (0.0f, 1.0f, (value - 0.707f) / (10.0f - 0.707f));
                return percentString (amount);
            })));

    // --- Feel --------------------------------------------------------------
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::swing, 1 },
        "Swing",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")
            .withStringFromValueFunction ([] (float value, int)
            {
                return percentString (value);
            })));

    // --- Output ------------------------------------------------------------
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::masterLevel, 1 },
        "Master Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        DrumEngine::defaultMasterLevel,
        juce::AudioParameterFloatAttributes().withLabel ("%")
            .withStringFromValueFunction ([] (float value, int)
            {
                return percentString (value);
            })));

    return layout;
}

//==============================================================================
Koola10DrumMachineAudioProcessor::Koola10DrumMachineAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "KOOLA10_DRUMS_STATE", createParameterLayout())
{
    // The shipped pattern means the plugin makes a beat on first launch without
    // the user drawing a single step, exactly like the synth starts on Classic Saw.
    sequencer.loadFactoryPattern();
}

Koola10DrumMachineAudioProcessor::~Koola10DrumMachineAudioProcessor() = default;

//==============================================================================
void Koola10DrumMachineAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    sequencer.prepare (sampleRate);

    pushParametersToEngine();
}

void Koola10DrumMachineAudioProcessor::releaseResources()
{
    engine.reset();
    sequencer.reset();
}

bool Koola10DrumMachineAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Instrument: no audio input, stereo output only.
    if (! layouts.getMainInputChannelSet().isDisabled())
        return false;

    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

//==============================================================================
void Koola10DrumMachineAudioProcessor::pushParametersToEngine()
{
    for (int pad = 0; pad < DrumPads::numPads; ++pad)
    {
        engine.setPadTune  (pad, readParameter (parameters, ParamIDs::padIds[pad][0]));
        engine.setPadDecay (pad, readParameter (parameters, ParamIDs::padIds[pad][1]));
        engine.setPadLevel (pad, readParameter (parameters, ParamIDs::padIds[pad][2]));
    }

    engine.setFilterCutoff    (readParameter (parameters, ParamIDs::filterCutoff));
    engine.setFilterResonance (readParameter (parameters, ParamIDs::filterResonance));
    engine.setMasterLevel     (readParameter (parameters, ParamIDs::masterLevel));

    sequencer.setSwing (readParameter (parameters, ParamIDs::swing));
}

void Koola10DrumMachineAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                     juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();

    buffer.clear();

    if (numSamples <= 0)
    {
        midiMessages.clear();
        return;
    }

    pushParametersToEngine();

    std::array<PendingHit, 128> hits {};
    int numHits = 0;

    auto addHit = [&] (int pad, int sampleOffset, float velocity)
    {
        if (numHits < (int) hits.size())
            hits[(size_t) numHits++] = PendingHit { juce::jlimit (0, numSamples - 1, sampleOffset),
                                                    pad,
                                                    juce::jlimit (0.0f, 1.0f, velocity) };
    };

    // MIDI pads (FL Studio's piano roll and any drum pad controller).
    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();

        if (message.isNoteOn())
        {
            const int pad = DrumPads::padForMidiNote (message.getNoteNumber());

            if (pad >= 0)
                addHit (pad, metadata.samplePosition, message.getFloatVelocity());
        }
    }

    // The step grid, clocked from the host's transport.
    sequencer.process (getPlayHead(), numSamples);

    for (int i = 0; i < sequencer.getNumEvents(); ++i)
    {
        const auto& event = sequencer.getEvent (i);
        addHit (event.pad, event.sampleOffset, 1.0f);
    }

    // Hits are placed where they belong, not at the start of the block, so a
    // step that falls mid-block is sample-accurate against the rest of the mix.
    std::sort (hits.begin(), hits.begin() + numHits,
               [] (const PendingHit& a, const PendingHit& b) { return a.sampleOffset < b.sampleOffset; });

    int rendered = 0;

    for (int i = 0; i < numHits; ++i)
    {
        const auto& hit = hits[(size_t) i];

        if (hit.sampleOffset > rendered)
        {
            engine.process (buffer, rendered, hit.sampleOffset - rendered);
            rendered = hit.sampleOffset;
        }

        engine.triggerPad (hit.pad, hit.velocity);
    }

    if (rendered < numSamples)
        engine.process (buffer, rendered, numSamples - rendered);

    midiMessages.clear();
}

//==============================================================================
juce::AudioProcessorEditor* Koola10DrumMachineAudioProcessor::createEditor()
{
    return new Koola10DrumMachineAudioProcessorEditor (*this);
}

//==============================================================================
void Koola10DrumMachineAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();

    // The step grid is not automatable, so it travels as a plain string in the
    // same state tree: 128 characters of '0' and '1'.
    state.setProperty ("pattern", sequencer.toPatternString(), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void Koola10DrumMachineAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);

    if (xml == nullptr)
        return;

    const auto state = juce::ValueTree::fromXml (*xml);

    if (! state.isValid())
        return;

    if (state.hasProperty ("pattern"))
        sequencer.setPatternFromString (state.getProperty ("pattern").toString());

    if (state.hasType (parameters.state.getType()))
        parameters.replaceState (state);
}
