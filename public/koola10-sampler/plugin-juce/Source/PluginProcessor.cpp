#include "PluginProcessor.h"
#include "PluginEditor.h"

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

    /** The six knobs on every slot, in panel order. */
    constexpr const char* slotKnobNames[6] =
    {
        "Tune", "Level", "Attack", "Release", "Start", "End"
    };

    /** Defaults: a slot that plays at its recorded pitch, at a useful level,
        with just enough attack and release to avoid clicks. */
    constexpr float slotKnobDefaults[6] =
    {
        0.50f,   // Tune    — the sample at the pitch it was recorded at
        0.80f,   // Level
        0.02f,   // Attack  — about 2 ms
        0.25f,   // Release — about 190 ms
        0.00f,   // Start   — from the first sample
        1.00f    // End     — to the last sample
    };
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
Koola10SamplerAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // --- Six knobs and one mode on each of the eight slots -----------------
    for (int slot = 0; slot < SamplerSlots::numSlots; ++slot)
    {
        const auto prefix = juce::String ("Slot ") + juce::String (slot + 1) + " ";

        for (int kind = 0; kind < 6; ++kind)
        {
            layout.add (std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID { ParamIDs::slotIds[slot][kind], 1 },
                prefix + slotKnobNames[kind],
                juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
                slotKnobDefaults[kind],
                juce::AudioParameterFloatAttributes().withLabel ("%")
                    .withStringFromValueFunction ([] (float value, int)
                    {
                        return percentString (value);
                    })));
        }

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { ParamIDs::slotModes[slot], 1 },
            prefix + "Play Mode",
            juce::StringArray { "One-Shot", "Loop" },
            0));
    }

    // --- Shared filter ------------------------------------------------------
    // Open by default (12 kHz), so a slot sounds like itself until the user
    // reaches for the knob. Always shown in kHz, like the synth and drums.
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

    // --- Output -------------------------------------------------------------
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::masterLevel, 1 },
        "Master Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        SamplerEngine::defaultMasterLevel,
        juce::AudioParameterFloatAttributes().withLabel ("%")
            .withStringFromValueFunction ([] (float value, int)
            {
                return percentString (value);
            })));

    return layout;
}

//==============================================================================
Koola10SamplerAudioProcessor::Koola10SamplerAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "KOOLA10_SAMPLER_STATE", createParameterLayout())
{
    // v1 plays the two formats decided in the brief: WAV and MP3. The MP3
    // reader is compiled out of JUCE by default; CMakeLists.txt switches it on
    // (JUCE_USE_MP3AUDIOFORMAT), see README section 2.
    formats.registerFormat (new juce::WAVAudioFormat(), true);

   #if JUCE_USE_MP3AUDIOFORMAT
    formats.registerFormat (new juce::MP3AudioFormat(), true);
   #endif

    for (auto& info : slotInfos)
        info = SlotInfo();

    // Replaced samples are freed here, once nothing is playing them.
    startTimerHz (2);
}

Koola10SamplerAudioProcessor::~Koola10SamplerAudioProcessor()
{
    stopTimer();
}

//==============================================================================
void Koola10SamplerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);

    pushParametersToEngine();
}

void Koola10SamplerAudioProcessor::releaseResources()
{
    engine.reset();
}

bool Koola10SamplerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Instrument: no audio input, stereo output only.
    if (! layouts.getMainInputChannelSet().isDisabled())
        return false;

    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

//==============================================================================
void Koola10SamplerAudioProcessor::pushParametersToEngine()
{
    for (int slot = 0; slot < SamplerSlots::numSlots; ++slot)
    {
        engine.setSlotTune    (slot, readParameter (parameters, ParamIDs::slotIds[slot][0]));
        engine.setSlotLevel   (slot, readParameter (parameters, ParamIDs::slotIds[slot][1]));
        engine.setSlotAttack  (slot, readParameter (parameters, ParamIDs::slotIds[slot][2]));
        engine.setSlotRelease (slot, readParameter (parameters, ParamIDs::slotIds[slot][3]));
        engine.setSlotStart   (slot, readParameter (parameters, ParamIDs::slotIds[slot][4]));
        engine.setSlotEnd     (slot, readParameter (parameters, ParamIDs::slotIds[slot][5]));

        engine.setSlotLooping (slot, readParameter (parameters, ParamIDs::slotModes[slot]) > 0.5f);
    }

    engine.setFilterCutoff    (readParameter (parameters, ParamIDs::filterCutoff));
    engine.setFilterResonance (readParameter (parameters, ParamIDs::filterResonance));
    engine.setMasterLevel     (readParameter (parameters, ParamIDs::masterLevel));
}

void Koola10SamplerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
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

    engine.beginBlock (numSamples);

    // Notes are started at the sample they arrived on. A chord struck halfway
    // through a block starts halfway through the block, not at its edge, which
    // is what makes the piano roll sit tight against the drums.
    int rendered = 0;

    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();

        if (! message.isNoteOn() && ! message.isNoteOff())
            continue;

        const int offset = juce::jlimit (0, numSamples - 1, metadata.samplePosition);

        if (offset > rendered)
        {
            engine.render (buffer, rendered, offset - rendered);
            rendered = offset;
        }

        if (message.isNoteOn())
            engine.noteOn (message.getNoteNumber(), message.getFloatVelocity());
        else
            engine.noteOff (message.getNoteNumber());
    }

    if (rendered < numSamples)
        engine.render (buffer, rendered, numSamples - rendered);

    midiMessages.clear();
}

