#include "PluginEditor.h"

#include <cmath>

//==============================================================================
namespace
{
    /** The six knob labels on every slot, in the same order as the parameters. */
    constexpr const char* slotKnobLabels[6] =
    {
        "TUNE", "LEVEL", "ATTACK", "RELEASE", "START", "END"
    };

    /** Label above, knob below. Shared by the slot cards and the editor. */
    void placeKnobIn (Knob& knob, juce::Rectangle<int> cell)
    {
        knob.label.setBounds (cell.removeFromTop (13));
        knob.slider.setBounds (cell.reduced (1, 0));
    }
}

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

    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff141416));
    setColour (juce::PopupMenu::textColourId,       juce::Colour (textColour));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff2a2a2e));
    setColour (juce::PopupMenu::highlightedTextColourId,       juce::Colour (textColour));

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
    g.setFont (juce::Font (juce::FontOptions (9.0f).withKerningFactor (0.16f)));
    g.setColour (juce::Colour (textColour).withAlpha (shouldDrawButtonAsDown ? 0.7f : 1.0f));
    g.drawText (button.getButtonText(), button.getLocalBounds(),
                juce::Justification::centred, false);
}

//==============================================================================
// Waveform strip
//==============================================================================
WaveformView::WaveformView()
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void WaveformView::setSample (std::shared_ptr<const LoadedSample> newSample)
{
    if (sample != newSample)
    {
        sample = std::move (newSample);
        repaint();
    }
}

void WaveformView::setTrim (float startProportion, float endProportion)
{
    if (startProportion != this->startProportion || endProportion != this->endProportion)
    {
        this->startProportion = juce::jlimit (0.0f, 1.0f, startProportion);
        this->endProportion = juce::jlimit (0.0f, 1.0f, endProportion);
        repaint();
    }
}

void WaveformView::setPlaying (bool shouldBePlaying)
{
    if (playing != shouldBePlaying)
    {
        playing = shouldBePlaying;
        repaint();
    }
}

void WaveformView::setEmptyMessage (const juce::String& text)
{
    if (emptyMessage != text)
    {
        emptyMessage = text;
        repaint();
    }
}

void WaveformView::mouseDown (const juce::MouseEvent&)
{
    if (onAudition != nullptr)
        onAudition();
}

void WaveformView::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (0.5f);

    g.setColour (juce::Colour (0xff0f0f12));
    g.fillRoundedRectangle (bounds, 2.0f);

    g.setColour (juce::Colour (playing ? Koola10LookAndFeel::accentColour
                                       : Koola10LookAndFeel::hairlineColour));
    g.drawRoundedRectangle (bounds, 2.0f, 1.0f);

    const auto strip = bounds.reduced (3.0f);

    if (sample == nullptr || sample->getLength() < 2)
    {
        g.setColour (juce::Colour (Koola10LookAndFeel::dimTextColour).withAlpha (0.8f));
        g.setFont (juce::Font (juce::FontOptions (8.5f).withKerningFactor (0.16f)));
        g.drawText (emptyMessage, getLocalBounds(), juce::Justification::centred, false);
        return;
    }

    const int width = juce::jmax (1, (int) strip.getWidth());
    const int length = sample->getLength();
    const auto* left = sample->buffer.getReadPointer (0);
    const auto* right = sample->buffer.getNumChannels() > 1 ? sample->buffer.getReadPointer (1)
                                                            : nullptr;

    const float midY = strip.getCentreY();
    const float halfHeight = strip.getHeight() * 0.46f;

    g.setColour (juce::Colour (Koola10LookAndFeel::textColour).withAlpha (0.72f));

    for (int x = 0; x < width; ++x)
    {
        const int from = juce::jlimit (0, length - 1, (int) ((juce::int64) x * length / width));
        const int to = juce::jlimit (from + 1, length,
                                     (int) ((juce::int64) (x + 1) * length / width));

        float peak = 0.0f;

        for (int i = from; i < to; ++i)
        {
            peak = juce::jmax (peak, std::abs (left[i]));

            if (right != nullptr)
                peak = juce::jmax (peak, std::abs (right[i]));
        }

        const float height = juce::jlimit (1.0f, halfHeight, peak * halfHeight);
        g.fillRect (strip.getX() + (float) x, midY - height, 1.0f, height * 2.0f);
    }

    // Everything outside START .. END is dimmed, so the trimmed part is obvious
    // without opening another editor.
    const float startX = strip.getX() + startProportion * strip.getWidth();
    const float endX = strip.getX() + endProportion * strip.getWidth();

    g.setColour (juce::Colour (0xcc0b0b0c));

    if (startX > strip.getX())
        g.fillRect (juce::Rectangle<float> (strip.getX(), strip.getY(),
                                            startX - strip.getX(), strip.getHeight()));

    if (endX < strip.getRight())
        g.fillRect (juce::Rectangle<float> (endX, strip.getY(),
                                            strip.getRight() - endX, strip.getHeight()));

    g.setColour (juce::Colour (Koola10LookAndFeel::accentColour).withAlpha (0.85f));
    g.fillRect (startX, strip.getY(), 1.0f, strip.getHeight());
    g.fillRect (endX - 1.0f, strip.getY(), 1.0f, strip.getHeight());
}

