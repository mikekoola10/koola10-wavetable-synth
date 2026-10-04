#include "SynthEngine.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
    // A ramp time of 20 ms is long enough to hide knob jumps, short enough that
    // the control still feels immediate.
    constexpr double smoothingSeconds = 0.02;

    constexpr int defaultFrameLength = SynthEngine::frameSize;
}

//==============================================================================
SynthEngine::SynthEngine()
{
    buildDefaultTable();

    positionSmoother.setCurrentAndTargetValue (0.0f);
    cutoffSmoother.setCurrentAndTargetValue (2500.0f);
    gainSmoother.setCurrentAndTargetValue (1.0f);
}

void SynthEngine::buildDefaultTable()
{
    // One single-cycle sine frame, so the plugin makes a sound before the user
    // has loaded any .wav. This is the "safety net" wavetable.
    table.assign (static_cast<size_t> (defaultFrameLength), 0.0f);

    for (int i = 0; i < defaultFrameLength; ++i)
        table[static_cast<size_t> (i)] = static_cast<float> (
            std::sin (juce::MathConstants<double>::twoPi
                      * static_cast<double> (i) / static_cast<double> (defaultFrameLength)));

    frameLength = defaultFrameLength;
    numFrames = 1;
    wavetableName = "Sine (built in)";
}

//==============================================================================
void SynthEngine::prepare (double newSampleRate, int maximumBlockSize)
{
    sampleRate = newSampleRate;

    positionSmoother.reset (sampleRate, smoothingSeconds);
    cutoffSmoother.reset (sampleRate, smoothingSeconds);
    gainSmoother.reset (sampleRate, smoothingSeconds);

    // --- STAGE 2: filter -------------------------------------------------
    juce::dsp::ProcessSpec spec { sampleRate,
                                  static_cast<juce::uint32> (juce::jmax (1, maximumBlockSize)),
                                  2 };
    lowPassFilter.prepare (spec);
    lowPassFilter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    lowPassFilter.setResonance (resonanceTarget.load());
    lowPassFilter.setCutoffFrequency (juce::jlimit (20.0f,
                                                    static_cast<float> (sampleRate * 0.45),
                                                    cutoffSmootherTarget.load()));

    // --- STAGE 3: envelope -----------------------------------------------
    adsr.setSampleRate (sampleRate);
    adsr.setParameters (adsrParameters);

    // --- Modulation sources ----------------------------------------------
    // Nothing is routed in v2, but a v3 LFO/sequencer would be prepared here.
    if (auto* source = positionModSource.load())  source->prepare (sampleRate, maximumBlockSize);
    if (auto* source = pitchModSource.load())     source->prepare (sampleRate, maximumBlockSize);
    if (auto* source = amplitudeModSource.load()) source->prepare (sampleRate, maximumBlockSize);

    reset();
}

void SynthEngine::reset()
{
    phase = 0.0;
    phaseIncrement = 0.0;
    adsr.reset();
    lowPassFilter.reset();
    positionSmoother.setCurrentAndTargetValue (positionSmootherTarget.load());
    cutoffSmoother.setCurrentAndTargetValue (cutoffSmootherTarget.load());
    gainSmoother.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (gainDecibelsTarget.load()));

    if (auto* source = positionModSource.load())  source->reset();
    if (auto* source = pitchModSource.load())     source->reset();
    if (auto* source = amplitudeModSource.load()) source->reset();
}

//==============================================================================
// Wavetable loading
//==============================================================================
bool SynthEngine::loadWavetableFile (const juce::File& file)
{
    if (! file.existsAsFile())
        return false;

    juce::AudioFormatManager formats;
    formats.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));

    if (reader == nullptr)
        return false;

    return installFromReader (*reader, file.getFileNameWithoutExtension());
}

bool SynthEngine::loadWavetableFromMemory (const void* data, size_t numBytes,
                                           const juce::String& displayName)
{
    if (data == nullptr || numBytes == 0)
        return false;

    // The factory waves are plain WAV files, so we can decode them straight
    // from the static bytes that juce_add_binary_data baked into the binary.
    juce::WavAudioFormat wavFormat;

    // `stream` does not own `data` (the BinaryData bytes outlive this call), and
    // `reader` is declared after it, so `reader` is destroyed before `stream` at
    // the end of this function. `createReaderFor` is passed ownsStream=false, so
    // the reader only borrows the stream - the two lifetimes are therefore
    // consistent and no reader ever outlives its stream, in Debug or Release.
    juce::MemoryInputStream stream (data, numBytes, false);

    std::unique_ptr<juce::AudioFormatReader> reader (wavFormat.createReaderFor (&stream, false));

    if (reader == nullptr)
        return false;

    // installFromReader reads the whole file before returning, so it never
    // retains a reference to the stream or the reader.
    return installFromReader (*reader, displayName);
}

