#include "SamplerEngine.h"

#include <algorithm>
#include <cmath>

//==============================================================================
namespace
{
    /**
        Reads one sample at a fractional position with a Catmull-Rom curve.

        Straight linear interpolation is audible as a dull top end once a
        sample is pitched up a few semitones; four points costs almost nothing
        here and keeps a one-shot sounding like itself across two octaves.
    */
    inline float readInterpolated (const juce::AudioBuffer<float>& buffer,
                                   int channel, double position) noexcept
    {
        const int length = buffer.getNumSamples();

        if (length <= 0 || channel < 0 || channel >= buffer.getNumChannels())
            return 0.0f;

        const int index = (int) std::floor (position);

        if (index < 0)
            return 0.0f;

        if (index >= length - 1)
            return buffer.getSample (channel, length - 1);

        const float t = (float) (position - (double) index);

        const float xm1 = buffer.getSample (channel, juce::jmax (0, index - 1));
        const float x0  = buffer.getSample (channel, index);
        const float x1  = buffer.getSample (channel, index + 1);
        const float x2  = buffer.getSample (channel, juce::jmin (length - 1, index + 2));

        const float a = -0.5f * xm1 + 1.5f * x0 - 1.5f * x1 + 0.5f * x2;
        const float b =        xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        const float c = -0.5f * xm1             + 0.5f * x1;

        return ((a * t + b) * t + c) * t + x0;
    }
}

//==============================================================================
SamplerEngine::SamplerEngine()
{
    for (int slot = 0; slot < SamplerSlots::numSlots; ++slot)
    {
        const auto index = (size_t) slot;

        slotTune[index].store (0.5f);       // the sample at its recorded pitch
        slotLevel[index].store (0.8f);
        slotAttack[index].store (0.02f);    // about 2 ms
        slotRelease[index].store (0.25f);   // about 190 ms
        slotStart[index].store (0.0f);
        slotEnd[index].store (1.0f);
        slotLooping[index].store (false);

        published[index].store (nullptr);
        voicesUsingSlot[index].store (0);
        auditionRequest[index].store (0);
        slotActivity[index].store (0.0f);
    }
}

//==============================================================================
void SamplerEngine::prepare (double newSampleRate, int maximumBlockSize)
{
    sampleRate = juce::jmax (8000.0, newSampleRate);

    for (auto& filter : lowPassFilters)
    {
        filter.prepare ({ sampleRate, (juce::uint32) juce::jmax (1, maximumBlockSize), 1 });
        filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    }

    lastGain = masterTarget.load();

    reset();
}

void SamplerEngine::reset()
{
    for (auto& voice : voices)
        stopVoice (voice);

    for (auto& filter : lowPassFilters)
        filter.reset();

    activeVoiceCount.store (0);
}

//==============================================================================
void SamplerEngine::setSlotSample (int slot, std::shared_ptr<const LoadedSample> sample)
{
    if (slot < 0 || slot >= SamplerSlots::numSlots)
        return;

    const auto index = (size_t) slot;
    const auto previous = slotSamples[index];

    slotSamples[index] = sample;
    published[index].store (sample.get());

    // The old sample may still be sounding. Keep it alive and let
    // releaseRetiredSamples() free it once the voices have let go.
    if (previous != nullptr && previous != sample)
        retired.push_back (Retired { previous, slot, blockCounter.load() });
}

void SamplerEngine::clearSlotSample (int slot)
{
    setSlotSample (slot, nullptr);
}

std::shared_ptr<const LoadedSample> SamplerEngine::getSlotSample (int slot) const
{
    if (slot < 0 || slot >= SamplerSlots::numSlots)
        return {};

    return slotSamples[(size_t) slot];
}

void SamplerEngine::releaseRetiredSamples()
{
    if (retired.empty())
        return;

    const auto now = blockCounter.load();

    for (auto it = retired.begin(); it != retired.end(); )
    {
        // Two rendered blocks have to have passed since the swap, and the slot
        // has to be silent, before the audio thread can no longer be reading
        // the old audio. A host that never runs its audio callback simply
        // holds on to the replaced samples; nothing is ever freed underneath a
        // playing voice.
        const bool enoughAudioPassed = now >= it->retiredAtBlock + 2;
        const bool slotIsSilent = voicesUsingSlot[(size_t) it->slot].load() == 0;

        if (enoughAudioPassed && slotIsSilent)
            it = retired.erase (it);
        else
            ++it;
    }
}