//==============================================================================
// One slot
//==============================================================================
SlotCard::SlotCard (Koola10SamplerAudioProcessor& processor, int slotIndex)
    : processorRef (processor), slot (slotIndex)
{
    addAndMakeVisible (waveform);
    waveform.onAudition = [this] { processorRef.getEngine().requestAudition (slot); };

    loadButton.setTooltip ("Load a WAV or MP3 into this slot");
    loadButton.onClick = [this] { chooseFile(); };
    addAndMakeVisible (loadButton);

    keysButton.setTooltip ("The piano roll plays this slot");
    keysButton.onClick = [this]
    {
        processorRef.setPlaySlot (slot);
        refresh();
    };
    addAndMakeVisible (keysButton);

    modeBox.setTooltip ("One-Shot plays to the end. Loop repeats between Start and End while the note is held.");

    // The attachment in JUCE 8 maps the parameter's position onto the box's
    // item list, but it does not fill the list in, so the two choices are added
    // here in the same order as the parameter's.
    modeBox.addItemList ({ "One-Shot", "Loop" }, 1);
    addAndMakeVisible (modeBox);

    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processorRef.getValueTreeState(), ParamIDs::slotModes[slot], modeBox);

    for (int kind = 0; kind < 6; ++kind)
        configureKnob (knobs[(size_t) kind], slotKnobLabels[kind], ParamIDs::slotIds[slot][kind], 54);

    refresh();
}

void SlotCard::configureKnob (Knob& knob, const juce::String& labelText,
                             const juce::String& parameterID, int textBoxWidth)
{
    knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, textBoxWidth, 14);
    knob.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                     juce::MathConstants<float>::pi * 2.75f, true);
    addAndMakeVisible (knob.slider);

    knob.label.setText (labelText, juce::dontSendNotification);
    knob.label.setJustificationType (juce::Justification::centred);
    knob.label.setFont (juce::Font (juce::FontOptions (9.0f).withKerningFactor (0.10f)));
    knob.label.setColour (juce::Label::textColourId, juce::Colour (Koola10LookAndFeel::dimTextColour));
    knob.label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (knob.label);

    knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorRef.getValueTreeState(), parameterID, knob.slider);
}

