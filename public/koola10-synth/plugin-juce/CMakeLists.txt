cmake_minimum_required(VERSION 3.22)

project(Koola10Synth VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    # Build the .cpp files in parallel. Harmless on other toolchains.
    add_compile_options(/MP)
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

    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE)

    FetchContent_MakeAvailable(JUCE)
endif()

# ---------------------------------------------------------------------------
# The plugin
# ---------------------------------------------------------------------------
juce_add_plugin(Koola10Synth
    COMPANY_NAME                "Koola10"
    PLUGIN_MANUFACTURER_CODE    Kool
    PLUGIN_CODE                 Kl10
    FORMATS                     VST3 Standalone
    PRODUCT_NAME                "Koola10 Synth"
    DESCRIPTION                 "Wavetable synthesiser: oscillator, low-pass filter, ADSR amplitude envelope."
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     TRUE
    VST3_CATEGORIES             Instrument Synth)

target_sources(Koola10Synth PRIVATE
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
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
