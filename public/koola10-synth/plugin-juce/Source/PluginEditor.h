#pragma once

#include "PluginProcessor.h"

#include <array>
#include <vector>

//==============================================================================
/**
    Pairing of a knob with the plain-English label above it.

    The label is a separate juce::Label rather than the slider's built-in label
    so that the two always sit in exactly the same place and use the same type.
*/
struct Knob
{
    juce::Slider slider;
    juce::Label  label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

//==============================================================================
/**
    A minimalist drawing style: hairline dividers, a near-monochrome palette,
    and knobs that are an arc plus a needle rather than a skeuomorphic dial.
*/
class Koola10LookAndFeel : public juce::LookAndFeel_V4
{
public:
    Koola10LookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics&, juce::TextButton&,
                         bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;

    // The shared minimalist palette.
    static constexpr juce::uint32 backgroundColour = 0xff0b0b0c;
    static constexpr juce::uint32 panelColour      = 0xff141416;
    static constexpr juce::uint32 hairlineColour   = 0x1fffffff;
    static constexpr juce::uint32 textColour       = 0xffececec;
    static constexpr juce::uint32 dimTextColour    = 0xff808088;
    static constexpr juce::uint32 accentColour     = 0xffeaeaea;
};

//==============================================================================
/** Draws the frame of the loaded wavetable that the Position knob points at. */
class WavetableDisplay : public juce::Component,
                         private juce::Timer
{
public:
    explicit WavetableDisplay (SynthEngine& engineToShow);
    ~WavetableDisplay() override;

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    SynthEngine& engine;
    std::vector<float> points;
};

//==============================================================================
/**
    The slim left sidebar.

    Its header holds the factory preset dropdown and the user Save/Load buttons;
    below that sits a search box and a scrollable list of waves grouped by
    category (Sub, Bass, Lead, Pad-Keys, FX) followed by a User group with the
    .wav files loaded this session. One click loads a wave into the oscillator.
*/
class WaveBrowser : public juce::Component,
                    public juce::ListBoxModel
{
public:
    explicit WaveBrowser (Koola10SynthAudioProcessor& processor);
    ~WaveBrowser() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    /** Rebuilds the row list (after a search change or a new user wave). */
    void refresh();

    /** Highlights the loaded wave and the active preset. Cheap; safe to poll. */
    void refreshSelection();

    // ListBoxModel
    int getNumRows() override;
    void paintListBoxItem (int rowNumber, juce::Graphics&, int width, int height,
                           bool rowIsSelected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;

private:
    struct Row
    {
        bool isHeader = false;
        juce::String text;
        int factoryWaveIndex = -1;   // >= 0 => load this factory wave
        int userFileIndex = -1;      // >= 0 => load this user file
    };

    void rebuildRows();
    void loadRow (const Row& row);
    void applySelectedPreset();
    void savePresetFile();
    void loadPresetFile();

    Koola10SynthAudioProcessor& processorRef;

    juce::ComboBox presetBox;
    juce::TextButton savePresetButton { "SAVE" };
    juce::TextButton loadPresetButton { "LOAD" };

    juce::TextEditor search;
    juce::ListBox list { "waves" };
    std::vector<Row> rows;

    juce::Rectangle<int> captionArea;

    std::unique_ptr<juce::FileChooser> presetFileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveBrowser)
};

//==============================================================================
class Koola10SynthAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         private juce::Timer
{
public:
    explicit Koola10SynthAudioProcessorEditor (Koola10SynthAudioProcessor&);
    ~Koola10SynthAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // Knob indices, in the order the sections read left to right.
    enum KnobIndex
    {
        positionKnob = 0,
        cutoffKnob,
        resonanceKnob,
        attackKnob,
        decayKnob,
        sustainKnob,
        releaseKnob,
        levelKnob,
        numKnobs
    };

    enum SectionIndex
    {
        oscillatorSection = 0,
        filterSection,
        amplitudeSection,
        outputSection,
        numSections
    };

    void configureKnob (Knob& knob, const juce::String& labelText, const juce::String& parameterID);
    void placeKnob (Knob& knob, juce::Rectangle<int> cell) const;
    void chooseWavetableFile();
    void refreshWavetableHeader();
    void timerCallback() override;

    Koola10SynthAudioProcessor& processorRef;
    Koola10LookAndFeel lookAndFeel;

    WavetableDisplay display;
    WaveBrowser browser;

    juce::TextButton loadButton { "LOAD .WAV" };
    juce::Label wavetableName;

    std::array<Knob, numKnobs> knobs;

    // Filled in by resized() and read back by paint().
    juce::Rectangle<int> headerArea;
    juce::Rectangle<int> browserPanelArea;
    juce::Rectangle<int> displayFrameArea;
    juce::Rectangle<int> knobRowArea;
    std::array<juce::Rectangle<int>, numSections> sectionTitleAreas;
    std::array<int, numSections - 1> dividerPositions {};

    std::unique_ptr<juce::FileChooser> fileChooser;

    // Last-seen state, so the poll timer only does work when something changed.
    juce::String lastWaveName;
    int lastUserWaveCount = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Koola10SynthAudioProcessorEditor)
};
