#include "PluginEditor.h"

//==============================================================================
// Minimalist drawing style
//==============================================================================
Koola10LookAndFeel::Koola10LookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (backgroundColour));

    setColour (juce::Label::textColourId,       juce::Colour (textColour));
    setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::outlineColourId,    juce::Colours::transparentBlack);

    setColour (juce::Slider::textBoxTextColourId,      juce::Colour (dimTextColour));
    setColour (juce::Slider::textBoxOutlineColourId,   juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, juce::Colour (accentColour).withAlpha (0.25f));

    setColour (juce::TextButton::buttonColourId,   juce::Colour (panelColour));
    setColour (juce::TextButton::buttonOnColourId, juce::Colour (panelColour));
    setColour (juce::TextButton::textColourOffId,  juce::Colour (textColour));
    setColour (juce::TextButton::textColourOnId,   juce::Colour (textColour));

    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff0f0f11));
    setColour (juce::ComboBox::textColourId,       juce::Colour (textColour));
    setColour (juce::ComboBox::outlineColourId,    juce::Colour (hairlineColour));
    setColour (juce::ComboBox::arrowColourId,      juce::Colour (dimTextColour));

    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff0f0f11));
    setColour (juce::TextEditor::textColourId,       juce::Colour (textColour));
    setColour (juce::TextEditor::outlineColourId,    juce::Colour (hairlineColour));
    setColour (juce::CaretComponent::caretColourId,  juce::Colour (accentColour));
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

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour (0x26ffffff));
    g.strokePath (track, juce::PathStrokeType (1.0f));

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

    const auto discRadius = juce::jmax (2.0f, arcRadius - 7.0f);
    g.setColour (juce::Colour (0xff1a1a1d));
    g.fillEllipse (centre.x - discRadius, centre.y - discRadius, discRadius * 2.0f, discRadius * 2.0f);

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
    g.setFont (juce::Font (juce::FontOptions (10.0f).withKerningFactor (0.18f)));
    g.setColour (juce::Colour (textColour).withAlpha (shouldDrawButtonAsDown ? 0.7f : 1.0f));
    g.drawText (button.getButtonText(), button.getLocalBounds(),
                juce::Justification::centred, false);
}

//==============================================================================
// Pads
//==============================================================================
PadButton::PadButton (int padIndex, const juce::String& name)
    : juce::Button (name), pad (padIndex), padName (name)
{
    setTooltip (name + " (MIDI note " + juce::String (DrumPads::midiNoteForPad (pad)) + ")");
}

void PadButton::setLit (bool shouldBeLit)
{
    if (lit != shouldBeLit)
    {
        lit = shouldBeLit;
        repaint();
    }
}

void PadButton::paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted,
                             bool shouldDrawButtonAsDown)
{
    auto bounds = getLocalBounds().toFloat().reduced (0.5f);

    auto fill = juce::Colour (Koola10LookAndFeel::panelColour);

    if (lit)
        fill = fill.brighter (0.55f);
    else if (shouldDrawButtonAsDown)
        fill = fill.brighter (0.30f);
    else if (shouldDrawButtonAsHighlighted)
        fill = fill.brighter (0.12f);

    g.setColour (fill);
    g.fillRoundedRectangle (bounds, 2.0f);

    g.setColour (juce::Colour (lit ? Koola10LookAndFeel::accentColour
                                   : Koola10LookAndFeel::hairlineColour));
    g.drawRoundedRectangle (bounds, 2.0f, lit ? 1.4f : 1.0f);

    auto text = getLocalBounds().reduced (8, 8);

    auto noteArea = text.removeFromBottom (12);

    g.setColour (juce::Colour (Koola10LookAndFeel::textColour).withAlpha (lit ? 1.0f : 0.82f));
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold).withKerningFactor (0.10f)));
    g.drawFittedText (padName, text, juce::Justification::centredLeft, 3, 0.9f);

    g.setColour (juce::Colour (Koola10LookAndFeel::dimTextColour));
    g.setFont (juce::Font (juce::FontOptions (8.5f)));
    g.drawText (juce::String (DrumPads::midiNoteForPad (pad)), noteArea,
                juce::Justification::bottomLeft, false);
}