bool SynthEngine::installFromReader (juce::AudioFormatReader& reader, const juce::String& displayName)
{
    if (reader.lengthInSamples < 64)
        return false;

    const int totalSamples = static_cast<int> (
        juce::jmin (reader.lengthInSamples, static_cast<juce::int64> (maxInputSamples)));
    const int numChannels = juce::jmax (1, static_cast<int> (reader.numChannels));

    juce::AudioBuffer<float> fileBuffer (numChannels, totalSamples);
    reader.read (&fileBuffer, 0, totalSamples, 0, true, true);

    // Collapse to mono: a wavetable is a shape, not a stereo image.
    std::vector<float> mono (static_cast<size_t> (totalSamples), 0.0f);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto* source = fileBuffer.getReadPointer (ch);

        for (int i = 0; i < totalSamples; ++i)
            mono[static_cast<size_t> (i)] += source[i];
    }

    const float channelScale = 1.0f / static_cast<float> (numChannels);

    for (auto& sample : mono)
        sample *= channelScale;

    return installFromMono (std::move (mono), displayName);
}

bool SynthEngine::installFromMono (std::vector<float> mono, const juce::String& displayName)
{
    if (mono.size() < 64)
        return false;

    const int totalSamples = static_cast<int> (mono.size());

    int newFrameLength = 0;
    int newNumFrames = 0;
    std::vector<float> newTable;

    if (totalSamples <= frameSize)
    {
        // Short file: one whole frame, exactly like v1.
        newFrameLength = totalSamples;
        newNumFrames = 1;
        newTable = std::move (mono);
    }
    else
    {
        // Slice into fixed 2048-sample frames; the final short frame is
        // zero-padded so every frame has the same length.
        newFrameLength = frameSize;
        newNumFrames = juce::jmin (maxFrames, (totalSamples + frameSize - 1) / frameSize);

        const size_t needed = static_cast<size_t> (newNumFrames)
                            * static_cast<size_t> (newFrameLength);
        newTable.assign (needed, 0.0f);

        const size_t samplesToCopy = juce::jmin (needed, mono.size());
        std::copy (mono.begin(), mono.begin() + static_cast<std::ptrdiff_t> (samplesToCopy),
                   newTable.begin());
    }

    {
        const juce::SpinLock::ScopedLockType lock (tableLock);
        table = std::move (newTable);
        frameLength = newFrameLength;
        numFrames = newNumFrames;
        wavetableName = displayName;
    }

    return true;
}

juce::String SynthEngine::getWavetableName() const
{
    const juce::SpinLock::ScopedLockType lock (tableLock);
    return wavetableName;
}

int SynthEngine::getNumFrames() const
{
    const juce::SpinLock::ScopedLockType lock (tableLock);
    return numFrames;
}

int SynthEngine::getFrameLength() const
{
    const juce::SpinLock::ScopedLockType lock (tableLock);
    return frameLength;
}

void SynthEngine::fillDisplayPoints (float* dest, int numPoints) const
{
    const juce::SpinLock::ScopedLockType lock (tableLock);

    if (numFrames <= 0 || frameLength <= 0 || numPoints <= 0)
        return;

    const float position = juce::jlimit (0.0f, 1.0f, positionSmoother.getCurrentValue());
    const float framePosition = position * static_cast<float> (juce::jmax (0, numFrames - 1));
    const int frameA = juce::jlimit (0, numFrames - 1, static_cast<int> (framePosition));
    const int frameB = juce::jmin (frameA + 1, numFrames - 1);
    const float morph = framePosition - static_cast<float> (frameA);

    // Draw the same post-crossfade frame the oscillator is playing.
    for (int i = 0; i < numPoints; ++i)
    {
        const double phaseForPoint = static_cast<double> (i) / static_cast<double> (numPoints);
        const float sampleA = sampleAt (frameA, phaseForPoint);
        const float sampleB = sampleAt (frameB, phaseForPoint);
        dest[i] = sampleA + (sampleB - sampleA) * morph;
    }
}

