#include "DrumEngine.h"

#include <cmath>

namespace
{
    constexpr double twoPi = juce::MathConstants<double>::twoPi;

    /** A multiplier that takes an envelope from 1.0 to -60 dB in `seconds`. */
    float envelopeCoefficient (double seconds, double sampleRate) noexcept
    {
        const double safeSeconds = juce::jmax (0.0002, seconds);
        return (float) std::exp (-6.9077553 / (safeSeconds * sampleRate));
    }

    /** Radians per sample for a frequency in Hz. */
    double phaseIncrement (double hertz, double sampleRate) noexcept
    {
        return twoPi * juce::jmax (0.01, hertz) / juce::jmax (1.0, sampleRate);
    }

    /** The Tune knob is a percentage of the travel: 0 % is an octave down,
        100 % an octave up, 50 % exactly the pad's designed pitch. */
    float tuneRatio (float proportion) noexcept
    {
        const float centred = juce::jlimit (0.0f, 1.0f, proportion) * 2.0f - 1.0f;
        return std::pow (2.0f, centred);
    }

    /** Base gain per pad, so one hit of each pad lands at a similar loudness
        before the Level knob is touched. */
    constexpr float padTrim[DrumPads::numPads] =
    {
        0.80f,   // kick
        0.62f,   // snare
        0.40f,   // closed hat
        0.38f,   // open hat
        0.55f,   // clap
        0.45f,   // rim
        0.60f,   // tom
        0.42f    // crash
    };
}

//==============================================================================
void DrumEngine::VoiceFilter::set (double sr, float cutoffHz, float resonance) noexcept
{
    const float highest = (float) (sr * 0.45);
    const float fc = juce::jlimit (20.0f, juce::jmax (40.0f, highest), cutoffHz);

    const float g = std::tan (juce::MathConstants<float>::pi * fc / (float) sr);
    k = 1.0f / juce::jmax (0.2f, resonance);

    a1 = 1.0f / (1.0f + g * (g + k));
    a2 = g * a1;
    a3 = g * a2;

    ic1 = 0.0f;
    ic2 = 0.0f;
}

//==============================================================================
DrumEngine::DrumEngine()
{
    for (int pad = 0; pad < DrumPads::numPads; ++pad)
    {
        padTune[(size_t) pad].store (0.5f);
        padDecay[(size_t) pad].store (0.5f);
        padLevel[(size_t) pad].store (0.75f);

        triggerRequest[(size_t) pad].store (0);
        handledRequest[(size_t) pad] = 0;

        padActivity[pad].store (0.0f);
    }

    cutoffSmoother.setCurrentAndTargetValue (defaultCutoffHz);
    gainSmoother.setCurrentAndTargetValue (defaultMasterLevel);
}

//==============================================================================
void DrumEngine::prepare (double newSampleRate, int maximumBlockSize)
{
    sampleRate = juce::jmax (8000.0, newSampleRate);

    cutoffSmoother.reset (sampleRate, 0.02);
    gainSmoother.reset (sampleRate, 0.02);

    juce::dsp::ProcessSpec spec { sampleRate,
                                  static_cast<juce::uint32> (juce::jmax (1, maximumBlockSize)),
                                  2 };

    lowPassFilter.prepare (spec);
    lowPassFilter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    lowPassFilter.setResonance (resonanceTarget.load());
    lowPassFilter.setCutoffFrequency (juce::jlimit (20.0f,
                                                    static_cast<float> (sampleRate * 0.45),
                                                    cutoffTarget.load()));

    reset();
}

void DrumEngine::reset()
{
    for (auto& voice : voices)
        voice = Voice {};

    for (auto& activity : padActivity)
        activity.store (0.0f);

    for (auto& handled : handledRequest)
        handled = 0;

    for (int pad = 0; pad < DrumPads::numPads; ++pad)
        handledRequest[(size_t) pad] = triggerRequest[(size_t) pad].load();

    lowPassFilter.reset();
    cutoffSmoother.setCurrentAndTargetValue (cutoffTarget.load());
    gainSmoother.setCurrentAndTargetValue (masterTarget.load());
}

