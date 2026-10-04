#include "PluginEditor.h"

#include "FactoryWaves.h"
#include "Presets.h"

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

    setColour (juce::ComboBox::backgroundColourId,          juce::Colour (0xff0f0f11));
    setColour (juce::ComboBox::textColourId,                juce::Colour (textColour));
    setColour (juce::ComboBox::outlineColourId,             juce::Colour (hairlineColour));
    setColour (juce::ComboBox::arrowColourId,               juce::Colour (dimTextColour));
    setColour (juce::ComboBox::focusedOutlineColourId,      juce::Colour (accentColour).withAlpha (0.45f));

    setColour (juce::PopupMenu::backgroundColourId,            juce::Colour (panelColour));
    setColour (juce::PopupMenu::textColourId,                  juce::Colour (textColour));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (accentColour).withAlpha (0.16f));
    setColour (juce::PopupMenu::highlightedTextColourId,       juce::Colour (textColour));

    setColour (juce::TextEditor::backgroundColourId,   juce::Colour (0xff0f0f11));
    setColour (juce::TextEditor::textColourId,         juce::Colour (textColour));
    setColour (juce::TextEditor::outlineColourId,      juce::Colour (hairlineColour));
    setColour (juce::TextEditor::focusedOutlineColourId, juce::Colour (accentColour).withAlpha (0.45f));
    setColour (juce::TextEditor::highlightColourId,    juce::Colour (accentColour).withAlpha (0.22f));
    setColour (juce::CaretComponent::caretColourId,    juce::Colour (accentColour));

    setColour (juce::ListBox::backgroundColourId,      juce::Colours::transparentBlack);
    setColour (juce::ListBox::outlineColourId,         juce::Colours::transparentBlack);
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
    g.setFont (juce::Font (juce::FontOptions (10.0f).withKerningFactor (0.18f)));
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
// The wave browser
//==============================================================================
WaveBrowser::WaveBrowser (Koola10SynthAudioProcessor& p)
    : processorRef (p)
{
    presetBox.setTextWhenNothingSelected ("Preset");
    presetBox.addSectionHeading ("Factory presets");

    for (int i = 0; i < Presets::getNumFactoryPresets(); ++i)
        presetBox.addItem (Presets::getFactoryPreset (i).name, i + 1);

    presetBox.onChange = [this] { applySelectedPreset(); };
    addAndMakeVisible (presetBox);

    savePresetButton.onClick = [this] { savePresetFile(); };
    addAndMakeVisible (savePresetButton);

    loadPresetButton.onClick = [this] { loadPresetFile(); };
    addAndMakeVisible (loadPresetButton);

    search.setTextToShowWhenEmpty ("Search waves", juce::Colour (Koola10LookAndFeel::dimTextColour));
    search.setFont (juce::Font (juce::FontOptions (12.0f)));
    search.onTextChange = [this] { rebuildRows(); };
    addAndMakeVisible (search);

    list.setModel (this);
    list.setRowHeight (22);
    list.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    list.setColour (juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (list);

    rebuildRows();
}

WaveBrowser::~WaveBrowser()
{
    list.setModel (nullptr);
}

void WaveBrowser::paint (juce::Graphics& g)
{
    g.setColour (juce::Colour (Koola10LookAndFeel::panelColour));
    g.fillRect (getLocalBounds());

    g.setColour (juce::Colour (Koola10LookAndFeel::dimTextColour));
    g.setFont (juce::Font (juce::FontOptions (9.5f).withKerningFactor (0.22f)));
    g.drawText ("PRESET  /  BROWSER", captionArea, juce::Justification::centredLeft, false);
}

void WaveBrowser::resized()
{
    auto area = getLocalBounds().reduced (8, 8);

    captionArea = area.removeFromTop (14);

    presetBox.setBounds (area.removeFromTop (26));
    area.removeFromTop (6);

    auto buttonRow = area.removeFromTop (24);
    const int halfButton = (buttonRow.getWidth() - 6) / 2;
    savePresetButton.setBounds (buttonRow.removeFromLeft (halfButton));
    buttonRow.removeFromLeft (6);
    loadPresetButton.setBounds (buttonRow);

    area.removeFromTop (12);
    search.setBounds (area.removeFromTop (26));
    area.removeFromTop (8);
    list.setBounds (area);
}

void WaveBrowser::refresh()
{
    rebuildRows();
}

void WaveBrowser::rebuildRows()
{
    rows.clear();

    const juce::String filter = search.getText().trim();

    // Factory waves, grouped in category order.
    for (int c = 0; c < FactoryWaves::getNumCategories(); ++c)
    {
        const juce::String category (FactoryWaves::getCategoryName (c));
        bool headerAdded = false;

        for (int i = 0; i < FactoryWaves::getNumWaves(); ++i)
        {
            const auto& wave = FactoryWaves::getWave (i);

            if (category != wave.category)
                continue;

            if (filter.isNotEmpty() && ! juce::String (wave.name).containsIgnoreCase (filter))
                continue;

            if (! headerAdded)
            {
                rows.push_back ({ true, category, -1, -1 });
                headerAdded = true;
            }

            rows.push_back ({ false, juce::String (wave.name), i, -1 });
        }
    }

    // The "User" group: .wav files loaded this session.
    {
        const auto& files = processorRef.getUserWaveFiles();
        bool headerAdded = false;

        for (int i = 0; i < files.size(); ++i)
        {
            const auto name = files.getReference (i).getFileNameWithoutExtension();

            if (filter.isNotEmpty() && ! name.containsIgnoreCase (filter))
                continue;

            if (! headerAdded)
            {
                rows.push_back ({ true, "User", -1, -1 });
                headerAdded = true;
            }

            rows.push_back ({ false, name, -1, i });
        }
    }

    list.updateContent();
    list.repaint();
    refreshSelection();
}

void WaveBrowser::refreshSelection()
{
    // --- Factory preset dropdown -----------------------------------------
    const int presetIndex = processorRef.getCurrentPresetIndex();
    const int wantedPresetId = presetIndex >= 0 ? presetIndex + 1 : 0;

    if (presetBox.getSelectedId() != wantedPresetId)
        presetBox.setSelectedId (wantedPresetId, juce::dontSendNotification);

    // --- Currently loaded wave -------------------------------------------
    const juce::String currentName = processorRef.getEngine().getWavetableName();
    const juce::String currentPath = processorRef.getCurrentWavetableFile().getFullPathName();

    int match = -1;

    for (int i = 0; i < static_cast<int> (rows.size()); ++i)
    {
        const auto& row = rows[static_cast<size_t> (i)];

        if (row.isHeader)
            continue;

        bool isMatch = false;

        if (row.factoryWaveIndex >= 0)
        {
            isMatch = currentName == FactoryWaves::getWave (row.factoryWaveIndex).name;
        }
        else if (row.userFileIndex >= 0 && currentPath.isNotEmpty())
        {
            const auto& file = processorRef.getUserWaveFiles()[row.userFileIndex];
            isMatch = file.getFullPathName() == currentPath;
        }

        if (isMatch)
        {
            match = i;
            break;
        }
    }

    if (match >= 0)
    {
        if (list.getSelectedRow() != match)
            list.selectRow (match, true, true);
    }
    else if (list.getSelectedRow() >= 0)
    {
        list.deselectAllRows();
    }
}

int WaveBrowser::getNumRows()
{
    return static_cast<int> (rows.size());
}

void WaveBrowser::paintListBoxItem (int rowNumber, juce::Graphics& g, int width, int height,
                                    bool rowIsSelected)
{
    if (! juce::isPositiveAndBelow (rowNumber, static_cast<int> (rows.size())))
        return;

    const auto& row = rows[static_cast<size_t> (rowNumber)];
    const auto bounds = juce::Rectangle<int> (0, 0, width, height);

    if (row.isHeader)
    {
        g.setColour (juce::Colour (Koola10LookAndFeel::dimTextColour));
        g.setFont (juce::Font (juce::FontOptions (9.5f).withKerningFactor (0.20f)));
        g.drawText (row.text.toUpperCase(), bounds.reduced (8, 0),
                    juce::Justification::centredLeft, false);
        return;
    }

    if (rowIsSelected)
    {
        g.setColour (juce::Colour (Koola10LookAndFeel::accentColour).withAlpha (0.14f));
        g.fillRect (bounds);
    }

    g.setColour (rowIsSelected ? juce::Colour (Koola10LookAndFeel::textColour)
                               : juce::Colour (Koola10LookAndFeel::dimTextColour).brighter (0.35f));
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawText (row.text, bounds.reduced (18, 0), juce::Justification::centredLeft, false);
}

void WaveBrowser::listBoxItemClicked (int row, const juce::MouseEvent&)
{
    if (juce::isPositiveAndBelow (row, static_cast<int> (rows.size())))
        loadRow (rows[static_cast<size_t> (row)]);
}

void WaveBrowser::loadRow (const Row& row)
{
    if (row.isHeader)
        return;

    if (row.factoryWaveIndex >= 0)
    {
        processorRef.loadFactoryWave (row.factoryWaveIndex);
    }
    else if (row.userFileIndex >= 0)
    {
        const auto& files = processorRef.getUserWaveFiles();

        if (juce::isPositiveAndBelow (row.userFileIndex, files.size()))
            processorRef.loadWavetableFromFile (files[row.userFileIndex]);
    }
}

void WaveBrowser::applySelectedPreset()
{
    const int id = presetBox.getSelectedId();

    if (id > 0)
        processorRef.applyFactoryPreset (id - 1);
}

void WaveBrowser::savePresetFile()
{
    presetFileChooser = std::make_unique<juce::FileChooser> (
        "Save preset",
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
        "*.koola10preset");

    const auto flags = juce::FileBrowserComponent::saveMode
                     | juce::FileBrowserComponent::canSelectFiles
                     | juce::FileBrowserComponent::warnAboutOverwriting;

    presetFileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser)
    {
        auto file = chooser.getResult();

        if (file.getFullPathName().isEmpty())
            return;

        if (! file.hasFileExtension ("koola10preset"))
            file = file.withFileExtension ("koola10preset");

        processorRef.savePresetToFile (file);
    });
}