//==============================================================================
// STAGE 1 helper — read one sample of one frame, with linear interpolation
// between the two nearest stored samples (this is what stops the
// high frequencies from sounding gritty).
float SynthEngine::sampleAt (int frame, double normalisedPhase) const noexcept
{
    if (numFrames <= 0 || frameLength <= 0)
        return 0.0f;

    const double exactIndex = normalisedPhase * static_cast<double> (frameLength);
    const int index0 = static_cast<int> (exactIndex) % frameLength;
    const int index1 = (index0 + 1) % frameLength;
    const float fraction = static_cast<float> (exactIndex - std::floor (exactIndex));

    const auto frameOffset = static_cast<size_t> (frame) * static_cast<size_t> (frameLength);
    const float a = table[frameOffset + static_cast<size_t> (index0)];
    const float b = table[frameOffset + static_cast<size_t> (index1)];

    return a + (b - a) * fraction;
}

//==============================================================================
void SynthEngine::setWavetablePosition (float newPosition)
{
    positionSmootherTarget.store (juce::jlimit (0.0f, 1.0f, newPosition));
}

void SynthEngine::setFilterCutoff (float cutoffHz)
{
    const float highest = static_cast<float> (sampleRate * 0.45);
    cutoffSmootherTarget.store (juce::jlimit (20.0f, highest, cutoffHz));
}

void SynthEngine::setFilterResonance (float resonance)
{
    resonanceTarget.store (juce::jlimit (0.1f, 10.0f, resonance));
}

void SynthEngine::setEnvelope (float attackSeconds, float decaySeconds,
                               float sustainLevel, float releaseSeconds)
{
    pendingAttack.store  (juce::jmax (0.001f, attackSeconds));
    pendingDecay.store   (juce::jmax (0.001f, decaySeconds));
    pendingSustain.store (juce::jlimit (0.0f, 1.0f, sustainLevel));
    pendingRelease.store (juce::jmax (0.001f, releaseSeconds));
    envelopeNeedsUpdate.store (true);
}

void SynthEngine::setMasterGainDecibels (float decibels)
{
    gainDecibelsTarget.store (juce::jlimit (-60.0f, 12.0f, decibels));
}

void SynthEngine::setModulation (ModTarget target, ModSource* source, float depth)
{
    const float clampedDepth = juce::jlimit (0.0f, 1.0f, depth);

    switch (target)
    {
        case ModTarget::wavetablePosition:
            positionModDepth.store (clampedDepth);
            positionModSource.store (source);
            break;

        case ModTarget::pitch:
            pitchModDepth.store (clampedDepth);
            pitchModSource.store (source);
            break;

        case ModTarget::amplitude:
            amplitudeModDepth.store (clampedDepth);
            amplitudeModSource.store (source);
            break;
    }
}

//==============================================================================
void SynthEngine::noteOn (int midiNoteNumber, float velocity)
{
    velocityGain = juce::jlimit (0.0f, 1.0f, velocity);

    const double frequency = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
    phaseIncrement = frequency / sampleRate;

    // Start each new note from the top of the wave so short notes are predictable.
    phase = 0.0;
    adsr.noteOn();

    if (auto* source = positionModSource.load())  source->noteOn();
    if (auto* source = pitchModSource.load())     source->noteOn();
    if (auto* source = amplitudeModSource.load()) source->noteOn();
}

void SynthEngine::noteOff()
{
    adsr.noteOff();

    if (auto* source = positionModSource.load())  source->noteOff();
    if (auto* source = pitchModSource.load())     source->noteOff();
    if (auto* source = amplitudeModSource.load()) source->noteOff();
}

void SynthEngine::allNotesOff()
{
    adsr.reset();
    phase = 0.0;
}

void SynthEngine::clearBlock (juce::AudioBuffer<float>& buffer, int startSample, int numSamples) const
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, startSample, numSamples);
}