//==============================================================================
void DrumEngine::setPadTune (int pad, float proportion)
{
    if (pad >= 0 && pad < DrumPads::numPads)
        padTune[(size_t) pad].store (juce::jlimit (0.0f, 1.0f, proportion));
}

void DrumEngine::setPadDecay (int pad, float proportion)
{
    if (pad >= 0 && pad < DrumPads::numPads)
        padDecay[(size_t) pad].store (juce::jlimit (0.0f, 1.0f, proportion));
}

void DrumEngine::setPadLevel (int pad, float proportion)
{
    if (pad >= 0 && pad < DrumPads::numPads)
        padLevel[(size_t) pad].store (juce::jlimit (0.0f, 1.0f, proportion));
}

void DrumEngine::setFilterCutoff (float cutoffHz)
{
    cutoffTarget.store (juce::jlimit (20.0f, 20000.0f, cutoffHz));
}

void DrumEngine::setFilterResonance (float resonance)
{
    resonanceTarget.store (juce::jlimit (0.1f, 10.0f, resonance));
}

void DrumEngine::setMasterLevel (float level)
{
    masterTarget.store (juce::jlimit (0.0f, 1.0f, level));
}

//==============================================================================
void DrumEngine::requestPadTrigger (int pad, float velocity)
{
    if (pad < 0 || pad >= DrumPads::numPads)
        return;

    // The counter only has to change for the audio thread to see the hit, so a
    // GUI click never has to allocate or take a lock.
    juce::ignoreUnused (velocity);
    triggerRequest[(size_t) pad].fetch_add (1);
}

float DrumEngine::getPadActivity (int pad) const noexcept
{
    if (pad < 0 || pad >= DrumPads::numPads)
        return 0.0f;

    return padActivity[pad].load();
}

//==============================================================================
void DrumEngine::triggerPad (int pad, float velocity)
{
    if (pad < 0 || pad >= DrumPads::numPads)
        return;

    // A closed hat chokes the open hat, the way a real hi-hat pedal does.
    if (pad == DrumPads::closedHat && voices[(size_t) DrumPads::openHat].active)
        voices[(size_t) DrumPads::openHat].active = false;

    startVoice (pad, velocity);
}

