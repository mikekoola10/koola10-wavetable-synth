cmake_minimum_required(VERSION 3.22)

project(Koola10Synth VERSION 2.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    # Build the .cpp files in parallel. Harmless on other toolchains.
    add_compile_options(/MP)

    # Visual Studio 2022 17.10+ (and Visual Studio 2026) ships an MSVC STL in
    # which std::mutex's constructor is constexpr. When object files produced by
    # a different toolchain/STL revision end up in the same link - which happens
    # if a stale build/ tree is reused after a compiler upgrade - the first
    # mutex lock can fail a Debug-heap check (debug_heap.cpp,
    # "is_block_type_valid(header->_block_use)"). Defining
    # _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR restores the previous mutex
    # construction behaviour for every translation unit and is Microsoft's
    # documented escape hatch for this class of crash. It is harmless on toolchains
    # that do not need it.
    add_compile_definitions(_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR)
endif()

# ---------------------------------------------------------------------------
# JUCE
#
# If a JUCE/ folder already sits next to this file we use it. Otherwise CMake
# downloads JUCE once into build/_deps and reuses it from then on.
# ---------------------------------------------------------------------------
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)

    # 8.0.12 is the first JUCE release that officially supports Visual Studio
    # 2026 (VS2026 support landed in 8.0.11 and became the default in 8.0.12).
    # JUCE 8.0.4 predates that work; building it with the VS2026 Debug CRT is
    # what produced the startup Debug-heap assertion. If you ever need to pin a
    # different revision, change GIT_TAG below and delete the build/ tree so the
    # new JUCE is fetched and compiled cleanly.
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.12
        GIT_SHALLOW    TRUE)

    FetchContent_MakeAvailable(JUCE)
endif()

# ---------------------------------------------------------------------------
# Factory wavetables
#
# These 24 single-cycle .wav files are baked into the plugin binary, so the
# instrument ships with sounds and never needs to find a file on disk. The
# generated header is #include "BinaryData.h" and each file is available as
# BinaryData::<name>, e.g. BinaryData::Sub_Sine_wav.
#
# Regenerate the .wav files with: python3 factory_waves_gen.py
# ---------------------------------------------------------------------------
juce_add_binary_data(Koola10FactoryWaves
    SOURCES
        "factory_waves/Sub/Sub Sine.wav"
        "factory_waves/Sub/Fat 808.wav"
        "factory_waves/Sub/Deep Triangle.wav"
        "factory_waves/Sub/Sub Pulse.wav"
        "factory_waves/Bass/Reese.wav"
        "factory_waves/Bass/Hollow Bass.wav"
        "factory_waves/Bass/Acid Saw.wav"
        "factory_waves/Bass/Wobble Fifth.wav"
        "factory_waves/Bass/Growl Lite.wav"
        "factory_waves/Lead/Classic Saw.wav"
        "factory_waves/Lead/Square 50.wav"
        "factory_waves/Lead/Pulse 25.wav"
        "factory_waves/Lead/Bright Pluck.wav"
        "factory_waves/Lead/Nasty Lead.wav"
        "factory_waves/Pad-Keys/Soft Saw.wav"
        "factory_waves/Pad-Keys/Airy.wav"
        "factory_waves/Pad-Keys/E-Piano.wav"
        "factory_waves/Pad-Keys/Mellow Keys.wav"
        "factory_waves/Pad-Keys/Glass.wav"
        "factory_waves/FX/Vocal Ah.wav"
        "factory_waves/FX/Bellish.wav"
        "factory_waves/FX/Digital Crush.wav"
        "factory_waves/FX/Static Cycle.wav"
        "factory_waves/FX/Sweep Up.wav")

# ---------------------------------------------------------------------------
# The plugin
# ---------------------------------------------------------------------------
juce_add_plugin(Koola10Synth
    COMPANY_NAME                "Koola10"
    PLUGIN_MANUFACTURER_CODE    Kool
    PLUGIN_CODE                 Kl10
    FORMATS                     VST3 Standalone
    PRODUCT_NAME                "Koola10 Synth"
    DESCRIPTION                 "Wavetable synthesiser: frame-based oscillator, low-pass filter, ADSR amplitude envelope."
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     TRUE
    VST3_CATEGORIES             Instrument Synth)

target_sources(Koola10Synth PRIVATE
    Source/FactoryWaves.cpp
    Source/Presets.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/SynthEngine.cpp)

target_include_directories(Koola10Synth PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source)

target_compile_definitions(Koola10Synth PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1)

target_link_libraries(Koola10Synth
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        Koola10FactoryWaves
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