//==============================================================================
void SynthEngine::process (juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    juce::ScopedNoDenormals noDenormals;

    if (numSamples <= 0 || buffer.getNumChannels() == 0)
        return;

    // Apply any envelope changes that arrived from the GUI since the last block.
    if (envelopeNeedsUpdate.exchange (false))
    {
        adsrParameters = juce::ADSR::Parameters { pendingAttack.load(),
                                                  pendingDecay.load(),
                                                  pendingSustain.load(),
                                                  pendingRelease.load() };
        adsr.setParameters (adsrParameters);
    }

    // --- Take the wavetable lock ------------------------------------------
    // If the user is mid-way through loading a new .wav we render silence for
    // this one block instead of reading half-written memory.
    const juce::SpinLock::ScopedTryLockType tableLockTry (tableLock);

    if (! tableLockTry.isLocked())
    {
        clearBlock (buffer, startSample, numSamples);
        return;
    }

    // ======================================================================
    // STAGE 1 — Wavetable oscillator
    // Turn the position knob (0..1) into a place in the table, then read the
    // two neighbouring frames and crossfade between them.
    // ======================================================================
    positionSmoother.setTargetValue (positionSmootherTarget.load());

    const bool voiceIsRunning = (numFrames > 0) && adsr.isActive();

    // Modulation routings are lock-free; read them once for the whole block.
    // v2 routes nothing, so these are all inactive and cost only a load.
    auto* positionMod = positionModSource.load();
    const float positionDepth = positionModDepth.load();
    const bool positionModActive = positionMod != nullptr && positionDepth != 0.0f
                                 && positionMod->isActive();

    auto* pitchMod = pitchModSource.load();
    const float pitchDepth = pitchModDepth.load();
    const bool pitchModActive = pitchMod != nullptr && pitchDepth != 0.0f
                              && pitchMod->isActive();

    auto* amplitudeMod = amplitudeModSource.load();
    const float amplitudeDepth = amplitudeModDepth.load();
    const bool amplitudeModActive = amplitudeMod != nullptr && amplitudeDepth != 0.0f
                                  && amplitudeMod->isActive();

    if (voiceIsRunning)
    {
        auto* left  = buffer.getWritePointer (0, startSample);
        auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1, startSample) : nullptr;

        for (int i = 0; i < numSamples; ++i)
        {
            float position = positionSmoother.getNextValue();

            if (positionModActive)
                position = juce::jlimit (0.0f, 1.0f,
                                         position + positionDepth * positionMod->getValue (i));

            const float framePosition = position * static_cast<float> (juce::jmax (0, numFrames - 1));
            const int frameA = juce::jlimit (0, numFrames - 1, static_cast<int> (framePosition));
            const int frameB = juce::jmin (frameA + 1, numFrames - 1);
            const float morph = framePosition - static_cast<float> (frameA);

            const float sampleA = sampleAt (frameA, phase);
            const float sampleB = sampleAt (frameB, phase);
            float value = sampleA + (sampleB - sampleA) * morph;

            if (amplitudeModActive)
            {
                const float gate = juce::jlimit (0.0f, 1.0f, amplitudeMod->getValue (i));
                value *= (1.0f - amplitudeDepth) + amplitudeDepth * gate;
            }

            left[i] = value;

            if (right != nullptr)
                right[i] = value;

            double increment = phaseIncrement;

            if (pitchModActive)
            {
                const float bipolar = 2.0f * pitchMod->getValue (i) - 1.0f;
                const float semitones = pitchDepth * bipolar;
                increment = phaseIncrement * std::pow (2.0, static_cast<double> (semitones) / 12.0);
            }

            phase += increment;

            while (phase >= 1.0)
                phase -= 1.0;

            while (phase < 0.0)
                phase += 1.0;
        }
    }
    else
    {
        clearBlock (buffer, startSample, numSamples);
    }

    // ======================================================================
    // STAGE 2 — Low-pass filter (juce::dsp)
    // The cutoff is smoothed across the block so dragging the knob sweeps
    // rather than stepping.
    // ======================================================================
    cutoffSmoother.setTargetValue (juce::jlimit (20.0f,
                                                 static_cast<float> (sampleRate * 0.45),
                                                 cutoffSmootherTarget.load()));
    lowPassFilter.setCutoffFrequency (cutoffSmoother.skip (numSamples));
    lowPassFilter.setResonance (resonanceTarget.load());

    {
        juce::dsp::AudioBlock<float> block (buffer);
        auto subBlock = block.getSubBlock (static_cast<size_t> (startSample),
                                           static_cast<size_t> (numSamples));
        lowPassFilter.process (juce::dsp::ProcessContextReplacing<float> (subBlock));
    }

    // ======================================================================
    // STAGE 3 — ADSR amplitude envelope
    // Scales every sample by how far through the note we are, so the note
    // fades in, drops to the sustain level, and fades out on release.
    // ======================================================================
    adsr.applyEnvelopeToBuffer (buffer, startSample, numSamples);

    // ======================================================================
    // STAGE 4 — Output gain
    // ======================================================================
    gainSmoother.setTargetValue (juce::Decibels::decibelsToGain (gainDecibelsTarget.load())
                                 * velocityGain);

    const float gainAtStart = gainSmoother.getCurrentValue();
    const float gainAtEnd   = gainSmoother.skip (numSamples);

    buffer.applyGainRamp (startSample, numSamples, gainAtStart, gainAtEnd);
}