void WaveBrowser::loadPresetFile()
{
    presetFileChooser = std::make_unique<juce::FileChooser> (
        "Load preset",
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
        "*.koola10preset");

    const auto flags = juce::FileBrowserComponent::openMode
                     | juce::FileBrowserComponent::canSelectFiles;

    presetFileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser)
    {
        const auto file = chooser.getResult();

        if (! file.existsAsFile())
            return;

        processorRef.loadPresetFromFile (file);
        refreshSelection();
    });
}

//==============================================================================
// The editor
//==============================================================================
Koola10SynthAudioProcessorEditor::Koola10SynthAudioProcessorEditor (Koola10SynthAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      processorRef (p),
      display (p.getEngine()),
      browser (p)
{
    setLookAndFeel (&lookAndFeel);

    addAndMakeVisible (display);
    addAndMakeVisible (browser);

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

    refreshWavetableHeader();
    browser.refreshSelection();

    lastWaveName = processorRef.getEngine().getWavetableName();
    lastUserWaveCount = processorRef.getUserWaveFiles().size();

    setResizable (false, false);
    setSize (1180, 560);

    startTimerHz (12);
}

Koola10SynthAudioProcessorEditor::~Koola10SynthAudioProcessorEditor()
{
    stopTimer();
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
    knob.label.setFont (juce::Font (juce::FontOptions (10.0f).withKerningFactor (0.12f)));
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

void Koola10SynthAudioProcessorEditor::refreshWavetableHeader()
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
            refreshWavetableHeader();
            browser.refresh();
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

void Koola10SynthAudioProcessorEditor::timerCallback()
{
    const juce::String name = processorRef.getEngine().getWavetableName();
    const int userCount = processorRef.getUserWaveFiles().size();

    if (name != lastWaveName || userCount != lastUserWaveCount)
    {
        lastWaveName = name;
        lastUserWaveCount = userCount;
        refreshWavetableHeader();
        browser.refresh();
    }

    browser.refreshSelection();
}

//==============================================================================
void Koola10SynthAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (28, 24);

    headerArea = area.removeFromTop (64);
    area.removeFromTop (18);

    // Slim browser sidebar on the left.
    browserPanelArea = area.removeFromLeft (210);
    browser.setBounds (browserPanelArea);
    area.removeFromLeft (22);

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
    g.setFont (juce::Font (juce::FontOptions (10.0f).withKerningFactor (0.24f)));
    g.drawText ("WAVETABLE SYNTHESIZER  \xc2\xb7  V2",
                headerArea.withTop (headerArea.getY() + 34).withHeight (20),
                juce::Justification::centredLeft, false);

    // Hairline under the header.
    g.setColour (juce::Colour (Koola10LookAndFeel::hairlineColour));
    g.fillRect (headerArea.getX(), headerArea.getBottom() + 9, headerArea.getWidth(), 1);

    // Hairline to the right of the browser panel.
    g.fillRect (browserPanelArea.getRight() + 8, browserPanelArea.getY(),
                1, browserPanelArea.getHeight());

    // --- Wavetable display frame -------------------------------------------
    g.drawRect (displayFrameArea, 1);

    // --- Section titles ----------------------------------------------------
    static const char* const sectionTitles[numSections] = { "OSCILLATOR", "FILTER", "AMPLITUDE", "OUTPUT" };

    g.setColour (juce::Colour (Koola10LookAndFeel::dimTextColour));
    g.setFont (juce::Font (juce::FontOptions (10.0f).withKerningFactor (0.24f)));

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
