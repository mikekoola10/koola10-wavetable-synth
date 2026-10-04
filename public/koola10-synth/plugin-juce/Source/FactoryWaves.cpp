#include "FactoryWaves.h"
#include "SynthEngine.h"

#include "BinaryData.h"

#include <array>

namespace
{
    // The embedded factory waves live in the generated BinaryData.cpp, which is
    // a separate translation unit. Rather than building the table in a file-scope
    // static (whose initialisation order relative to BinaryData.cpp is not
    // guaranteed), we build it on first use from a function-local static. That
    // guarantees the BinaryData symbols already exist by the time we take their
    // addresses, which removes a static-initialisation-order hazard that can
    // surface as a Debug-build crash on some toolchains.
    constexpr int numFactoryWaves = 24;

    const std::array<FactoryWave, numFactoryWaves>& getWaveTable()
    {
        static const std::array<FactoryWave, numFactoryWaves> waves
        {
            // --- Sub ----------------------------------------------------------
            FactoryWave { "Sub", "Sub Sine",      BinaryData::Sub_Sine_wav,      BinaryData::Sub_Sine_wavSize },
            FactoryWave { "Sub", "Fat 808",       BinaryData::Fat_808_wav,       BinaryData::Fat_808_wavSize },
            FactoryWave { "Sub", "Deep Triangle", BinaryData::Deep_Triangle_wav, BinaryData::Deep_Triangle_wavSize },
            FactoryWave { "Sub", "Sub Pulse",     BinaryData::Sub_Pulse_wav,     BinaryData::Sub_Pulse_wavSize },

            // --- Bass ---------------------------------------------------------
            FactoryWave { "Bass", "Reese",        BinaryData::Reese_wav,         BinaryData::Reese_wavSize },
            FactoryWave { "Bass", "Hollow Bass",  BinaryData::Hollow_Bass_wav,   BinaryData::Hollow_Bass_wavSize },
            FactoryWave { "Bass", "Acid Saw",     BinaryData::Acid_Saw_wav,      BinaryData::Acid_Saw_wavSize },
            FactoryWave { "Bass", "Wobble Fifth", BinaryData::Wobble_Fifth_wav,  BinaryData::Wobble_Fifth_wavSize },
            FactoryWave { "Bass", "Growl Lite",   BinaryData::Growl_Lite_wav,    BinaryData::Growl_Lite_wavSize },

            // --- Lead ---------------------------------------------------------
            FactoryWave { "Lead", "Classic Saw",  BinaryData::Classic_Saw_wav,   BinaryData::Classic_Saw_wavSize },
            FactoryWave { "Lead", "Square 50",    BinaryData::Square_50_wav,     BinaryData::Square_50_wavSize },
            FactoryWave { "Lead", "Pulse 25",     BinaryData::Pulse_25_wav,      BinaryData::Pulse_25_wavSize },
            FactoryWave { "Lead", "Bright Pluck", BinaryData::Bright_Pluck_wav,  BinaryData::Bright_Pluck_wavSize },
            FactoryWave { "Lead", "Nasty Lead",   BinaryData::Nasty_Lead_wav,    BinaryData::Nasty_Lead_wavSize },

            // --- Pad-Keys -----------------------------------------------------
            FactoryWave { "Pad-Keys", "Soft Saw",    BinaryData::Soft_Saw_wav,    BinaryData::Soft_Saw_wavSize },
            FactoryWave { "Pad-Keys", "Airy",        BinaryData::Airy_wav,        BinaryData::Airy_wavSize },
            FactoryWave { "Pad-Keys", "E-Piano",     BinaryData::EPiano_wav,      BinaryData::EPiano_wavSize },
            FactoryWave { "Pad-Keys", "Mellow Keys", BinaryData::Mellow_Keys_wav, BinaryData::Mellow_Keys_wavSize },
            FactoryWave { "Pad-Keys", "Glass",       BinaryData::Glass_wav,       BinaryData::Glass_wavSize },

            // --- FX -----------------------------------------------------------
            FactoryWave { "FX", "Vocal Ah",      BinaryData::Vocal_Ah_wav,      BinaryData::Vocal_Ah_wavSize },
            FactoryWave { "FX", "Bellish",       BinaryData::Bellish_wav,       BinaryData::Bellish_wavSize },
            FactoryWave { "FX", "Digital Crush", BinaryData::Digital_Crush_wav, BinaryData::Digital_Crush_wavSize },
            FactoryWave { "FX", "Static Cycle",  BinaryData::Static_Cycle_wav,  BinaryData::Static_Cycle_wavSize },
            FactoryWave { "FX", "Sweep Up",      BinaryData::Sweep_Up_wav,      BinaryData::Sweep_Up_wavSize },
        };

        return waves;
    }

    const char* const categoryOrder[] = { "Sub", "Bass", "Lead", "Pad-Keys", "FX" };

    constexpr int numCategories = (int) (sizeof (categoryOrder) / sizeof (categoryOrder[0]));
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
        return getWaveTable()[(size_t) index];
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
        const auto& waves = getWaveTable();

        for (int i = 0; i < numFactoryWaves; ++i)
            if (name.equalsIgnoreCase (waves[(size_t) i].name))
                return i;

        return -1;
    }

    bool loadIntoEngine (SynthEngine& engine, int index)
    {
        if (! juce::isPositiveAndBelow (index, numFactoryWaves))
            return false;

        const auto& wave = getWaveTable()[(size_t) index];

        return engine.loadWavetableFromMemory (wave.data,
                                               static_cast<size_t> (wave.dataSize),
                                               wave.name);
    }
}