//==============================================================================
// Steps
//==============================================================================
StepButton::StepButton (int padIndex, int stepIndex)
    : juce::Button ("step"), pad (padIndex), step (stepIndex)
{
    setClickingTogglesState (true);
}

void StepButton::setStepOn (bool shouldBeOn)
{
    if (getToggleState() != shouldBeOn)
        setToggleState (shouldBeOn, juce::dontSendNotification);
}

void StepButton::setPlayhead (bool isPlayheadHere)
{
    if (playhead != isPlayheadHere)
    {
        playhead = isPlayheadHere;
        repaint();
    }
}

void StepButton::paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = getLocalBounds().toFloat().reduced (0.5f);
    const bool on = getToggleState();

    if (playhead)
    {
        g.setColour (juce::Colour (0x14ffffff));
        g.fillRoundedRectangle (bounds.expanded (2.0f), 2.0f);
    }

    auto fill = on ? juce::Colour (Koola10LookAndFeel::accentColour)
                   : juce::Colour (0xff101012);

    if (shouldDrawButtonAsDown)
        fill = fill.brighter (0.35f);
    else if (shouldDrawButtonAsHighlighted)
        fill = on ? fill.brighter (0.10f) : fill.brighter (0.45f);

    g.setColour (fill);
    g.fillRoundedRectangle (bounds, 2.0f);

    g.setColour (on ? juce::Colour (Koola10LookAndFeel::accentColour)
                    : juce::Colour (Koola10LookAndFeel::hairlineColour));
    g.drawRoundedRectangle (bounds, 2.0f, 1.0f);
}

//==============================================================================
// The editor
//==============================================================================
Koola10DrumMachineAudioProcessorEditor::Koola10DrumMachineAudioProcessorEditor (Koola10DrumMachineAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel (&lookAndFeel);

    // --- Pads and their three knobs ---------------------------------------
    for (int pad = 0; pad < DrumPads::numPads; ++pad)
    {
        auto* button = padButtons.add (new PadButton (pad, DrumPads::names[pad]));
        button->onClick = [this, pad] { processorRef.getEngine().requestPadTrigger (pad); };
        addAndMakeVisible (button);

        configureKnob (knobs[(size_t) (pad * knobsPerPad + 0)], "TUNE",  ParamIDs::padIds[pad][0], 58);
        configureKnob (knobs[(size_t) (pad * knobsPerPad + 1)], "DECAY", ParamIDs::padIds[pad][1], 58);
        configureKnob (knobs[(size_t) (pad * knobsPerPad + 2)], "LEVEL", ParamIDs::padIds[pad][2], 58);
    }

    // --- The 8 x 16 step grid ---------------------------------------------
    for (int pad = 0; pad < DrumPads::numPads; ++pad)
    {
        for (int step = 0; step < StepSequencer::numSteps; ++step)
        {
            auto* button = stepButtons.add (new StepButton (pad, step));
            button->onClick = [this, pad, step, button]
            {
                processorRef.getSequencer().setStep (pad, step, button->getToggleState());
            };

            addAndMakeVisible (button);
        }
    }

    clearButton.onClick = [this]
    {
        processorRef.getSequencer().clear();
        refreshFromSequencer();
    };
    addAndMakeVisible (clearButton);

    // --- Globals -----------------------------------------------------------
    configureKnob (knobs[(size_t) cutoffKnob],    "FILTER CUTOFF", ParamIDs::filterCutoff,    92);
    configureKnob (knobs[(size_t) resonanceKnob], "RESONANCE",     ParamIDs::filterResonance, 92);
    configureKnob (knobs[(size_t) swingKnob],     "SWING",         ParamIDs::swing,           92);
    configureKnob (knobs[(size_t) masterKnob],    "MASTER LEVEL",  ParamIDs::masterLevel,     92);

    tempoLabel.setJustificationType (juce::Justification::centredRight);
    tempoLabel.setFont (juce::Font (juce::FontOptions (11.0f)));
    tempoLabel.setColour (juce::Label::textColourId, juce::Colour (Koola10LookAndFeel::dimTextColour));
    tempoLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (tempoLabel);

    refreshFromSequencer();

    setResizable (false, false);
    setSize (1240, 700);

    startTimerHz (24);
}

