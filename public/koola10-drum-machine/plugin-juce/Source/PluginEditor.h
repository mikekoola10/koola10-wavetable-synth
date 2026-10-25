#pragma once

#include "PluginProcessor.h"

#include <array>

//==============================================================================
/** A knob plus the plain-English label that sits above it. */
struct Knob
{
    juce::Slider slider;
    juce::Label  label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

//==============================================================================
/**
    The same minimalist style as Koola10 Synth: hairline dividers, a
    near-monochrome palette, and knobs that are an arc plus a needle rather than
    a skeuomorphic dial.
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

    // The shared minimalist palette, identical to Koola10 Synth.
    static constexpr juce::uint32 backgroundColour = 0xff0b0b0c;
    static constexpr juce::uint32 panelColour      = 0xff141416;
    static constexpr juce::uint32 hairlineColour   = 0x1fffffff;
    static constexpr juce::uint32 textColour       = 0xffececec;
    static constexpr juce::uint32 dimTextColour    = 0xff808088;
    static constexpr juce::uint32 accentColour     = 0xffeaeaea;
};

//==============================================================================
/** One of the eight pads. Clicking it plays the pad; it lights while it sounds. */
class PadButton : public juce::Button
{
public:
    PadButton (int padIndex, const juce::String& name);

    void paintButton (juce::Graphics&, bool shouldDrawButtonAsHighlighted,
                      bool shouldDrawButtonAsDown) override;

    /** Called by the editor's timer with the pad's current activity. */
    void setLit (bool shouldBeLit);

    int getPadIndex() const noexcept { return pad; }

private:
    int pad = 0;
    juce::String padName;
    bool lit = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PadButton)
};

//==============================================================================
/** One cell of the 8 x 16 step grid. */
class StepButton : public juce::Button
{
public:
    StepButton (int padIndex, int stepIndex);

    void paintButton (juce::Graphics&, bool shouldDrawButtonAsHighlighted,
                      bool shouldDrawButtonAsDown) override;

    void setStepOn (bool shouldBeOn);
    void setPlayhead (bool isPlayheadHere);

    int getPadIndex() const noexcept { return pad; }
    int getStepIndex() const noexcept { return step; }

private:
    int pad = 0;
    int step = 0;
    bool playhead = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StepButton)
};

//==============================================================================
class Koola10DrumMachineAudioProcessorEditor : public juce::AudioProcessorEditor,
                                               private juce::Timer
{
public:
    explicit Koola10DrumMachineAudioProcessorEditor (Koola10DrumMachineAudioProcessor&);
    ~Koola10DrumMachineAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // 24 pad knobs (3 per pad), then the four globals.
    static constexpr int knobsPerPad = 3;
    static constexpr int numPadKnobs = DrumPads::numPads * knobsPerPad;
    static constexpr int numKnobs = numPadKnobs + 4;

    enum GlobalKnob { cutoffKnob = numPadKnobs, resonanceKnob, swingKnob, masterKnob };

    void configureKnob (Knob& knob, const juce::String& labelText, const juce::String& parameterID,
                        int textBoxWidth);
    void placeKnob (Knob& knob, juce::Rectangle<int> cell) const;

    void refreshFromSequencer();
    void timerCallback() override;

    Koola10DrumMachineAudioProcessor& processorRef;
    Koola10LookAndFeel lookAndFeel;

    juce::OwnedArray<PadButton> padButtons;
    juce::OwnedArray<StepButton> stepButtons;

    std::array<Knob, numKnobs> knobs;

    juce::TextButton clearButton { "CLEAR" };
    juce::Label tempoLabel;

    // Filled in by resized() and read back by paint().
    juce::Rectangle<int> headerArea;
    juce::Rectangle<int> padsCaptionArea;
    juce::Rectangle<int> padsArea;
    juce::Rectangle<int> sequencerCaptionArea;
    juce::Rectangle<int> sequencerArea;
    juce::Rectangle<int> globalsCaptionArea;
    std::array<juce::Rectangle<int>, DrumPads::numPads> rowLabelAreas;
    std::array<juce::Rectangle<int>, StepSequencer::numSteps> stepNumberAreas;

    int lastStep = -2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Koola10DrumMachineAudioProcessorEditor)
};