//==============================================================================
void Koola10SamplerAudioProcessor::publishSlot (int slot,
                                                std::shared_ptr<const LoadedSample> sample,
                                                const juce::String& name,
                                                const juce::String& path,
                                                bool missing)
{
    if (slot < 0 || slot >= SamplerSlots::numSlots)
        return;

    const juce::ScopedLock sl (slotLock);

    slotInfos[(size_t) slot] = SlotInfo { name, path, sample != nullptr, missing };
    engine.setSlotSample (slot, std::move (sample));
}

Koola10SamplerAudioProcessor::SlotInfo
Koola10SamplerAudioProcessor::getSlotInfo (int slot) const
{
    if (slot < 0 || slot >= SamplerSlots::numSlots)
        return {};

    const juce::ScopedLock sl (slotLock);

    return slotInfos[(size_t) slot];
}

std::shared_ptr<const LoadedSample>
Koola10SamplerAudioProcessor::getSlotSample (int slot) const
{
    const juce::ScopedLock sl (slotLock);

    return engine.getSlotSample (slot);
}

bool Koola10SamplerAudioProcessor::loadSlot (int slot, const juce::File& file)
{
    if (slot < 0 || slot >= SamplerSlots::numSlots)
        return false;

    const auto fallbackName = file.getFileName();
    const auto path = file.getFullPathName();

    // A path that is not there is not an error the user has to fix: the slot
    // keeps showing the name it was saved with and stays silent.
    if (! file.existsAsFile())
    {
        publishSlot (slot, nullptr, fallbackName, path, true);
        return false;
    }

    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));

    if (reader == nullptr)
    {
        publishSlot (slot, nullptr, fallbackName, path, true);
        return false;
    }

    const double sourceRate = reader->sampleRate > 0.0 ? reader->sampleRate : 44100.0;

    // Two minutes per slot is plenty for a one-shot or a loop, and it stops a
    // stray full-length track from eating a gigabyte of RAM across eight slots.
    const juce::int64 maximumFrames = (juce::int64) (sourceRate * 120.0);
    const int numFrames = (int) juce::jlimit<juce::int64> (0, maximumFrames, reader->lengthInSamples);

    if (numFrames < 2)
    {
        publishSlot (slot, nullptr, fallbackName, path, true);
        return false;
    }

    auto sample = std::make_shared<LoadedSample>();
    sample->sourceSampleRate = sourceRate;
    sample->name = fallbackName;
    sample->path = path;

    const int numChannels = juce::jlimit (1, 2, juce::jmax (1, (int) reader->numChannels));

    sample->buffer.setSize (numChannels, numFrames);
    sample->buffer.clear();

    const bool read = reader->read (&sample->buffer, 0, numFrames, 0, true, numChannels > 1);

    if (! read)
    {
        publishSlot (slot, nullptr, fallbackName, path, true);
        return false;
    }

    publishSlot (slot, sample, sample->name, sample->path, false);
    return true;
}

bool Koola10SamplerAudioProcessor::loadSlotFromPath (int slot, const juce::String& path,
                                                     const juce::String& knownName)
{
    if (path.isEmpty())
    {
        clearSlot (slot);
        return false;
    }

    const juce::File file (path);

    if (! file.existsAsFile())
    {
        publishSlot (slot, nullptr, knownName.isNotEmpty() ? knownName : file.getFileName(), path, true);
        return false;
    }

    return loadSlot (slot, file);
}

void Koola10SamplerAudioProcessor::clearSlot (int slot)
{
    publishSlot (slot, nullptr, {}, {}, false);
}

void Koola10SamplerAudioProcessor::setPlaySlot (int slot)
{
    engine.setPlaySlot (slot);
}

void Koola10SamplerAudioProcessor::timerCallback()
{
    engine.releaseRetiredSamples();
}

//==============================================================================
juce::AudioProcessorEditor* Koola10SamplerAudioProcessor::createEditor()
{
    return new Koola10SamplerAudioProcessorEditor (*this);
}

//==============================================================================
void Koola10SamplerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();

    // The slot paths are not automatable, so they travel as plain strings in
    // the same state tree: "slot3_path" etc.
    {
        const juce::ScopedLock sl (slotLock);

        for (int slot = 0; slot < SamplerSlots::numSlots; ++slot)
        {
            const auto prefix = juce::String ("slot") + juce::String (slot + 1) + "_";
            const auto& info = slotInfos[(size_t) slot];

            state.setProperty (prefix + "path", info.path, nullptr);
            state.setProperty (prefix + "name", info.name, nullptr);
        }
    }

    state.setProperty ("play_slot", engine.getPlaySlot(), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void Koola10SamplerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);

    if (xml == nullptr)
        return;

    const auto state = juce::ValueTree::fromXml (*xml);

    if (! state.isValid())
        return;

    if (state.hasType (parameters.state.getType()))
        parameters.replaceState (state);

    if (state.hasProperty ("play_slot"))
        engine.setPlaySlot ((int) state.getProperty ("play_slot"));

    for (int slot = 0; slot < SamplerSlots::numSlots; ++slot)
    {
        const auto prefix = juce::String ("slot") + juce::String (slot + 1) + "_";
        const auto path = state.getProperty (prefix + "path").toString();
        const auto name = state.getProperty (prefix + "name").toString();

        loadSlotFromPath (slot, path, name);
    }

    pushParametersToEngine();
}