Koola10DrumMachineAudioProcessorEditor::~Koola10DrumMachineAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

//==============================================================================
void Koola10DrumMachineAudioProcessorEditor::configureKnob (Knob& knob, const juce::String& labelText,
                                                            const juce::String& parameterID,
                                                            int textBoxWidth)
{
    knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, textBoxWidth, 15);
    knob.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                     juce::MathConstants<float>::pi * 2.75f, true);
    addAndMakeVisible (knob.slider);

    knob.label.setText (labelText, juce::dontSendNotification);
    knob.label.setJustificationType (juce::Justification::centred);
    knob.label.setFont (juce::Font (juce::FontOptions (9.5f).withKerningFactor (0.10f)));
    knob.label.setColour (juce::Label::textColourId, juce::Colour (Koola10LookAndFeel::dimTextColour));
    knob.label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (knob.label);

    knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.getValueTreeState(), parameterID, knob.slider);
}

void Koola10DrumMachineAudioProcessorEditor::placeKnob (Knob& knob, juce::Rectangle<int> cell) const
{
    knob.label.setBounds (cell.removeFromTop (16));
    knob.slider.setBounds (cell.reduced (2, 0));
}

//==============================================================================
void Koola10DrumMachineAudioProcessorEditor::refreshFromSequencer()
{
    auto& sequencer = processorRef.getSequencer();

    for (int pad = 0; pad < DrumPads::numPads; ++pad)
        for (int step = 0; step < StepSequencer::numSteps; ++step)
            stepButtons[pad * StepSequencer::numSteps + step]
                ->setStepOn (sequencer.getStep (pad, step));

    lastStep = -2;
}

void Koola10DrumMachineAudioProcessorEditor::timerCallback()
{
    auto& engine = processorRef.getEngine();
    auto& sequencer = processorRef.getSequencer();

    for (int pad = 0; pad < DrumPads::numPads; ++pad)
        padButtons[pad]->setLit (engine.getPadActivity (pad) > 0.02f);

    const int step = sequencer.getCurrentStep();

    if (step != lastStep)
    {
        lastStep = step;

        for (int i = 0; i < StepSequencer::numSteps; ++i)
        {
            const bool here = (step == i);

            for (int pad = 0; pad < DrumPads::numPads; ++pad)
                stepButtons[pad * StepSequencer::numSteps + i]->setPlayhead (here);
        }
    }

    const double tempo = sequencer.getTempo();
    const bool running = sequencer.isRunning();

    tempoLabel.setText (juce::String (tempo, 1) + " BPM  \xc2\xb7  "
                            + (sequencer.isFollowingHost() ? "HOST CLOCK"
                                                           : (running ? "INTERNAL CLOCK" : "STOPPED")),
                        juce::dontSendNotification);
}