//==============================================================================
void SamplerEngine::setSlotTune (int slot, float proportion)
{
    if (slot >= 0 && slot < SamplerSlots::numSlots)
        slotTune[(size_t) slot].store (juce::jlimit (0.0f, 1.0f, proportion));
}

void SamplerEngine::setSlotLevel (int slot, float proportion)
{
    if (slot >= 0 && slot < SamplerSlots::numSlots)
        slotLevel[(size_t) slot].store (juce::jlimit (0.0f, 1.0f, proportion));
}

void SamplerEngine::setSlotAttack (int slot, float proportion)
{
    if (slot >= 0 && slot < SamplerSlots::numSlots)
        slotAttack[(size_t) slot].store (juce::jlimit (0.0f, 1.0f, proportion));
}

void SamplerEngine::setSlotRelease (int slot, float proportion)
{
    if (slot >= 0 && slot < SamplerSlots::numSlots)
        slotRelease[(size_t) slot].store (juce::jlimit (0.0f, 1.0f, proportion));
}

void SamplerEngine::setSlotStart (int slot, float proportion)
{
    if (slot >= 0 && slot < SamplerSlots::numSlots)
        slotStart[(size_t) slot].store (juce::jlimit (0.0f, 1.0f, proportion));
}

void SamplerEngine::setSlotEnd (int slot, float proportion)
{
    if (slot >= 0 && slot < SamplerSlots::numSlots)
        slotEnd[(size_t) slot].store (juce::jlimit (0.0f, 1.0f, proportion));
}

void SamplerEngine::setSlotLooping (int slot, bool shouldLoop)
{
    if (slot >= 0 && slot < SamplerSlots::numSlots)
        slotLooping[(size_t) slot].store (shouldLoop);
}

void SamplerEngine::setFilterCutoff (float cutoffHz)
{
    cutoffTarget.store (juce::jlimit (20.0f, 20000.0f, cutoffHz));
}

void SamplerEngine::setFilterResonance (float resonance)
{
    resonanceTarget.store (juce::jlimit (0.1f, 10.0f, resonance));
}

void SamplerEngine::setMasterLevel (float level)
{
    masterTarget.store (juce::jlimit (0.0f, 1.0f, level));
}

void SamplerEngine::setPlaySlot (int slot)
{
    playSlot.store (juce::jlimit (0, SamplerSlots::numSlots - 1, slot));
}

//==============================================================================
void SamplerEngine::noteOn (int midiNote, float velocity)
{
    startVoice (playSlot.load(), midiNote, velocity);
}

void SamplerEngine::noteOff (int midiNote)
{
    for (auto& voice : voices)
    {
        if (voice.active && voice.looping && voice.note == midiNote)
        {
            voice.attacking = false;
            voice.releasing = true;
        }
    }
}

void SamplerEngine::requestAudition (int slot)
{
    if (slot >= 0 && slot < SamplerSlots::numSlots)
        auditionRequest[(size_t) slot].fetch_add (1);
}

float SamplerEngine::getSlotActivity (int slot) const noexcept
{
    if (slot < 0 || slot >= SamplerSlots::numSlots)
        return 0.0f;

    return slotActivity[(size_t) slot].load();
}

//==============================================================================
SamplerEngine::Voice* SamplerEngine::findFreeVoice()
{
    for (auto& voice : voices)
        if (! voice.active)
            return &voice;

    // All sixteen are busy: take the one that has been sounding longest, which
    // is the least surprising choice when a fast run outruns the polyphony.
    Voice* oldest = &voices[0];

    for (auto& voice : voices)
        if (voice.age < oldest->age)
            oldest = &voice;

    return oldest;
}

