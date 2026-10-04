#pragma once

#include <juce_core/juce_core.h>

/**
    A modulation source.

    v2 ships no LFOs or step sequencers yet — this interface exists so that the
    v3 "Gross Beat update" can add trance gates, pitch envelopes and
    wavetable-position sequencers without rewriting the oscillator, filter or
    envelope code.

    A source answers one question: "what is your value for this sample of the
    block?". The answer is unipolar (0 .. 1). The engine scales it by the depth
    stored in its ModulationRouting and, for targets where a bipolar signal
    makes sense (pitch), maps it to -1 .. +1 internally.
*/
class ModSource
{
public:
    virtual ~ModSource() = default;

    /** Called once before playback starts. */
    virtual void prepare (double sampleRate, int maximumBlockSize)
    {
        juce::ignoreUnused (sampleRate, maximumBlockSize);
    }

    /** Called when playback stops. */
    virtual void reset() {}

    /** Called on note start / stop so time-based sources can restart. */
    virtual void noteOn()  {}
    virtual void noteOff() {}

    /** Unipolar 0 .. 1 value for sample `sampleIndexInBlock` of the block. */
    virtual float getValue (int sampleIndexInBlock) const noexcept = 0;

    /** Returning false lets the engine skip this source entirely (cheap). */
    virtual bool isActive() const noexcept { return true; }
};

/** The engine parameters a ModSource may be routed to. */
enum class ModTarget
{
    wavetablePosition,
    pitch,
    amplitude
};

/** One "source -> target" connection with a depth (0 .. 1). */
struct ModulationRouting
{
    ModSource* source = nullptr;
    float depth = 0.0f;

    bool isActive() const noexcept
    {
        return source != nullptr && depth != 0.0f && source->isActive();
    }
};