//==============================================================================
void Koola10DrumMachineAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (28, 24);

    headerArea = area.removeFromTop (64);
    area.removeFromTop (10);

    padsCaptionArea = area.removeFromTop (16);
    padsArea = area.removeFromTop (224);
    area.removeFromTop (16);

    sequencerCaptionArea = area.removeFromTop (18);
    area.removeFromTop (8);

    auto globalsColumn = area.removeFromRight (216);
    area.removeFromRight (24);
    sequencerArea = area;

    globalsCaptionArea = globalsColumn.removeFromTop (18);

    // --- Header: tempo readout on the right -------------------------------
    auto headerRight = headerArea;
    tempoLabel.setBounds (headerRight.removeFromRight (300).withSizeKeepingCentre (300, 30));

    // --- Pads: four columns, two rows -------------------------------------
    constexpr int padGapX = 16;
    constexpr int padGapY = 14;
    constexpr int padButtonWidth = 96;

    const int cellWidth = (padsArea.getWidth() - padGapX * 3) / 4;
    const int cellHeight = (padsArea.getHeight() - padGapY) / 2;

    for (int pad = 0; pad < DrumPads::numPads; ++pad)
    {
        const int column = pad % 4;
        const int row = pad / 4;

        juce::Rectangle<int> cell (padsArea.getX() + column * (cellWidth + padGapX),
                                   padsArea.getY() + row * (cellHeight + padGapY),
                                   cellWidth, cellHeight);

        padButtons[pad]->setBounds (cell.removeFromLeft (padButtonWidth).reduced (0, 2));

        const int knobWidth = cell.getWidth() / knobsPerPad;

        for (int k = 0; k < knobsPerPad; ++k)
            placeKnob (knobs[(size_t) (pad * knobsPerPad + k)], cell.removeFromLeft (knobWidth));
    }

    // --- Step grid --------------------------------------------------------
    auto grid = sequencerArea;
    auto numbers = grid.removeFromTop (16);
    grid.removeFromTop (6);

    constexpr int rowGap = 4;
    constexpr int labelWidth = 96;
    constexpr int stepGap = 3;

    const int rowHeight = juce::jmax (12, (grid.getHeight() - rowGap * (DrumPads::numPads - 1))
                                            / DrumPads::numPads);

    const int stepX = grid.getX() + labelWidth + 4;
    const int stepWidth = juce::jmax (12, (grid.getRight() - stepX - stepGap * (StepSequencer::numSteps - 1))
                                             / StepSequencer::numSteps);

    for (int step = 0; step < StepSequencer::numSteps; ++step)
        stepNumberAreas[(size_t) step] = juce::Rectangle<int> (stepX + step * (stepWidth + stepGap),
                                                               numbers.getY(),
                                                               stepWidth, numbers.getHeight());

    for (int pad = 0; pad < DrumPads::numPads; ++pad)
    {
        const int y = grid.getY() + pad * (rowHeight + rowGap);

        rowLabelAreas[(size_t) pad] = juce::Rectangle<int> (grid.getX(), y, labelWidth, rowHeight);

        for (int step = 0; step < StepSequencer::numSteps; ++step)
            stepButtons[pad * StepSequencer::numSteps + step]
                ->setBounds (stepX + step * (stepWidth + stepGap), y + 2, stepWidth, rowHeight - 4);
    }

    // --- Clear button, in the sequencer caption row -----------------------
    auto captionRow = sequencerCaptionArea;
    clearButton.setBounds (captionRow.removeFromRight (64)
                                       .withSizeKeepingCentre (64, 18)
                                       .translated (0, 1));

    // --- Globals: 2 x 2 ---------------------------------------------------
    const int globalCellWidth = globalsColumn.getWidth() / 2;
    const int globalCellHeight = globalsColumn.getHeight() / 2;

    placeKnob (knobs[(size_t) cutoffKnob],
               juce::Rectangle<int> (globalsColumn.getX(), globalsColumn.getY(),
                                     globalCellWidth, globalCellHeight));
    placeKnob (knobs[(size_t) resonanceKnob],
               juce::Rectangle<int> (globalsColumn.getX() + globalCellWidth, globalsColumn.getY(),
                                     globalCellWidth, globalCellHeight));
    placeKnob (knobs[(size_t) swingKnob],
               juce::Rectangle<int> (globalsColumn.getX(), globalsColumn.getY() + globalCellHeight,
                                     globalCellWidth, globalCellHeight));
    placeKnob (knobs[(size_t) masterKnob],
               juce::Rectangle<int> (globalsColumn.getX() + globalCellWidth,
                                     globalsColumn.getY() + globalCellHeight,
                                     globalCellWidth, globalCellHeight));
}