void SamplerEngine::startVoice (int slot, int note, float velocity)
{
    if (slot < 0 || slot >= SamplerSlots::numSlots)
        return;

    const auto* sample = published[(size_t) slot].load();

    // An empty slot, or one whose file has moved, is silent. It must never be
    // a crash or a stuck note.
    if (sample == nullptr || sample->getLength() < 2)
        return;

    auto* voice = findFreeVoice();

    if (voice == nullptr)
        return;

    if (voice->active)
        stopVoice (*voice);

    const auto index = (size_t) slot;

    const double length = (double) sample->getLength();
    const double maxStart = juce::jmax (0.0, length - 2.0);

    const double startPos = juce::jlimit (0.0, maxStart,
                                          (double) slotStart[index].load() * (length - 1.0));
    double endPos = juce::jlimit (0.0, length, (double) slotEnd[index].load() * length);

    if (endPos < startPos + 2.0)
        endPos = juce::jmin (length, startPos + 2.0);

    const double semitones = (double) (note - SamplerSlots::rootNote)
                           + (double) (slotTune[index].load() - 0.5f)
                                 * 2.0 * SamplerSlots::tuneRangeSemitones;

    const double pitchRatio = std::pow (2.0, semitones / 12.0);
    const double rate = pitchRatio * (sample->sourceSampleRate / juce::jmax (1.0, sampleRate));

    const double attackProportion  = (double) slotAttack[index].load();
    const double releaseProportion = (double) slotRelease[index].load();

    const double attackSeconds  = 0.001 + 2.0 * attackProportion * attackProportion;
    const double releaseSeconds = 0.005 + 3.0 * releaseProportion * releaseProportion;

    voice->active = true;
    voice->slot = slot;
    voice->note = note;
    voice->sample = sample;
    voice->position = startPos;
    voice->startPosition = startPos;
    voice->endPosition = endPos;
    voice->rate = juce::jlimit (0.01, 32.0, rate);
    voice->looping = slotLooping[index].load();
    voice->gain = slotLevel[index].load() * juce::jlimit (0.0f, 1.0f, velocity);
    voice->env = 0.0f;
    voice->attacking = true;
    voice->releasing = false;
    voice->attackStep = (float) (1.0 / juce::jmax (1.0, attackSeconds * sampleRate));
    voice->releaseStep = (float) (1.0 / juce::jmax (1.0, releaseSeconds * sampleRate));
    voice->age = ++voiceAge;

    voicesUsingSlot[index].fetch_add (1);
}

void SamplerEngine::stopVoice (Voice& voice) noexcept
{
    if (voice.active && voice.slot >= 0 && voice.slot < SamplerSlots::numSlots)
        voicesUsingSlot[(size_t) voice.slot].fetch_sub (1);

    voice.active = false;
    voice.sample = nullptr;
    voice.slot = -1;
}

//==============================================================================
void SamplerEngine::renderVoice (Voice& voice, float& outLeft, float& outRight) noexcept
{
    const auto* sample = voice.sample;

    if (sample == nullptr || sample->getLength() < 2)
    {
        stopVoice (voice);
        return;
    }

    const int length = sample->getLength();

    if (voice.position >= voice.endPosition)
    {
        if (voice.looping)
        {
            voice.position = voice.startPosition;
        }
        else
        {
            stopVoice (voice);
            return;
        }
    }

    if (voice.position < 0.0)
        voice.position = voice.startPosition;

    // Straight ramps: up on note-on, down on note-off while a loop is held.
    if (voice.attacking)
    {
        voice.env += voice.attackStep;

        if (voice.env >= 1.0f)
        {
            voice.env = 1.0f;
            voice.attacking = false;
        }
    }

    if (voice.releasing)
    {
        voice.env -= voice.releaseStep;

        if (voice.env <= 0.0f)
        {
            stopVoice (voice);
            return;
        }
    }

    // A trimmed tail stops where the user put it, so a one-shot gets a 2 ms
    // fade over its last few samples. Cutting a waveform dead is a click; this
    // is short enough to be inaudible and long enough to stop it.
    float edgeFade = 1.0f;

    if (! voice.looping)
    {
        const double fadeLength = 0.002 * sampleRate;
        const double remaining = voice.endPosition - voice.position;

        if (remaining < fadeLength)
            edgeFade = (float) juce::jlimit (0.0, 1.0, remaining / juce::jmax (1.0, fadeLength));
    }

    const float gain = voice.env * voice.gain * edgeFade;
    const int numChannels = sample->buffer.getNumChannels();

    outLeft  += readInterpolated (sample->buffer, 0, voice.position) * gain;
    outRight += readInterpolated (sample->buffer, numChannels > 1 ? 1 : 0, voice.position) * gain;

    voice.position += voice.rate;

    if (voice.position >= (double) length && voice.looping)
        voice.position = voice.startPosition;
}

//==============================================================================
void SamplerEngine::handleAuditionRequests()
{
    for (int slot = 0; slot < SamplerSlots::numSlots; ++slot)
    {
        const auto requested = auditionRequest[(size_t) slot].load();

        if (requested != handledAudition[(size_t) slot])
        {
            handledAudition[(size_t) slot] = requested;
            startVoice (slot, SamplerSlots::rootNote, 1.0f);
        }
    }
}

