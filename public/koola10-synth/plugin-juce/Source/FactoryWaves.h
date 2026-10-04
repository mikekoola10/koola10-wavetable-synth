#pragma once

#include <juce_core/juce_core.h>

class SynthEngine;

/**
    One built-in wavetable. `data` points at a .wav file that was baked into the
    plugin binary by juce_add_binary_data, so nothing has to be loaded from disk.
*/
struct FactoryWave
{
    const char* category;   // "Sub", "Bass", "Lead", "Pad-Keys", "FX"
    const char* name;       // e.g. "Sub Sine"
    const void* data;       // pointer to the embedded .wav bytes
    int dataSize;           // number of embedded bytes
};

/**
    The 24 factory waves, in the order the browser shows them: grouped by
    category, and within each category in the order the generator script wrote
    them.
*/
namespace FactoryWaves
{
    int getNumWaves();
    const FactoryWave& getWave (int index);

    /** Number of categories and their display order. */
    int getNumCategories();
    const char* getCategoryName (int categoryIndex);

    /** Case-insensitive lookup by display name. Returns -1 when not found. */
    int indexOfName (const juce::String& name);

    /** Decodes the embedded .wav and loads it into the engine. */
    bool loadIntoEngine (SynthEngine& engine, int index);
}
