#include "SynthEngine.h"

#include <cmath>

namespace
{
    // A ramp time of 20 ms is long enough to hide knob jumps, short enough that
    // the control still feels immediate.
    constexpr double smoothingSeconds = 0.02;

    constexpr int defaultFrameLength = 2048;
}

//==============================================================================
SynthEngine::SynthEngine()
{
    buildDefaultTable();

    positionSmoother.setCurrentAndTargetValue (0.0f);
    cutoffSmoother.setCurrentAndTargetValue (2000.0f);
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
}

//==============================================================================
bool SynthEngine::loadWavetableFile (const juce::File& file)
{
    if (! file.existsAsFile())
        return false;

    juce::AudioFormatManager formats;
    formats.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));

    if (reader == nullptr || reader->lengthInSamples < 64)
        return false;

    const int totalSamples = static_cast<int> (
        juce::jmin (reader->lengthInSamples, static_cast<juce::int64> (1 << 24)));

    // --- Guess the frame length ------------------------------------------
    // Wavetables are usually stored as N frames of a fixed size, e.g.
    // 64 frames x 2048 samples. Try the common sizes first and accept the first
    // one that divides the file into 1..256 whole frames.
    int chosenFrameLength = 0;
    int chosenNumFrames   = 0;

    for (int candidate : { 2048, 1024, 512, 256, 128, 64 })
    {
        if (totalSamples % candidate != 0)
            continue;

        const int frames = totalSamples / candidate;

        if (frames >= 1 && frames <= maxFrames)
        {
            chosenFrameLength = candidate;
            chosenNumFrames = frames;
            break;
        }
    }

    if (chosenFrameLength == 0)
    {
        // Not a tidy multi-frame table: treat the whole file as ONE frame.
        chosenFrameLength = totalSamples;
        chosenNumFrames = 1;
    }

    // --- Read the file into memory ---------------------------------------
    juce::AudioBuffer<float> fileBuffer (static_cast<int> (reader->numChannels), totalSamples);
    reader->read (&fileBuffer, 0, totalSamples, 0, true, true);

    // Collapse to mono: a wavetable is a shape, not a stereo image.
    std::vector<float> mono (static_cast<size_t> (totalSamples), 0.0f);

    for (int ch = 0; ch < fileBuffer.getNumChannels(); ++ch)
        for (int i = 0; i < totalSamples; ++i)
            mono[static_cast<size_t> (i)] += fileBuffer.getSample (ch, i);

    const float channelScale = 1.0f / static_cast<float> (juce::jmax (1, fileBuffer.getNumChannels()));

    for (auto& sample : mono)
        sample *= channelScale;

    // --- Hand the new table to the audio thread --------------------------
    {
        const juce::SpinLock::ScopedLockType lock (tableLock);
        table = std::move (mono);
        frameLength = chosenFrameLength;
        numFrames = chosenNumFrames;
        wavetableName = file.getFileNameWithoutExtension();
    }

    return true;
}

juce::String SynthEngine::getWavetableName() const
{
    const juce::SpinLock::ScopedLockType lock (tableLock);
    return wavetableName;
}

void SynthEngine::fillDisplayPoints (float* dest, int numPoints) const
{
    const juce::SpinLock::ScopedLockType lock (tableLock);

    if (numFrames <= 0 || frameLength <= 0 || numPoints <= 0)
        return;

    const float position = positionSmoother.getCurrentValue();
    const int frame = juce::jlimit (0, numFrames - 1,
                                    static_cast<int> (std::lround (position * static_cast<float> (numFrames - 1))));

    for (int i = 0; i < numPoints; ++i)
        dest[i] = sampleAt (frame, static_cast<double> (i) / static_cast<double> (numPoints));
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

//==============================================================================
void SynthEngine::noteOn (int midiNoteNumber, float velocity)
{
    velocityGain = juce::jlimit (0.0f, 1.0f, velocity);

    const double frequency = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
    phaseIncrement = frequency / sampleRate;

    // Start each new note from the top of the wave so short notes are predictable.
    phase = 0.0;
    adsr.noteOn();
}

void SynthEngine::noteOff()
{
    adsr.noteOff();
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
    // two neighbouring frames and blend between them.
    // ======================================================================
    positionSmoother.setTargetValue (positionSmootherTarget.load());

    const bool voiceIsRunning = (numFrames > 0) && adsr.isActive();

    if (voiceIsRunning)
    {
        auto* left  = buffer.getWritePointer (0, startSample);
        auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1, startSample) : nullptr;

        for (int i = 0; i < numSamples; ++i)
        {
            const float position = positionSmoother.getNextValue();

            const float framePosition = position * static_cast<float> (juce::jmax (0, numFrames - 1));
            const int frameA = juce::jlimit (0, numFrames - 1, static_cast<int> (framePosition));
            const int frameB = juce::jmin (frameA + 1, numFrames - 1);
            const float morph = framePosition - static_cast<float> (frameA);

            const float sampleA = sampleAt (frameA, phase);
            const float sampleB = sampleAt (frameB, phase);
            const float value = sampleA + (sampleB - sampleA) * morph;

            left[i] = value;

            if (right != nullptr)
                right[i] = value;

            phase += phaseIncrement;

            if (phase >= 1.0)
                phase -= 1.0;
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
