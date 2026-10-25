#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>

/**
    The 16-step pattern grid and its clock.

    Tempo follows the host. Each block, the sequencer asks the play head where it
    is, works out which sixteenth-note steps begin inside the block, and hands
    back the hits with a sample offset so the processor can place them exactly.

    The standalone build has no transport, so the sequencer free-runs there at
    130 BPM, which is the tempo of the Diner beat these pads were voiced for.

    Nothing here touches audio: the processor decides where the hits land.
*/
class StepSequencer
{
public:
    static constexpr int numSteps = 16;
    static constexpr int numPads = 8;

    /** One hit: which pad, and how far into the block it starts. */
    struct Event
    {
        int pad = 0;
        int sampleOffset = 0;
    };

    StepSequencer();

    void prepare (double newSampleRate);
    void reset();

    //==============================================================================
    // Pattern edit (message thread) and read (either thread)
    //==============================================================================

    bool getStep (int pad, int step) const noexcept;
    void setStep (int pad, int step, bool shouldBeOn) noexcept;
    void clear() noexcept;

    /** 128 characters of '0' and '1', row by row: pad 0 steps 0-15, then pad 1. */
    juce::String toPatternString() const;
    void setPatternFromString (const juce::String& text) noexcept;

    /** The shipped pattern: a straight four-bar-ready house/hip-hop bed. */
    void loadFactoryPattern() noexcept;

    //==============================================================================
    void setSwing (float swingProportion) noexcept;

    //==============================================================================
    /** Reads the transport for this block and fills the event list.
        Call once per block, before rendering. */
    void process (juce::AudioPlayHead* playHead, int numSamples);

    int getNumEvents() const noexcept { return numEvents; }
    const Event& getEvent (int index) const noexcept { return events[(size_t) index]; }

    /** The step the play head is sitting on, for the grid highlight. -1 when stopped. */
    int getCurrentStep() const noexcept { return currentStep.load(); }

    /** The tempo in use: the host's when it reports one, otherwise 130. */
    double getTempo() const noexcept { return tempo.load(); }

    bool isRunning() const noexcept { return running.load(); }

    /** True when the clock is coming from a host transport, false when the
        sequencer is free-running (the standalone build). */
    bool isFollowingHost() const noexcept { return hostClock.load(); }

    /** Step numbers are always 0..15, even for negative input. */
    static int wrapStep (long long step) noexcept
    {
        const auto wrapped = step % numSteps;
        return (int) (wrapped < 0 ? wrapped + numSteps : wrapped);
    }

private:
    void addEvent (int pad, int sampleOffset) noexcept;
    void scanSteps (double startPos, double endPos, double advance, int numSamples) noexcept;

    std::array<std::atomic<bool>, (size_t) numPads * numSteps> pattern;

    std::atomic<float> swing { 0.0f };
    std::atomic<int> currentStep { -1 };
    std::atomic<double> tempo { 130.0 };
    std::atomic<bool> running { false };
    std::atomic<bool> hostClock { false };

    std::array<Event, 64> events;
    int numEvents = 0;

    double sampleRate = 44100.0;

    // Free-running clock, used when there is no host transport (standalone).
    double internalPosition = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StepSequencer)
};