void SlotCard::resized()
{
    auto area = getLocalBounds().reduced (10, 8);

    auto top = area.removeFromTop (18);

    auto right = top;
    loadButton.setBounds (right.removeFromRight (46).reduced (0, 1));
    right.removeFromRight (4);
    modeBox.setBounds (right.removeFromRight (84).reduced (0, 1));
    right.removeFromRight (4);
    keysButton.setBounds (right.removeFromRight (44).reduced (0, 1));
    right.removeFromRight (10);
    titleArea = right;

    area.removeFromTop (2);
    waveform.setBounds (area.removeFromTop (38));
    area.removeFromTop (4);

    const int knobWidth = juce::jmax (40, area.getWidth() / 6);

    for (int kind = 0; kind < 6; ++kind)
        placeKnobIn (knobs[(size_t) kind], area.removeFromLeft (knobWidth));
}

void SlotCard::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (0.5f);

    g.setColour (juce::Colour (Koola10LookAndFeel::panelColour));
    g.fillRoundedRectangle (bounds, 3.0f);

    // The card with the keys gets a brighter frame, so it is obvious at a
    // glance which slot the piano roll is playing.
    g.setColour (juce::Colour (keysHere ? Koola10LookAndFeel::accentColour
                                        : Koola10LookAndFeel::hairlineColour));
    g.drawRoundedRectangle (bounds, 3.0f, keysHere ? 1.4f : 1.0f);

    g.setColour (juce::Colour (Koola10LookAndFeel::textColour));
    g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold).withKerningFactor (0.14f)));
    g.drawText (SamplerSlots::names[slot], titleArea, juce::Justification::centredLeft, false);

    const auto nameArea = titleArea.withTrimmedLeft (50).withTrimmedRight (4);

    if (fileMissing)
        g.setColour (juce::Colour (0xffb98080));
    else
        g.setColour (juce::Colour (Koola10LookAndFeel::dimTextColour));

    g.setFont (juce::Font (juce::FontOptions (9.0f)));

    const auto text = fileName.isEmpty() ? juce::String ("EMPTY  \xc2\xb7  CLICK LOAD")
                                         : fileName;

    g.drawFittedText (text, nameArea, juce::Justification::centredLeft, 1, 0.85f);
}

void SlotCard::chooseFile()
{
    chooser = std::make_unique<juce::FileChooser> ("Load a sample into " + juce::String (SamplerSlots::names[slot]),
                                                   juce::File::getSpecialLocation (juce::File::userMusicDirectory),
                                                   "*.wav;*.mp3");

    const auto flags = juce::FileBrowserComponent::openMode
                     | juce::FileBrowserComponent::canSelectFiles;

    // The panel can be closed while the browser is still open, so the callback
    // holds a safe pointer rather than a raw this.
    const juce::Component::SafePointer<SlotCard> safeThis (this);

    chooser->launchAsync (flags, [safeThis] (const juce::FileChooser& chooserResult)
    {
        if (safeThis == nullptr)
            return;

        const auto file = chooserResult.getResult();

        if (file != juce::File())
        {
            safeThis->processorRef.loadSlot (safeThis->slot, file);
            safeThis->refresh();
        }
    });
}

void SlotCard::refresh()
{
    const auto info = processorRef.getSlotInfo (slot);

    bool changed = false;

    if (info.name != fileName)
    {
        fileName = info.name;
        changed = true;
    }

    if (info.missing != fileMissing)
    {
        fileMissing = info.missing;
        changed = true;
    }

    waveform.setEmptyMessage (fileMissing ? "FILE NOT FOUND  \xc2\xb7  CLICK LOAD"
                                          : "NO SAMPLE LOADED  \xc2\xb7  CLICK LOAD");
    waveform.setSample (processorRef.getSlotSample (slot));

    const auto& state = processorRef.getValueTreeState();

    float start = 0.0f, end = 1.0f;

    if (auto* value = state.getRawParameterValue (ParamIDs::slotIds[slot][4]))
        start = value->load();

    if (auto* value = state.getRawParameterValue (ParamIDs::slotIds[slot][5]))
        end = value->load();

    if (start != lastStart || end != lastEnd)
    {
        lastStart = start;
        lastEnd = end;
        changed = true;
    }

    waveform.setTrim (start, end);

    const bool nowPlaying = processorRef.getEngine().getSlotActivity (slot) > 0.02f;

    if (nowPlaying != playing)
    {
        playing = nowPlaying;
        changed = true;
    }

    waveform.setPlaying (playing);

    const bool nowKeys = processorRef.getPlaySlot() == slot;

    if (nowKeys != keysHere)
    {
        keysHere = nowKeys;
        changed = true;
    }

    if (changed)
        repaint();
}

