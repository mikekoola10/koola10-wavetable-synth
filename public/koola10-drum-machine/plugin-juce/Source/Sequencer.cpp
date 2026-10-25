#include "Sequencer.h"
#include "DrumEngine.h"

#include <cmath>

namespace
{
    constexpr double fallbackTempo = 130.0;
}

//==============================================================================
StepSequencer::StepSequencer()
{
    for (auto& cell : pattern)
        cell.store (false);

    events.fill (Event {});
    loadFactoryPattern();
}

void StepSequencer::prepare (double newSampleRate)
{
    sampleRate = juce::jmax (8000.0, newSampleRate);
    reset();
}

void StepSequencer::reset()
{
    numEvents = 0;
    internalPosition = 0.0;
    currentStep.store (-1);
}

//==============================================================================
bool StepSequencer::getStep (int pad, int step) const noexcept
{
    if (pad < 0 || pad >= numPads || step < 0 || step >= numSteps)
        return false;

    return pattern[(size_t) (pad * numSteps + step)].load();
}

void StepSequencer::setStep (int pad, int step, bool shouldBeOn) noexcept
{
    if (pad < 0 || pad >= numPads || step < 0 || step >= numSteps)
        return;

    pattern[(size_t) (pad * numSteps + step)].store (shouldBeOn);
}

void StepSequencer::clear() noexcept
{
    for (auto& cell : pattern)
        cell.store (false);
}

//==============================================================================
juce::String StepSequencer::toPatternString() const
{
    juce::String text;

    for (int pad = 0; pad < numPads; ++pad)
        for (int step = 0; step < numSteps; ++step)
            text << (getStep (pad, step) ? '1' : '0');

    return text;
}

void StepSequencer::setPatternFromString (const juce::String& text) noexcept
{
    if (text.length() != numPads * numSteps)
        return;

    for (int pad = 0; pad < numPads; ++pad)
        for (int step = 0; step < numSteps; ++step)
            pattern[(size_t) (pad * numSteps + step)]
                .store (text[pad * numSteps + step] == '1');
}

//==============================================================================
void StepSequencer::loadFactoryPattern() noexcept
{
    clear();

    // Kick on every beat.
    for (int step : { 0, 4, 8, 12 })
        setStep (DrumPads::kick, step, true);

    // Snare on the backbeat, with a clap layered on the second one.
    setStep (DrumPads::snare, 4, true);
    setStep (DrumPads::snare, 12, true);
    setStep (DrumPads::clap, 12, true);

    // Closed hats on the off-eighths, open hat where the bar turns around.
    for (int step : { 0, 2, 4, 6, 8, 10, 12 })
        setStep (DrumPads::closedHat, step, true);

    setStep (DrumPads::openHat, 14, true);

    // A rim tick and a small tom fill so the bar has movement.
    setStep (DrumPads::rim, 7, true);
    setStep (DrumPads::tom, 11, true);
    setStep (DrumPads::tom, 15, true);

    setStep (DrumPads::crash, 0, true);
}

//==============================================================================
void StepSequencer::setSwing (float swingProportion) noexcept
{
    swing.store (juce::jlimit (0.0f, 1.0f, swingProportion));
}

//==============================================================================
void StepSequencer::addEvent (int pad, int sampleOffset) noexcept
{
    if (numEvents >= (int) events.size())
        return;

    events[(size_t) numEvents++] = Event { pad, sampleOffset };
}

void StepSequencer::scanSteps (double startPos, double endPos, double advance, int numSamples) noexcept
{
    if (advance <= 0.0 || endPos <= startPos)
        return;

    // Half a sample of slack, so a step that lands exactly on the first sample
    // of a block is played once rather than never (or twice).
    const double tolerance = 0.5 * advance / (double) juce::jmax (1, numSamples);
    const double from = startPos - tolerance;
    const double to = endPos - tolerance;

    // Swing delays every off-beat sixteenth. At 100 % an off-beat lands two
    // thirds of the way through the pair, which is the classic triplet feel.
    const double swingOffset = (double) juce::jlimit (0.0f, 1.0f, swing.load()) * (1.0 / 6.0);

    const long long firstStep = (long long) std::floor (from) - 1;
    const long long lastStep = (long long) std::floor (to);

    for (long long step = firstStep; step <= lastStep; ++step)
    {
        const bool offBeat = (step % 2) != 0;
        const double onset = (double) step + (offBeat ? swingOffset : 0.0);

        if (onset < from || onset >= to)
            continue;

        const int index = wrapStep (step);
        const double relative = (onset - from) / juce::jmax (1.0e-9, to - from);
        const int offset = juce::jlimit (0, juce::jmax (0, numSamples - 1),
                                        (int) std::floor (relative * (double) numSamples));

        for (int pad = 0; pad < numPads; ++pad)
            if (pattern[(size_t) (pad * numSteps + index)].load())
                addEvent (pad, offset);
    }
}

//==============================================================================
void StepSequencer::process (juce::AudioPlayHead* playHead, int numSamples)
{
    numEvents = 0;

    if (numSamples <= 0)
        return;

    double startPos = 0.0;
    double bpm = fallbackTempo;
    bool hostDriven = false;
    bool hostHasTransport = false;

    if (playHead != nullptr)
    {
        const auto position = playHead->getPosition();

        if (position)
        {
            const auto ppq = position->getPpqPosition();

            if (ppq)
            {
                hostHasTransport = true;

                if (position->getIsPlaying())
                {
                    if (const auto hostBpm = position->getBpm())
                        bpm = juce::jlimit (20.0, 400.0, *hostBpm);

                    startPos = *ppq * 4.0;      // quarter notes -> sixteenths
                    hostDriven = true;
                }
            }
        }
    }

    hostClock.store (hostHasTransport);

    // A host that reports a timeline but is not playing means silence: the
    // transport is stopped or parked, and drums must not free-run under it.
    if (hostHasTransport && ! hostDriven)
    {
        running.store (false);
        currentStep.store (-1);
        internalPosition = 0.0;
        return;
    }

    // No timeline at all (the standalone build): run on our own clock, so the
    // pattern can be auditioned without a DAW.
    if (! hostDriven)
    {
        startPos = internalPosition;
        bpm = fallbackTempo;
    }

    tempo.store (bpm);
    running.store (true);

    const double advance = (double) numSamples * bpm / 60.0 * 4.0
                         / juce::jmax (1.0, sampleRate);

    const double endPos = startPos + advance;

    scanSteps (startPos, endPos, advance, numSamples);

    if (! hostDriven)
        internalPosition = endPos;

    currentStep.store (wrapStep ((long long) std::floor (endPos - 1.0e-9)));
}