void SamplerEngine::pushParametersToDsp()
{
    const float cutoff = juce::jlimit (20.0f, 20000.0f, cutoffTarget.load());
    const float resonance = juce::jlimit (0.1f, 10.0f, resonanceTarget.load());

    for (auto& filter : lowPassFilters)
    {
        filter.setCutoffFrequency (cutoff);
        filter.setResonance (resonance);
    }
}

void SamplerEngine::beginBlock (int numBlockSamples)
{
    blockNumSamples = juce::jmax (1, numBlockSamples);
    blockCounter.fetch_add (1);

    handleAuditionRequests();
    pushParametersToDsp();

    blockGainStart = lastGain;
    blockGainTarget = masterTarget.load();
    lastGain = blockGainTarget;
}

void SamplerEngine::render (juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    if (numSamples <= 0)
        return;

    const int numChannels = buffer.getNumChannels();
    const int bufferLength = buffer.getNumSamples();

    if (numChannels <= 0 || startSample < 0 || startSample >= bufferLength)
        return;

    if (blockNumSamples <= 0)
        blockNumSamples = bufferLength;

    numSamples = juce::jmin (numSamples, bufferLength - startSample);

    if (numSamples <= 0)
        return;

    auto* leftOut = buffer.getWritePointer (0);
    auto* rightOut = numChannels > 1 ? buffer.getWritePointer (1) : nullptr;

    // -----------------------------------------------------------------------
    // STAGE 1 — Sixteen sample voices, summed
    // -----------------------------------------------------------------------
    for (int i = startSample; i < startSample + numSamples; ++i)
    {
        float left = 0.0f, right = 0.0f;

        for (auto& voice : voices)
            if (voice.active)
                renderVoice (voice, left, right);

        if (rightOut != nullptr)
        {
            leftOut[i] += left;
            rightOut[i] += right;
        }
        else
        {
            leftOut[i] += 0.5f * (left + right);
        }
    }

    // -----------------------------------------------------------------------
    // STAGE 2 — Shared low-pass filter (juce::dsp), one per output channel
    // -----------------------------------------------------------------------
    const int channelsToFill = juce::jmin (2, numChannels);

    for (int channel = 0; channel < channelsToFill; ++channel)
    {
        auto* data = buffer.getWritePointer (channel) + startSample;
        auto& filter = lowPassFilters[(size_t) channel];

        for (int i = 0; i < numSamples; ++i)
            data[i] = filter.processSample (0, data[i]);
    }

    // -----------------------------------------------------------------------
    // STAGE 3 — Master level, then a plain clamp
    //   The ramp is a straight line across the block, so it does not step when
    //   the block is split into segments.
    // -----------------------------------------------------------------------
    const float blockProportionStart = (float) startSample / (float) blockNumSamples;
    const float blockProportionEnd = (float) (startSample + numSamples) / (float) blockNumSamples;

    const float gainStart = blockGainStart + (blockGainTarget - blockGainStart) * blockProportionStart;
    const float gainEnd   = blockGainStart + (blockGainTarget - blockGainStart) * blockProportionEnd;

    buffer.applyGainRamp (startSample, numSamples, gainStart, gainEnd);

    for (int channel = 0; channel < channelsToFill; ++channel)
    {
        auto* data = buffer.getWritePointer (channel) + startSample;

        for (int i = 0; i < numSamples; ++i)
        {
            const float value = data[i];
            data[i] = std::isfinite (value) ? juce::jlimit (-1.0f, 1.0f, value) : 0.0f;
        }
    }

    // -----------------------------------------------------------------------
    // Panel feedback: which slots are sounding, and how busy the voices are
    // -----------------------------------------------------------------------
    std::array<float, SamplerSlots::numSlots> activity {};
    int liveVoices = 0;

    for (const auto& voice : voices)
    {
        if (! voice.active)
            continue;

        ++liveVoices;

        if (voice.slot >= 0 && voice.slot < SamplerSlots::numSlots)
            activity[(size_t) voice.slot] = 1.0f;
    }

    for (int slot = 0; slot < SamplerSlots::numSlots; ++slot)
        slotActivity[(size_t) slot].store (activity[(size_t) slot]);

    activeVoiceCount.store (liveVoices);
}

void SamplerEngine::process (juce::AudioBuffer<float>& buffer)
{
    juce::ScopedNoDenormals noDenormals;

    buffer.clear();

    beginBlock (buffer.getNumSamples());
    render (buffer, 0, buffer.getNumSamples());
}
