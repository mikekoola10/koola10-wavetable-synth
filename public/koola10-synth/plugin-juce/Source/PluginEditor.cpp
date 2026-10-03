#include "PluginEditor.h"

//==============================================================================
// Minimalist drawing style
//==============================================================================
Koola10LookAndFeel::Koola10LookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (backgroundColour));

    setColour (juce::Label::textColourId,            juce::Colour (textColour));
    setColour (juce::Label::backgroundColourId,      juce::Colours::transparentBlack);
    setColour (juce::Label::outlineColourId,         juce::Colours::transparentBlack);

    setColour (juce::Slider::textBoxTextColourId,    juce::Colour (dimTextColour));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, juce::Colour (accentColour).withAlpha (0.25f));

    setColour (juce::TextButton::buttonColourId,     juce::Colour (panelColour));
    setColour (juce::TextButton::buttonOnColourId,   juce::Colour (panelColour));
    setColour (juce::TextButton::textColourOffId,    juce::Colour (textColour));
    setColour (juce::TextButton::textColourOnId,     juce::Colour (textColour));
}

void Koola10LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPosProportional, float rotaryStartAngle,
                                           float rotaryEndAngle, juce::Slider&)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (6.0f);
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;

    if (radius <= 8.0f)
        return;

    const auto centre = bounds.getCentre();
    const auto arcRadius = radius - 1.0f;

    // Dim track showing the whole travel of the knob.
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour (0x26ffffff));
    g.strokePath (track, juce::PathStrokeType (1.0f));

    // Bright arc showing the current value.
    const auto sliderPos = juce::jlimit (0.0f, 1.0f, sliderPosProportional);

    if (sliderPos > 0.002f)
    {
        const auto currentAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        juce::Path valueArc;
        valueArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                                rotaryStartAngle, currentAngle, true);
        g.setColour (juce::Colour (accentColour));
        g.strokePath (valueArc, juce::PathStrokeType (1.8f));
    }

    // Flat inner disc keeps the knob from looking like a physical dial.
    const auto discRadius = juce::jmax (2.0f, arcRadius - 7.0f);
    g.setColour (juce::Colour (0xff1a1a1d));
    g.fillEllipse (centre.x - discRadius, centre.y - discRadius, discRadius * 2.0f, discRadius * 2.0f);

    // Needle.
    juce::Path needle;
    needle.addRectangle (-0.75f, -discRadius + 1.5f, 1.5f, discRadius * 0.55f);
    g.setColour (juce::Colour (accentColour));
    g.fillPath (needle, juce::AffineTransform::rotation (rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle))
                            .translated (centre.x, centre.y));
}

void Koola10LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                               const juce::Colour&, bool shouldDrawButtonAsHighlighted,
                                               bool shouldDrawButtonAsDown)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);

    auto fill = juce::Colour (panelColour);

    if (shouldDrawButtonAsDown)
        fill = fill.brighter (0.25f);
    else if (shouldDrawButtonAsHighlighted)
        fill = fill.brighter (0.12f);

    g.setColour (fill);
    g.fillRoundedRectangle (bounds, 2.0f);

    g.setColour (juce::Colour (hairlineColour));
    g.drawRoundedRectangle (bounds, 2.0f, 1.0f);
}

void Koola10LookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                         bool, bool shouldDrawButtonAsDown)
{
    g.setFont (juce::Font (juce::FontOptions (10.0f).withExtraKerningFactor (0.18f)));
    g.setColour (juce::Colour (textColour).withAlpha (shouldDrawButtonAsDown ? 0.7f : 1.0f));
    g.drawText (button.getButtonText(), button.getLocalBounds(),
                juce::Justification::centred, false);
}

//==============================================================================
// Wavetable display
//==============================================================================
WavetableDisplay::WavetableDisplay (SynthEngine& engineToShow)
    : engine (engineToShow)
{
    points.assign (512, 0.0f);
    startTimerHz (24);
}

WavetableDisplay::~WavetableDisplay()
{
    stopTimer();
}

void WavetableDisplay::timerCallback()
{
    engine.fillDisplayPoints (points.data(), static_cast<int> (points.size()));
    repaint();
}