void DrumEngine::startVoice (int pad, float velocity)
{
    Voice& voice = voices[(size_t) pad];
    voice = Voice {};

    voice.active = true;
    voice.pad = pad;
    voice.velocity = juce::jlimit (0.0f, 1.0f, velocity);
    voice.level = padLevel[(size_t) pad].load() * padTrim[pad];

    const float ratio = tuneRatio (padTune[(size_t) pad].load());
    const float decay = juce::jlimit (0.0f, 1.0f, padDecay[(size_t) pad].load());

    const double sr = sampleRate;

    voice.noise.seed (0x9e3779b9u ^ ((juce::uint32) (pad + 1) * 2654435761u) ^ (noiseSeed += 0x9e3779b9u));

    switch (pad)
    {
        //----------------------------------------------------------------------
        // 1. KICK — sine with a fast pitch drop, a beater click and a long tail.
        //----------------------------------------------------------------------
        case DrumPads::kick:
        {
            const float baseHz = 48.0f * ratio;

            voice.inc = phaseIncrement (baseHz, sr);
            voice.incB = phaseIncrement (baseHz * 2.02f, sr);

            voice.pitchStart = 7.0f;                       // starts eight times higher
            voice.pitchEnv = 1.0f;
            voice.pitchCoeff = envelopeCoefficient (0.030, sr);

            voice.bodyAMix = 1.0f;
            voice.bodyBMix = 0.10f;

            voice.ampEnv = 1.0f;
            voice.ampCoeff = envelopeCoefficient (0.14 + 1.30 * decay, sr);

            voice.clickMix = 0.25f;
            voice.clickEnv = 1.0f;
            voice.clickCoeff = envelopeCoefficient (0.0018, sr);

            voice.filter.set (sr, 1200.0f, 0.707f);
            break;
        }

        //----------------------------------------------------------------------
        // 2. SNARE — two body tones plus a band-passed noise burst.
        //----------------------------------------------------------------------
        case DrumPads::snare:
        {
            const float baseHz = 185.0f * ratio;

            voice.inc = phaseIncrement (baseHz, sr);
            voice.incB = phaseIncrement (baseHz * 1.78f, sr);

            voice.bodyAMix = 0.55f;
            voice.bodyBMix = 0.30f;

            voice.pitchStart = 0.45f;
            voice.pitchEnv = 1.0f;
            voice.pitchCoeff = envelopeCoefficient (0.025, sr);

            voice.ampEnv = 1.0f;
            voice.ampCoeff = envelopeCoefficient (0.05 + 0.28 * decay, sr);

            voice.noiseHighMix = 0.15f;
            voice.noiseBandMix = 1.0f;
            voice.noiseEnv = 1.0f;
            voice.noiseCoeff = envelopeCoefficient (0.06 + 0.42 * decay, sr);

            voice.filter.set (sr, 1750.0f * ratio, 0.9f);
            break;
        }

        //----------------------------------------------------------------------
        // 3. CLOSED HAT — high-passed noise, very short.
        //----------------------------------------------------------------------
        case DrumPads::closedHat:
        {
            voice.noiseHighMix = 0.90f;
            voice.noiseBandMix = 0.35f;
            voice.noiseEnv = 1.0f;
            voice.noiseCoeff = envelopeCoefficient (0.018 + 0.10 * decay, sr);

            voice.filter.set (sr, 7800.0f * ratio, 0.7f);
            break;
        }

        //----------------------------------------------------------------------
        // 4. OPEN HAT — the same source, held open.
        //----------------------------------------------------------------------
        case DrumPads::openHat:
        {
            voice.noiseHighMix = 0.85f;
            voice.noiseBandMix = 0.40f;
            voice.noiseEnv = 1.0f;
            voice.noiseCoeff = envelopeCoefficient (0.07 + 0.75 * decay, sr);

            voice.filter.set (sr, 7200.0f * ratio, 0.7f);
            break;
        }

        //----------------------------------------------------------------------
        // 5. CLAP — three quick noise re-hits, then a tail.
        //----------------------------------------------------------------------
        case DrumPads::clap:
        {
            voice.noiseHighMix = 0.25f;
            voice.noiseBandMix = 1.0f;
            voice.noiseEnv = 1.0f;
            voice.noiseCoeff = envelopeCoefficient (0.045 + 0.30 * decay, sr);

            voice.filter.set (sr, 1050.0f * ratio, 1.4f);

            voice.burstsRemaining = 3;
            voice.burstInterval = juce::jmax (1, (int) (0.010 * sr));
            voice.burstCountdown = voice.burstInterval;
            break;
        }

        //----------------------------------------------------------------------
        // 6. RIM — a short resonant ping.
        //----------------------------------------------------------------------
        case DrumPads::rim:
        {
            const float baseHz = 1750.0f * ratio;

            voice.inc = phaseIncrement (baseHz, sr);
            voice.incB = phaseIncrement (baseHz * 2.70f, sr);

            voice.bodyAMix = 0.55f;
            voice.bodyBMix = 0.25f;

            voice.pitchStart = 0.20f;
            voice.pitchEnv = 1.0f;
            voice.pitchCoeff = envelopeCoefficient (0.008, sr);

            voice.ampEnv = 1.0f;
            voice.ampCoeff = envelopeCoefficient (0.012 + 0.14 * decay, sr);

            voice.noiseHighMix = 0.20f;
            voice.noiseBandMix = 1.0f;
            voice.noiseEnv = 1.0f;
            voice.noiseCoeff = envelopeCoefficient (0.006 + 0.04 * decay, sr);

            voice.clickMix = 0.35f;
            voice.clickEnv = 1.0f;
            voice.clickCoeff = envelopeCoefficient (0.0015, sr);

            voice.filter.set (sr, 3200.0f * ratio, 2.0f);
            break;
        }

        //----------------------------------------------------------------------
        // 7. TOM — pitched sine drop with a small noise attack.
        //----------------------------------------------------------------------
        case DrumPads::tom:
        {
            const float baseHz = 118.0f * ratio;

            voice.inc = phaseIncrement (baseHz, sr);
            voice.incB = phaseIncrement (baseHz * 1.50f, sr);

            voice.bodyAMix = 1.0f;
            voice.bodyBMix = 0.12f;

            voice.pitchStart = 1.50f;
            voice.pitchEnv = 1.0f;
            voice.pitchCoeff = envelopeCoefficient (0.060, sr);

            voice.ampEnv = 1.0f;
            voice.ampCoeff = envelopeCoefficient (0.10 + 0.85 * decay, sr);

            voice.noiseBandMix = 0.50f;
            voice.noiseEnv = 1.0f;
            voice.noiseCoeff = envelopeCoefficient (0.015 + 0.06 * decay, sr);

            voice.clickMix = 0.10f;
            voice.clickEnv = 1.0f;
            voice.clickCoeff = envelopeCoefficient (0.0018, sr);

            voice.filter.set (sr, 900.0f * ratio, 0.8f);
            break;
        }

        //----------------------------------------------------------------------
        // 8. CRASH — a filtered noise wash with a metallic sheen on top.
        //----------------------------------------------------------------------
        case DrumPads::crash:
        default:
        {
            const float baseHz = 520.0f * ratio;

            voice.inc = phaseIncrement (baseHz, sr);
            voice.incB = phaseIncrement (baseHz * 1.47f, sr);
            voice.incC = phaseIncrement (baseHz * 1.93f, sr);

            voice.bodyAMix = 0.22f;
            voice.bodyBMix = 0.16f;
            voice.bodyCMix = 0.12f;

            voice.pitchStart = 0.0f;

            voice.ampEnv = 1.0f;
            voice.ampCoeff = envelopeCoefficient (0.18 + 1.00 * decay, sr);

            voice.noiseHighMix = 0.95f;
            voice.noiseBandMix = 0.25f;
            voice.noiseEnv = 1.0f;
            voice.noiseCoeff = envelopeCoefficient (0.22 + 1.50 * decay, sr);

            voice.filter.set (sr, 4300.0f * ratio, 0.6f);
            break;
        }
    }
}

