#include "FactoryWaves.h"
#include "SynthEngine.h"

#include "BinaryData.h"

namespace
{
    const FactoryWave factoryWaves[] =
    {
        // --- Sub --------------------------------------------------------------
        { "Sub", "Sub Sine",      BinaryData::Sub_Sine_wav,      BinaryData::Sub_Sine_wavSize },
        { "Sub", "Fat 808",       BinaryData::Fat_808_wav,       BinaryData::Fat_808_wavSize },
        { "Sub", "Deep Triangle", BinaryData::Deep_Triangle_wav, BinaryData::Deep_Triangle_wavSize },
        { "Sub", "Sub Pulse",     BinaryData::Sub_Pulse_wav,     BinaryData::Sub_Pulse_wavSize },

        // --- Bass -------------------------------------------------------------
        { "Bass", "Reese",        BinaryData::Reese_wav,         BinaryData::Reese_wavSize },
        { "Bass", "Hollow Bass",  BinaryData::Hollow_Bass_wav,   BinaryData::Hollow_Bass_wavSize },
        { "Bass", "Acid Saw",     BinaryData::Acid_Saw_wav,      BinaryData::Acid_Saw_wavSize },
        { "Bass", "Wobble Fifth", BinaryData::Wobble_Fifth_wav,  BinaryData::Wobble_Fifth_wavSize },
        { "Bass", "Growl Lite",   BinaryData::Growl_Lite_wav,    BinaryData::Growl_Lite_wavSize },

        // --- Lead -------------------------------------------------------------
        { "Lead", "Classic Saw",  BinaryData::Classic_Saw_wav,   BinaryData::Classic_Saw_wavSize },
        { "Lead", "Square 50",    BinaryData::Square_50_wav,     BinaryData::Square_50_wavSize },
        { "Lead", "Pulse 25",     BinaryData::Pulse_25_wav,      BinaryData::Pulse_25_wavSize },
        { "Lead", "Bright Pluck", BinaryData::Bright_Pluck_wav,  BinaryData::Bright_Pluck_wavSize },
        { "Lead", "Nasty Lead",   BinaryData::Nasty_Lead_wav,    BinaryData::Nasty_Lead_wavSize },

        // --- Pad-Keys ---------------------------------------------------------
        { "Pad-Keys", "Soft Saw",    BinaryData::Soft_Saw_wav,    BinaryData::Soft_Saw_wavSize },
        { "Pad-Keys", "Airy",        BinaryData::Airy_wav,        BinaryData::Airy_wavSize },
        { "Pad-Keys", "E-Piano",     BinaryData::EPiano_wav,      BinaryData::EPiano_wavSize },
        { "Pad-Keys", "Mellow Keys", BinaryData::Mellow_Keys_wav, BinaryData::Mellow_Keys_wavSize },
        { "Pad-Keys", "Glass",       BinaryData::Glass_wav,       BinaryData::Glass_wavSize },

        // --- FX ---------------------------------------------------------------
        { "FX", "Vocal Ah",      BinaryData::Vocal_Ah_wav,      BinaryData::Vocal_Ah_wavSize },
        { "FX", "Bellish",       BinaryData::Bellish_wav,       BinaryData::Bellish_wavSize },
        { "FX", "Digital Crush", BinaryData::Digital_Crush_wav, BinaryData::Digital_Crush_wavSize },
        { "FX", "Static Cycle",  BinaryData::Static_Cycle_wav,  BinaryData::Static_Cycle_wavSize },
        { "FX", "Sweep Up",      BinaryData::Sweep_Up_wav,      BinaryData::Sweep_Up_wavSize },
    };

    const char* const categoryOrder[] = { "Sub", "Bass", "Lead", "Pad-Keys", "FX" };

    constexpr int numFactoryWaves = (int) (sizeof (factoryWaves) / sizeof (factoryWaves[0]));
    constexpr int numCategories   = (int) (sizeof (categoryOrder) / sizeof (categoryOrder[0]));
}

namespace FactoryWaves
{
    int getNumWaves()
    {
        return numFactoryWaves;
    }

    const FactoryWave& getWave (int index)
    {
        jassert (juce::isPositiveAndBelow (index, numFactoryWaves));
        return factoryWaves[index];
    }

    int getNumCategories()
    {
        return numCategories;
    }

    const char* getCategoryName (int categoryIndex)
    {
        jassert (juce::isPositiveAndBelow (categoryIndex, numCategories));
        return categoryOrder[categoryIndex];
    }

    int indexOfName (const juce::String& name)
    {
        for (int i = 0; i < numFactoryWaves; ++i)
            if (name.equalsIgnoreCase (factoryWaves[i].name))
                return i;

        return -1;
    }

    bool loadIntoEngine (SynthEngine& engine, int index)
    {
        if (! juce::isPositiveAndBelow (index, numFactoryWaves))
            return false;

        const auto& wave = factoryWaves[index];

        return engine.loadWavetableFromMemory (wave.data,
                                               static_cast<size_t> (wave.dataSize),
                                               wave.name);
    }
}