//==============================================================================
// The editor
//==============================================================================
Koola10SamplerAudioProcessorEditor::Koola10SamplerAudioProcessorEditor (Koola10SamplerAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel (&lookAndFeel);

    for (int slot = 0; slot < SamplerSlots::numSlots; ++slot)
        addAndMakeVisible (cards.add (new SlotCard (processorRef, slot)));

    configureKnob (cutoffKnob,    "FILTER CUTOFF", ParamIDs::filterCutoff,    78);
    configureKnob (resonanceKnob, "RESONANCE",     ParamIDs::filterResonance, 60);
    configureKnob (masterKnob,    "MASTER LEVEL",  ParamIDs::masterLevel,     60);

    voicesLine = "VOICES  0 / 16";
    keysLine = "KEYS PLAY  SLOT 1";

    setResizable (false, false);
    setSize (1240, 820);

    startTimerHz (12);
}

Koola10SamplerAudioProcessorEditor::~Koola10SamplerAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void Koola10SamplerAudioProcessorEditor::configureKnob (Knob& knob, const juce::String& labelText,
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

void Koola10SamplerAudioProcessorEditor::placeKnob (Knob& knob, juce::Rectangle<int> cell) const
{
    placeKnobIn (knob, cell);
}

void Koola10SamplerAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (28, 24);

    headerArea = area.removeFromTop (58);
    area.removeFromTop (10);

    slotsCaptionArea = area.removeFromTop (16);
    area.removeFromTop (8);

    auto sharedColumn = area.removeFromRight (300);
    area.removeFromRight (24);
    slotsArea = area;

    sharedCaptionArea = sharedColumn.removeFromTop (16);
    sharedColumn.removeFromTop (12);
    sharedArea = sharedColumn;

    // --- Eight cards: two columns, four rows ------------------------------
    constexpr int gapX = 16;
    constexpr int gapY = 12;

    const int cardWidth = (slotsArea.getWidth() - gapX) / 2;
    const int cardHeight = (slotsArea.getHeight() - gapY * 3) / 4;

    for (int slot = 0; slot < SamplerSlots::numSlots; ++slot)
    {
        const int column = slot % 2;
        const int row = slot / 2;

        cards[slot]->setBounds (slotsArea.getX() + column * (cardWidth + gapX),
                                slotsArea.getY() + row * (cardHeight + gapY),
                                cardWidth, cardHeight);
    }

    // --- Shared knobs: cutoff + resonance, then master --------------------
    auto shared = sharedArea;

    auto firstRow = shared.removeFromTop (104);
    placeKnob (cutoffKnob, firstRow.removeFromLeft (firstRow.getWidth() / 2));
    placeKnob (resonanceKnob, firstRow);

    shared.removeFromTop (12);
    placeKnob (masterKnob, shared.removeFromTop (104).withSizeKeepingCentre (150, 104));

    shared.removeFromTop (20);
    infoArea = shared;
}

void Koola10SamplerAudioProcessorEditor::timerCallback()
{
    for (auto* card : cards)
        card->refresh();

    const auto voices = "VOICES  " + juce::String (processorRef.getEngine().getActiveVoiceCount()) + " / 16";
    const auto keys = "KEYS PLAY  SLOT " + juce::String (processorRef.getPlaySlot() + 1);

    if (voices != voicesLine || keys != keysLine)
    {
        voicesLine = voices;
        keysLine = keys;
        repaint (infoArea);
    }
}