void WavetableDisplay::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    g.setColour (juce::Colour (Koola10LookAndFeel::panelColour));
    g.fillRect (bounds);

    if (points.empty())
        return;

    const auto count = static_cast<int> (points.size());
    const auto centreY = bounds.getCentreY();
    const auto scale = bounds.getHeight() * 0.42f;

    // Zero line, so a flat or empty table is still readable.
    g.setColour (juce::Colour (0x14ffffff));
    g.fillRect (bounds.getX(), centreY, bounds.getWidth(), 1.0f);

    juce::Path waveform;

    for (int i = 0; i < count; ++i)
    {
        const auto px = bounds.getX() + bounds.getWidth() * static_cast<float> (i)
                                              / static_cast<float> (juce::jmax (1, count - 1));
        const auto py = centreY - points[static_cast<size_t> (i)] * scale;


        if (i == 0)
            waveform.startNewSubPath (px, py);
        else
            waveform.lineTo (px, py);
    }

    g.setColour (juce::Colour (Koola10LookAndFeel::accentColour));
    g.strokePath (waveform, juce::PathStrokeType (1.5f));
}

//==============================================================================
// The editor
//==============================================================================
Koola10SynthAudioProcessorEditor::Koola10SynthAudioProcessorEditor (Koola10SynthAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      processorRef (p),
      display (p.getEngine())
{
    setLookAndFeel (&lookAndFeel);

    addAndMakeVisible (display);

    loadButton.onClick = [this] { chooseWavetableFile(); };
    addAndMakeVisible (loadButton);

    wavetableName.setJustificationType (juce::Justification::centredRight);
    wavetableName.setFont (juce::Font (juce::FontOptions (11.0f)));
    wavetableName.setColour (juce::Label::textColourId, juce::Colour (Koola10LookAndFeel::dimTextColour));
    wavetableName.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (wavetableName);

    configureKnob (knobs[positionKnob],  "WAVETABLE POSITION", ParamIDs::wavetablePosition);
    configureKnob (knobs[cutoffKnob],    "FILTER CUTOFF",      ParamIDs::filterCutoff);
    configureKnob (knobs[resonanceKnob], "FILTER RESONANCE",   ParamIDs::filterResonance);
    configureKnob (knobs[attackKnob],    "ATTACK",             ParamIDs::ampAttack);
    configureKnob (knobs[decayKnob],     "DECAY",              ParamIDs::ampDecay);
    configureKnob (knobs[sustainKnob],   "SUSTAIN",            ParamIDs::ampSustain);
    configureKnob (knobs[releaseKnob],   "RELEASE",            ParamIDs::ampRelease);
    configureKnob (knobs[levelKnob],     "MASTER LEVEL",       ParamIDs::outputGain);

    refreshWavetableName();

    setResizable (false, false);
    setSize (940, 520);
}

Koola10SynthAudioProcessorEditor::~Koola10SynthAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void Koola10SynthAudioProcessorEditor::configureKnob (Knob& knob, const juce::String& labelText,
                                                      const juce::String& parameterID)
{
    knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 78, 16);
    knob.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                     juce::MathConstants<float>::pi * 2.75f, true);
    addAndMakeVisible (knob.slider);

    knob.label.setText (labelText, juce::dontSendNotification);
    knob.label.setJustificationType (juce::Justification::centred);
    knob.label.setFont (juce::Font (juce::FontOptions (10.0f).withExtraKerningFactor (0.12f)));
    knob.label.setColour (juce::Label::textColourId, juce::Colour (Koola10LookAndFeel::dimTextColour));
    knob.label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (knob.label);

    knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.getValueTreeState(), parameterID, knob.slider);
}

void Koola10SynthAudioProcessorEditor::placeKnob (Knob& knob, juce::Rectangle<int> cell) const
{
    knob.label.setBounds (cell.removeFromTop (20));
    knob.slider.setBounds (cell.reduced (4, 0));
}

void Koola10SynthAudioProcessorEditor::refreshWavetableName()
{
    wavetableName.setText (processorRef.getEngine().getWavetableName(), juce::dontSendNotification);
}

void Koola10SynthAudioProcessorEditor::chooseWavetableFile()
{
    fileChooser = std::make_unique<juce::FileChooser> (
        "Choose a wavetable .wav file",
        juce::File::getSpecialLocation (juce::File::userMusicDirectory),
        "*.wav");

    const auto flags = juce::FileBrowserComponent::openMode
                     | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser)
    {
        const auto file = chooser.getResult();

        if (! file.existsAsFile())
            return;

        if (processorRef.loadWavetableFromFile (file))
        {
            refreshWavetableName();
        }
        else
        {
            juce::NativeMessageBox::showMessageBoxAsync (
                juce::MessageBoxIconType::WarningIcon,
                "Could not load that wavetable",
                "The file could not be read as a .wav file. Try a standard PCM or float .wav.");
        }
    });
}