//==============================================================================
float DrumEngine::renderVoice (Voice& voice) noexcept
{
    const float rawNoise = voice.noise.next();

    float low = 0.0f, band = 0.0f, high = 0.0f;
    voice.filter.process (rawNoise, low, band, high);

    // The pitch envelope bends every partial of the pad at once, which is what
    // makes an 808 kick fall rather than sweep.
    const double pitchMul = 1.0 + (double) voice.pitchStart * (double) voice.pitchEnv;

    voice.phase  += voice.inc  * pitchMul;
    voice.phaseB += voice.incB * pitchMul;
    voice.phaseC += voice.incC * pitchMul;

    while (voice.phase  >= twoPi) voice.phase  -= twoPi;
    while (voice.phaseB >= twoPi) voice.phaseB -= twoPi;
    while (voice.phaseC >= twoPi) voice.phaseC -= twoPi;

    const float body = (float) std::sin (voice.phase)  * voice.bodyAMix
                     + (float) std::sin (voice.phaseB) * voice.bodyBMix
                     + (float) std::sin (voice.phaseC) * voice.bodyCMix;

    const float noisePart = (high * voice.noiseHighMix + band * voice.noiseBandMix) * voice.noiseEnv;
    const float clickPart = rawNoise * voice.clickEnv * voice.clickMix;

    const float value = (body * voice.ampEnv + noisePart + clickPart)
                      * voice.level * voice.velocity;

    voice.ampEnv   *= voice.ampCoeff;
    voice.pitchEnv *= voice.pitchCoeff;
    voice.noiseEnv *= voice.noiseCoeff;
    voice.clickEnv *= voice.clickCoeff;

    // Clap: re-open the noise envelope a few times, a few milliseconds apart.
    if (voice.burstsRemaining > 0 && --voice.burstCountdown <= 0)
    {
        voice.noiseEnv = 1.0f;
        --voice.burstsRemaining;
        voice.burstCountdown = voice.burstInterval;
    }

    if (voice.ampEnv < 1.0e-4f && voice.noiseEnv < 1.0e-4f && voice.clickEnv < 1.0e-4f)
        voice.active = false;

    return value;
}