//==============================================================================
void Koola10DrumMachineAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (Koola10LookAndFeel::backgroundColour));

    // --- Header -----------------------------------------------------------
    g.setColour (juce::Colour (Koola10LookAndFeel::textColour));
    g.setFont (juce::Font (juce::FontOptions (22.0f, juce::Font::bold)));
    g.drawText ("KOOLA10 DRUMS", headerArea.withHeight (36),
                juce::Justification::centredLeft, false);

    g.setColour (juce::Colour (Koola10LookAndFeel::dimTextColour));
    g.setFont (juce::Font (juce::FontOptions (10.0f).withKerningFactor (0.24f)));
    g.drawText ("SYNTHESISED DRUM MACHINE  \xc2\xb7  V1",
                headerArea.withTop (headerArea.getY() + 34).withHeight (20),
                juce::Justification::centredLeft, false);

    g.setColour (juce::Colour (Koola10LookAndFeel::hairlineColour));
    g.fillRect (headerArea.getX(), headerArea.getBottom() + 7, headerArea.getWidth(), 1);

    // --- Section captions -------------------------------------------------
    g.setColour (juce::Colour (Koola10LookAndFeel::dimTextColour));
    g.setFont (juce::Font (juce::FontOptions (9.5f).withKerningFactor (0.24f)));
    g.drawText ("PADS", padsCaptionArea, juce::Justification::centredLeft, false);
    g.drawText ("SEQUENCER  \xc2\xb7  16 STEPS", sequencerCaptionArea,
                juce::Justification::centredLeft, false);
    g.drawText ("GLOBAL", globalsCaptionArea, juce::Justification::centredLeft, false);

    // --- Step numbers, with the downbeats brighter ------------------------
    for (int step = 0; step < StepSequencer::numSteps; ++step)
    {
        const bool downbeat = (step % 4) == 0;

        g.setColour (juce::Colour (Koola10LookAndFeel::dimTextColour)
                        .withAlpha (downbeat ? 0.95f : 0.5f));
        g.setFont (juce::Font (juce::FontOptions (9.0f)));
        g.drawText (juce::String (step + 1), stepNumberAreas[(size_t) step],
                    juce::Justification::centred, false);
    }

    // --- Row labels -------------------------------------------------------
    const int currentStep = processorRef.getSequencer().getCurrentStep();

    g.setColour (juce::Colour (Koola10LookAndFeel::dimTextColour));
    g.setFont (juce::Font (juce::FontOptions (9.5f).withKerningFactor (0.16f)));

    for (int pad = 0; pad < DrumPads::numPads; ++pad)
        g.drawText (DrumPads::names[pad], rowLabelAreas[(size_t) pad].withTrimmedLeft (2),
                    juce::Justification::centredLeft, false);

    // --- Hairlines --------------------------------------------------------
    g.setColour (juce::Colour (Koola10LookAndFeel::hairlineColour));

    g.fillRect (sequencerArea.getX(), sequencerCaptionArea.getY() - 1, sequencerArea.getWidth(), 1);

    // Vertical divider between the grid and the global knobs.
    const int dividerX = sequencerArea.getRight() + 12;
    g.fillRect (dividerX, globalsCaptionArea.getY(), 1,
                getLocalBounds().getBottom() - 24 - globalsCaptionArea.getY());

    // --- Stopped hint -----------------------------------------------------
    if (currentStep < 0 && processorRef.getSequencer().isFollowingHost())
    {
        g.setColour (juce::Colour (Koola10LookAndFeel::dimTextColour).withAlpha (0.65f));
        g.setFont (juce::Font (juce::FontOptions (9.5f).withKerningFactor (0.14f)));
        g.drawText ("TRANSPORT STOPPED", sequencerCaptionArea.withTrimmedRight (76),
                    juce::Justification::centredRight, false);
    }
}