//==============================================================================
void Koola10SynthAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (28, 24);

    headerArea = area.removeFromTop (64);
    area.removeFromTop (18);

    displayFrameArea = area.removeFromTop (100);
    display.setBounds (displayFrameArea.reduced (1));

    area.removeFromTop (34);
    knobRowArea = area;

    // Right-hand side of the header: the loaded table name and the load button.
    auto headerRight = headerArea;
    loadButton.setBounds (headerRight.removeFromRight (116).withSizeKeepingCentre (116, 32));
    headerRight.removeFromRight (14);
    const int wavetableNameWidth = juce::jmin (320, headerRight.getWidth());
    wavetableName.setBounds (headerRight.removeFromRight (wavetableNameWidth)
                                          .withSizeKeepingCentre (wavetableNameWidth, 32));

    // Eight knobs split into four labelled sections, with hairline dividers.
    static constexpr int knobsPerSection[numSections] = { 1, 2, 4, 1 };
    constexpr int cellWidth = 84;
    constexpr int gapAroundDivider = 32;

    int totalWidth = 0;

    for (int s = 0; s < numSections; ++s)
    {
        totalWidth += knobsPerSection[s] * cellWidth;

        if (s > 0)
            totalWidth += gapAroundDivider * 2 + 1;
    }

    int x = knobRowArea.getX() + juce::jmax (0, (knobRowArea.getWidth() - totalWidth) / 2);
    int knobIndex = 0;

    for (int s = 0; s < numSections; ++s)
    {
        if (s > 0)
        {
            x += gapAroundDivider;
            dividerPositions[static_cast<size_t> (s - 1)] = x;
            x += 1 + gapAroundDivider;
        }

        juce::Rectangle<int> sectionArea (x, knobRowArea.getY(),
                                          knobsPerSection[s] * cellWidth, knobRowArea.getHeight());
        sectionTitleAreas[static_cast<size_t> (s)] = sectionArea.removeFromTop (20);

        for (int k = 0; k < knobsPerSection[s]; ++k)
            placeKnob (knobs[static_cast<size_t> (knobIndex++)], sectionArea.removeFromLeft (cellWidth));

        x += knobsPerSection[s] * cellWidth;
    }
}

//==============================================================================
void Koola10SynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (Koola10LookAndFeel::backgroundColour));

    // --- Header ------------------------------------------------------------
    g.setColour (juce::Colour (Koola10LookAndFeel::textColour));
    g.setFont (juce::Font (juce::FontOptions (22.0f, juce::Font::bold)));
    g.drawText ("KOOLA10 SYNTH", headerArea.withHeight (36),
                juce::Justification::centredLeft, false);

    g.setColour (juce::Colour (Koola10LookAndFeel::dimTextColour));
    g.setFont (juce::Font (juce::FontOptions (10.0f).withExtraKerningFactor (0.24f)));
    g.drawText ("WAVETABLE SYNTHESIZER  ·  V1",
                headerArea.withTop (headerArea.getY() + 34).withHeight (20),
                juce::Justification::centredLeft, false);

    // Hairline under the header.
    g.setColour (juce::Colour (Koola10LookAndFeel::hairlineColour));
    g.fillRect (headerArea.getX(), headerArea.getBottom() + 9, headerArea.getWidth(), 1);

    // --- Wavetable display frame -------------------------------------------
    g.drawRect (displayFrameArea, 1);

    // --- Section titles ----------------------------------------------------
    static const char* const sectionTitles[numSections] = { "OSCILLATOR", "FILTER", "AMPLITUDE", "OUTPUT" };

    g.setColour (juce::Colour (Koola10LookAndFeel::dimTextColour));
    g.setFont (juce::Font (juce::FontOptions (10.0f).withExtraKerningFactor (0.24f)));

    for (int s = 0; s < numSections; ++s)
        g.drawText (sectionTitles[s], sectionTitleAreas[static_cast<size_t> (s)],
                    juce::Justification::centred, false);

    // --- Section dividers --------------------------------------------------
    const int dividerTop = sectionTitleAreas[0].getBottom() + 8;
    const int dividerBottom = knobRowArea.getBottom() - 22;

    if (dividerBottom > dividerTop)
    {
        g.setColour (juce::Colour (Koola10LookAndFeel::hairlineColour));

        for (auto dividerX : dividerPositions)
            g.fillRect (dividerX, dividerTop, 1, dividerBottom - dividerTop);
    }
}