//==============================================================================
void DrumEngine::process (juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    if (numSamples <= 0 || buffer.getNumChannels() == 0)
        return;

    juce::ScopedNoDenormals noDenormals;

    // Pad hits requested from the GUI are picked up here, at the top of a
    // segment, so they line up with the block instead of waiting a block.
    for (int pad = 0; pad < DrumPads::numPads; ++pad)
    {
        const auto requested = triggerRequest[(size_t) pad].load();

        if (requested != handledRequest[(size_t) pad])
        {
            handledRequest[(size_t) pad] = requested;
            triggerPad (pad, 1.0f);
        }
    }

    const int numChannels = buffer.getNumChannels();
    auto* left = buffer.getWritePointer (0, startSample);
    auto* right = numChannels > 1 ? buffer.getWritePointer (1, startSample) : nullptr;

    std::array<float, DrumPads::numPads> peak {};

    // ======================================================================
    // STAGE 1 — Eight one-shot voices, summed to mono
    // ======================================================================
    for (int i = 0; i < numSamples; ++i)
    {
        float sum = 0.0f;

        for (auto& voice : voices)
        {
            if (! voice.active)
                continue;

            sum += renderVoice (voice);

            auto& voicePeak = peak[(size_t) voice.pad];
            const float envelope = juce::jmax (voice.ampEnv, voice.noiseEnv);

            if (envelope > voicePeak)
                voicePeak = envelope;
        }

        left[i] = sum;

        if (right != nullptr)
            right[i] = sum;
    }

    for (int pad = 0; pad < DrumPads::numPads; ++pad)
        padActivity[pad].store (peak[(size_t) pad]);

    // ======================================================================
    // STAGE 2 — Shared low-pass filter (juce::dsp)
    // Cutoff is smoothed so turning the knob sweeps instead of stepping.
    // ======================================================================
    cutoffSmoother.setTargetValue (juce::jlimit (20.0f,
                                                 static_cast<float> (sampleRate * 0.45),
                                                 cutoffTarget.load()));
    lowPassFilter.setCutoffFrequency (cutoffSmoother.skip (numSamples));
    lowPassFilter.setResonance (resonanceTarget.load());

    {
        juce::dsp::AudioBlock<float> block (buffer);
        auto subBlock = block.getSubBlock (static_cast<size_t> (startSample),
                                           static_cast<size_t> (numSamples));

        lowPassFilter.process (juce::dsp::ProcessContextReplacing<float> (subBlock));
    }

    // ======================================================================
    // STAGE 3 — Master level, then a plain clamp so nothing leaves the plugin
    // above full scale however many pads land on the same step.
    // ======================================================================
    gainSmoother.setTargetValue (masterTarget.load());

    const float gainAtStart = gainSmoother.getCurrentValue();
    const float gainAtEnd = gainSmoother.skip (numSamples);

    buffer.applyGainRamp (startSample, numSamples, gainAtStart, gainAtEnd);

    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* samples = buffer.getWritePointer (channel, startSample);

        for (int i = 0; i < numSamples; ++i)
            samples[i] = juce::jlimit (-1.0f, 1.0f, samples[i]);
    }
}