void Koola10SamplerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (Koola10LookAndFeel::backgroundColour));

    // --- Header -----------------------------------------------------------
    g.setColour (juce::Colour (Koola10LookAndFeel::textColour));
    g.setFont (juce::Font (juce::FontOptions (22.0f, juce::Font::bold)));
    g.drawText ("KOOLA10 SAMPLER", headerArea.withHeight (34),
                juce::Justification::centredLeft, false);

    g.setColour (juce::Colour (Koola10LookAndFeel::dimTextColour));
    g.setFont (juce::Font (juce::FontOptions (10.0f).withKerningFactor (0.24f)));
    g.drawText ("SAMPLE PLAYER  \xc2\xb7  V1  \xc2\xb7  WAV + MP3",
                headerArea.withTop (headerArea.getY() + 32).withHeight (18),
                juce::Justification::centredLeft, false);

    g.setColour (juce::Colour (Koola10LookAndFeel::hairlineColour));
    g.fillRect (headerArea.getX(), headerArea.getBottom() + 6, headerArea.getWidth(), 1);

    // --- Section captions -------------------------------------------------
    g.setColour (juce::Colour (Koola10LookAndFeel::dimTextColour));
    g.setFont (juce::Font (juce::FontOptions (9.5f).withKerningFactor (0.24f)));
    g.drawText ("SLOTS  \xc2\xb7  EIGHT SAMPLES", slotsCaptionArea,
                juce::Justification::centredLeft, false);
    g.drawText ("SHARED", sharedCaptionArea, juce::Justification::centredLeft, false);

    // --- Divider between the slots and the shared column ------------------
    const int dividerX = sharedCaptionArea.getX() - 12;
    g.fillRect (dividerX, slotsCaptionArea.getY() - 2, 1,
                slotsArea.getBottom() - slotsCaptionArea.getY() + 2);

    // --- Shared column readouts -------------------------------------------
    if (infoArea.isEmpty())
        return;

    g.setColour (juce::Colour (Koola10LookAndFeel::hairlineColour));
    g.fillRect (infoArea.getX(), infoArea.getY() - 12, infoArea.getWidth(), 1);

    auto text = infoArea;
    text.removeFromTop (8);

    g.setFont (juce::Font (juce::FontOptions (9.5f).withKerningFactor (0.16f)));
    g.setColour (juce::Colour (Koola10LookAndFeel::textColour).withAlpha (0.85f));

    g.drawText (voicesLine, text.removeFromTop (16), juce::Justification::centredLeft, false);

    g.setColour (juce::Colour (Koola10LookAndFeel::dimTextColour));
    g.drawText ("ROOT NOTE  C3  \xc2\xb7  MIDI 60", text.removeFromTop (14),
                juce::Justification::centredLeft, false);
    g.drawText (keysLine, text.removeFromTop (14), juce::Justification::centredLeft, false);

    text.removeFromTop (10);

    g.setColour (juce::Colour (Koola10LookAndFeel::dimTextColour).withAlpha (0.8f));
    g.setFont (juce::Font (juce::FontOptions (9.0f).withKerningFactor (0.04f)));

    g.drawFittedText ("LOAD puts a WAV or MP3 in a slot. Click the waveform to hear it "
                      "at the root note, and KEYS to hand the piano roll to that slot.",
                      text.removeFromTop (58), juce::Justification::topLeft, 4, 0.85f);

    g.drawFittedText ("TUNE is the pitch of the whole sound: 0 % is two octaves down, "
                      "50 % is the file as recorded, 100 % is two octaves up.",
                      text.removeFromTop (58), juce::Justification::topLeft, 4, 0.85f);

    g.drawFittedText ("LOOP repeats between START and END while the note is held, so a "
                      "trimmed loop has no click at the seam. ONE-SHOT always plays to the end.",
                      text.removeFromTop (58), juce::Justification::topLeft, 4, 0.85f);
}
