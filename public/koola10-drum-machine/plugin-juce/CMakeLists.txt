cmake_minimum_required(VERSION 3.22)

project(Koola10Drums VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    # Build the .cpp files in parallel. Harmless on other toolchains.
    add_compile_options(/MP)

    # Visual Studio 2022 17.10+ and Visual Studio 2026 ship an MSVC STL in which
    # std::mutex's constructor is constexpr. Mixing object files from an older
    # toolchain with a newer one in the same link can trip a Debug-heap check
    # (debug_heap.cpp, "is_block_type_valid(header->_block_use)") on the first
    # mutex lock. This define restores the previous construction behaviour and is
    # Microsoft's documented escape hatch. It matches the setting already used by
    # Koola10 Synth.
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

    # Same JUCE as Koola10 Synth v2: 8.0.12 is the first release that officially
    # supports Visual Studio 2026, and it builds cleanly with Visual Studio 2022.
    # If you pin a different revision, delete the build/ folder so the new JUCE
    # is fetched and compiled from scratch.
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.12
        GIT_SHALLOW    TRUE)

    FetchContent_MakeAvailable(JUCE)
endif()

# ---------------------------------------------------------------------------
# The plugin
# ---------------------------------------------------------------------------
juce_add_plugin(Koola10Drums
    COMPANY_NAME                "Koola10"
    PLUGIN_MANUFACTURER_CODE    Kool
    PLUGIN_CODE                 Kd10
    FORMATS                     VST3 Standalone
    PRODUCT_NAME                "Koola10 Drum Machine"
    DESCRIPTION                 "Synthesised drum machine: eight pads, a 16-step sequencer and a shared low-pass filter."
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     TRUE
    VST3_CATEGORIES             Instrument Drum)

target_sources(Koola10Drums PRIVATE
    Source/DrumEngine.cpp
    Source/Sequencer.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(Koola10Drums PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source)

target_compile_definitions(Koola10Drums PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1)

target_link_libraries(Koola10Drums
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
