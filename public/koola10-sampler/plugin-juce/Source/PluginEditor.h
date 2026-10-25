#pragma once

#include "PluginProcessor.h"

#include <array>
#include <functional>
#include <memory>

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
    The same minimalist style as Koola10 Synth and the drum machine: hairline
    dividers, a near-monochrome palette, and knobs that are an arc plus a needle
    rather than a skeuomorphic dial.
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
/**
    The waveform strip in a slot: the whole file, drawn quietly, with the part
    between START and END drawn bright.
*/
class WaveformView : public juce::Component
{
public:
    WaveformView();

    void paint (juce::Graphics&) override;

    void setSample (std::shared_ptr<const LoadedSample> newSample);
    void setTrim (float startProportion, float endProportion);
    void setPlaying (bool shouldBePlaying);
    void setEmptyMessage (const juce::String& text);

    /** Called when the strip is clicked, so the slot can be auditioned. */
    std::function<void()> onAudition;

    void mouseDown (const juce::MouseEvent&) override;

private:
    std::shared_ptr<const LoadedSample> sample;
    float startProportion = 0.0f;
    float endProportion = 1.0f;
    bool playing = false;
    juce::String emptyMessage { "NO SAMPLE LOADED" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformView)
};

//==============================================================================
/**
    One of the eight slots: a file, a waveform, six knobs, a play mode and the
    button that hands the keyboard to this slot.
*/
class SlotCard : public juce::Component
{
public:
    SlotCard (Koola10SamplerAudioProcessor& processor, int slotIndex);

    void paint (juce::Graphics&) override;
    void resized() override;

    /** Called from the editor's timer. Only repaints when something changed. */
    void refresh();

    int getSlotIndex() const noexcept { return slot; }

private:
    void configureKnob (Knob& knob, const juce::String& labelText,
                        const juce::String& parameterID, int textBoxWidth);
    void chooseFile();

    Koola10SamplerAudioProcessor& processorRef;
    int slot = 0;

    WaveformView waveform;

    juce::TextButton loadButton { "LOAD" };
    juce::TextButton keysButton { "KEYS" };
    juce::ComboBox modeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;

    std::array<Knob, 6> knobs;

    std::unique_ptr<juce::FileChooser> chooser;

    juce::Rectangle<int> titleArea;

    juce::String fileName;
    bool fileMissing = false;
    bool keysHere = false;
    bool playing = false;
    float lastStart = -1.0f, lastEnd = -1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SlotCard)
};

//==============================================================================
class Koola10SamplerAudioProcessorEditor : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    explicit Koola10SamplerAudioProcessorEditor (Koola10SamplerAudioProcessor&);
    ~Koola10SamplerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void configureKnob (Knob& knob, const juce::String& labelText,
                        const juce::String& parameterID, int textBoxWidth);
    void placeKnob (Knob& knob, juce::Rectangle<int> cell) const;
    void timerCallback() override;

    Koola10SamplerAudioProcessor& processorRef;
    Koola10LookAndFeel lookAndFeel;

    juce::OwnedArray<SlotCard> cards;

    Knob cutoffKnob, resonanceKnob, masterKnob;

    // Filled in by resized() and read back by paint().
    juce::Rectangle<int> headerArea;
    juce::Rectangle<int> slotsCaptionArea;
    juce::Rectangle<int> slotsArea;
    juce::Rectangle<int> sharedCaptionArea;
    juce::Rectangle<int> sharedArea;
    juce::Rectangle<int> infoArea;

    juce::String voicesLine;
    juce::String keysLine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Koola10SamplerAudioProcessorEditor)
};
