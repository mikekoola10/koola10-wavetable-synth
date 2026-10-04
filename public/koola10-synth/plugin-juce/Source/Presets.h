#pragma once

#include <juce_core/juce_core.h>

/**
    A built-in preset: which factory wave to load, plus a value for every knob.

    The values are the same units the parameters use:
      * wavetablePosition : 0 .. 1
      * filterCutoff      : Hz
      * filterResonance   : JUCE Q (0.707 = no resonance)
      * attack/decay/release : seconds
      * sustain           : 0 .. 1
      * masterLevel       : 0 .. 1
*/
struct FactoryPreset
{
    const char* name;
    const char* waveName;
    float wavetablePosition;
    float filterCutoff;
    float filterResonance;
    float attack;
    float decay;
    float sustain;
    float release;
    float masterLevel;
};

namespace Presets
{
    int getNumFactoryPresets();
    const FactoryPreset& getFactoryPreset (int index);

    /** Case-insensitive lookup by preset name. Returns -1 when not found. */
    int indexOfName (const juce::String& name);
}
