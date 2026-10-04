#include "Presets.h"

namespace
{
    // Every preset names a factory wave so it recalls a complete sound with one
    // click. The wave names must match Source/FactoryWaves.cpp exactly.
    const FactoryPreset factoryPresets[] =
    {
        // name            waveName        pos   cutoff  res    att     dec    sus   rel    master
        { "Deep Sub",      "Sub Sine",     0.0f,  1200.0f, 0.707f, 0.008f, 0.35f, 0.85f, 0.45f, 0.75f },
        { "Reese Bass",    "Reese",        0.0f,  1500.0f, 1.40f,  0.004f, 0.30f, 0.80f, 0.25f, 0.75f },
        { "Acid Saw",      "Acid Saw",     0.0f,   900.0f, 4.00f,  0.003f, 0.22f, 0.45f, 0.18f, 0.70f },
        { "Bright Pluck",  "Bright Pluck", 0.0f,  6000.0f, 0.90f,  0.002f, 0.20f, 0.00f, 0.30f, 0.70f },
        { "Nasty Lead",    "Nasty Lead",   0.3f,  4500.0f, 1.80f,  0.006f, 0.30f, 0.75f, 0.30f, 0.65f },
        { "Airy Pad",      "Airy",         0.5f,  3200.0f, 0.80f,  0.800f, 1.50f, 0.80f, 2.20f, 0.70f },
        { "E-Piano Keys",  "E-Piano",      0.0f,  4000.0f, 0.75f,  0.003f, 0.70f, 0.25f, 0.60f, 0.72f },
        { "Vocal FX",      "Vocal Ah",     0.4f,  3000.0f, 1.50f,  0.050f, 0.50f, 0.70f, 0.80f, 0.65f },
    };

    constexpr int numFactoryPresets = (int) (sizeof (factoryPresets) / sizeof (factoryPresets[0]));
}

namespace Presets
{
    int getNumFactoryPresets()
    {
        return numFactoryPresets;
    }

    const FactoryPreset& getFactoryPreset (int index)
    {
        jassert (juce::isPositiveAndBelow (index, numFactoryPresets));
        return factoryPresets[index];
    }

    int indexOfName (const juce::String& name)
    {
        for (int i = 0; i < numFactoryPresets; ++i)
            if (name.equalsIgnoreCase (factoryPresets[i].name))
                return i;

        return -1;
    }
}
