#include "PluginEditor.h"
#include <limits>

namespace
{
const juce::StringArray noiseNames {
    "White Noise", "Pink Noise", "Vinyl 1930", "Vinyl 1950", "Vinyl 1960",
    "Mechanical", "Electrical"
};
constexpr const char* effectParameterIds[] = {
    "pitchOn", "phaserOn", "autoFilterOn", "chorusOn", "flangerOn",
    "modulationOn", "autoPanOn", "delayOn", "stereoShaperOn", "compressorOn"
};
constexpr const char* effectLabels[] = {
    "Pitch Shifter", "Phaser", "Auto Filter", "Chorus", "Flanger",
    "Modulation", "Auto Pan", "Delay", "Stereo Shaper", "Glue Comp"
};

// Design space of background.svg; every control is placed in these units and then scaled to the panel.
constexpr float designWidth = 960.0f;
constexpr float designHeight = 1076.0f;
constexpr float layerRowTop = 78.0f;
constexpr float layerRowPitch = 58.0f;
constexpr float layerColumnX[] { 288.0f, 504.0f, 720.0f };
constexpr float layerColumnWidth = 208.0f;
constexpr float effectCellPitch = 182.0f;
constexpr float effectCellWidth = 172.0f;
constexpr float effectRowTop[] { 318.0f, 500.0f };
constexpr float gateControlX = 748.0f;
constexpr float gateControlWidth = 180.0f;
constexpr float gateControlTop = 810.0f;
constexpr float gateControlPitch = 44.0f;

/** Fits the whole 960 x 1076 design into the available area, centred, so nothing is ever clipped. */
struct FittedDesign
{
    float scale = 1.0f;
    float originX = 0.0f;
    float originY = 0.0f;
};

FittedDesign fitDesign (float width, float height)
{
    const auto scale = juce::jmin (width / designWidth, height / designHeight);
    return { scale,
             (width - designWidth * scale) * 0.5f,
             (height - designHeight * scale) * 0.5f };
}
}

MKNoiseRhythmMachineEditor::MKNoiseRhythmMachineEditor (MKNoiseRhythmMachineProcessor& p)
    : AudioProcessorEditor (&p), processor (p), gateGrid (p)
{
    setLookAndFeel (&analogLookAndFeel);

#if JUCE_IOS
    // Fill the available screen so the panel can scale to fit either orientation without clipping.
    auto screenArea = juce::Rectangle<int> { 0, 0, 1112, 834 };
    if (auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        screenArea = display->userArea;
    setSize (juce::jmax (320, screenArea.getWidth()), juce::jmax (320, screenArea.getHeight()));
#else
    setSize (900, 1009);
#endif

    title.setText ("MK NOISE RhYTM Machine", juce::dontSendNotification);
    title.setColour (juce::Label::textColourId, mkui::colours::textBright);
    title.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (title);

    subtitle.setText ("THREE-LAYER NOISE INSTRUMENT  |  TEMPO-SYNCED TRANCE GATE",
                      juce::dontSendNotification);
    subtitle.setColour (juce::Label::textColourId, mkui::colours::textDim);
    addAndMakeVisible (subtitle);

    const auto addHeading = [this] (juce::Label& label, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setColour (juce::Label::textColourId, mkui::colours::text);
        addAndMakeVisible (label);
    };
    addHeading (layerHeading, "NOISE LAYERS");
    layerHeading.setJustificationType (juce::Justification::centredRight);
    addHeading (effectsHeading, "EFFECTS  |  GATE IS ALWAYS LAST");
    addHeading (gateHeading, "GATE12 TRANCE GATE  |  DRAG CURVE POINTS");
    const juce::StringArray noiseControlNames { "Level", "Pan", "Pitch" };
    for (size_t i = 0; i < noiseSliderLabels.size(); ++i)
    {
        addHeading (noiseSliderLabels[i], noiseControlNames[static_cast<int> (i)]);
        noiseSliderLabels[i].setColour (juce::Label::textColourId, mkui::colours::textDim);
    }

    for (int i = 0; i < 3; ++i)
    {
        const auto suffix = juce::String (i + 1);
        layerLabels[static_cast<size_t> (i)].setText ("LAYER " + suffix, juce::dontSendNotification);
        layerLabels[static_cast<size_t> (i)].setColour (juce::Label::textColourId, mkui::colours::textDim);
        addAndMakeVisible (layerLabels[static_cast<size_t> (i)]);

        voiceButtons[static_cast<size_t> (i)].setButtonText ("ON");
        voiceButtons[static_cast<size_t> (i)].setColour (juce::ToggleButton::textColourId,
                                                         mkui::colours::text);
        addAndMakeVisible (voiceButtons[static_cast<size_t> (i)]);
        voiceAttachments[static_cast<size_t> (i)] =
            std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
                processor.parameters, "voiceOn" + suffix, voiceButtons[static_cast<size_t> (i)]);

        for (int type = 0; type < noiseNames.size(); ++type)
            noiseTypes[static_cast<size_t> (i)].addItem (noiseNames[type], type + 1);
        addAndMakeVisible (noiseTypes[static_cast<size_t> (i)]);
        typeAttachments[static_cast<size_t> (i)] =
            std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
                processor.parameters, "voiceType" + suffix, noiseTypes[static_cast<size_t> (i)]);

        styleSlider (voiceLevels[static_cast<size_t> (i)]);
        styleSlider (voicePans[static_cast<size_t> (i)]);
        styleSlider (voiceTunes[static_cast<size_t> (i)]);
        voiceLevels[static_cast<size_t> (i)].setTextValueSuffix (" dB");
        voicePans[static_cast<size_t> (i)].setTextValueSuffix ("");
        voiceTunes[static_cast<size_t> (i)].setTextValueSuffix (" st");
        addAndMakeVisible (voiceLevels[static_cast<size_t> (i)]);
        addAndMakeVisible (voicePans[static_cast<size_t> (i)]);
        addAndMakeVisible (voiceTunes[static_cast<size_t> (i)]);
        levelAttachments[static_cast<size_t> (i)] =
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
                processor.parameters, "voiceGain" + suffix, voiceLevels[static_cast<size_t> (i)]);
        panAttachments[static_cast<size_t> (i)] =
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
                processor.parameters, "voicePan" + suffix, voicePans[static_cast<size_t> (i)]);
        tuneAttachments[static_cast<size_t> (i)] =
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
                processor.parameters, "voiceTune" + suffix, voiceTunes[static_cast<size_t> (i)]);
    }

    for (int i = 0; i < 10; ++i)
    {
        effectButtons[static_cast<size_t> (i)].setButtonText (effectLabels[i]);
        effectButtons[static_cast<size_t> (i)].setColour (juce::ToggleButton::textColourId,
                                                          mkui::colours::text);
        addAndMakeVisible (effectButtons[static_cast<size_t> (i)]);
        effectAttachments[static_cast<size_t> (i)] =
            std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
                processor.parameters, effectParameterIds[i], effectButtons[static_cast<size_t> (i)]);
        auto& knob = effectKnobs[static_cast<size_t> (i)];
        styleSlider (knob);
        knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 66, 20);
        knob.setTextValueSuffix (i == 0 ? " st" : "");
        knob.setName (juce::String (effectLabels[i]) + " Amount");
        addAndMakeVisible (knob);
        knobAttachments[static_cast<size_t> (i)] =
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
                processor.parameters, MKNoiseRhythmMachineProcessor::effectAmountParameterID (i), knob);
    }

    gateEnabled.setButtonText ("Gate On");
    gateEnabled.setColour (juce::ToggleButton::textColourId, mkui::colours::text);
    addAndMakeVisible (gateEnabled);
    gateEnabledAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.parameters, "gateOn", gateEnabled);
    disableEffects.setButtonText ("Disable All FX");
    disableEffects.setColour (juce::ToggleButton::textColourId, mkui::colours::text);
    disableEffects.onClick = [this]
    {
        for (const auto* id : effectParameterIds)
            if (auto* parameter = processor.parameters.getParameter (id))
                parameter->setValueNotifyingHost (0.0f);
        if (auto* parameter = processor.parameters.getParameter ("gateOn"))
            parameter->setValueNotifyingHost (0.0f);
    };
    addAndMakeVisible (disableEffects);

    for (int i = 0; i < processor.parameters.getParameter ("gatePreset")->getNumSteps(); ++i)
        gatePreset.addItem (processor.parameters.getParameter ("gatePreset")->getText (
                                static_cast<float> (i) / static_cast<float> (
                                    juce::jmax (1, processor.parameters.getParameter ("gatePreset")->getNumSteps() - 1)),
                                64), i + 1);
    gatePreset.setSelectedItemIndex (juce::roundToInt (
        processor.parameters.getRawParameterValue ("gatePreset")->load()), juce::dontSendNotification);
    gatePreset.onChange = [this]
    {
        if (gatePreset.getSelectedItemIndex() >= 0)
        {
            processor.applyGatePreset (gatePreset.getSelectedItemIndex());
            timerCallback();
        }
    };
    gatePreset.setViewportIgnoreDragFlag (true);
    addAndMakeVisible (gatePreset);

    styleSlider (gateSteps);
    styleSlider (gateAttack);
    styleSlider (gateRelease);
    styleSlider (gateTension);
    styleSlider (outputGain);
    styleSlider (gateSwing);
    gateSwing.setSliderStyle (juce::Slider::LinearVertical);
    gateSwing.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 20);
    gateSwing.setTextValueSuffix (" %");
    gateSwing.setName ("Gate Swing");
    gateSwing.setTooltip ("Sixteenth-note swing: 0% is straight; 100% delays the offbeat to 2:1 timing");
    addHeading (swingLabel, "Swing");
    const juce::StringArray gateControlNames {
        "Attack", "Release", "Curve Tension", "Output"
    };
    for (int i = 0; i < static_cast<int> (gateControlLabels.size()); ++i)
    {
        auto& label = gateControlLabels[static_cast<size_t> (i)];
        label.setText (gateControlNames[i], juce::dontSendNotification);
        label.setColour (juce::Label::textColourId, mkui::colours::textDim);
        addAndMakeVisible (label);
    }
    gateSteps.setTextValueSuffix (" pts");
    gateAttack.setTextValueSuffix (" ms");
    gateRelease.setTextValueSuffix (" ms");
    outputGain.setTextValueSuffix (" dB");
    gateSteps.setTooltip ("Number of editable curve points, from 8 to 32");
    gateAttack.setTooltip ("Gate envelope attack time");
    gateRelease.setTooltip ("Gate envelope release time");
    for (int i = 0; i < 3; ++i)
    {
        voiceLevels[static_cast<size_t> (i)].setTooltip ("Noise layer level");
        voicePans[static_cast<size_t> (i)].setTooltip ("Noise layer stereo pan");
        voiceTunes[static_cast<size_t> (i)].setTooltip ("Real audio pitch shifting, -24 to +24 semitones");
    }
    for (auto* slider : { &gateSteps, &gateAttack, &gateRelease, &gateTension, &outputGain, &gateSwing })
        addAndMakeVisible (*slider);

    gateSteps.setRange (8, 32, 1);
    gateStepsAttachment = std::make_unique<juce::ParameterAttachment> (
        *processor.parameters.getParameter ("gateSteps"),
        [this] (float value) { gateSteps.setValue (value, juce::dontSendNotification); }, nullptr);
    gateStepsAttachment->sendInitialUpdate();
    gateSteps.onValueChange = [this] { processor.setGatePointCount (juce::roundToInt (gateSteps.getValue())); };
    gateAttackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.parameters, "gateAttack", gateAttack);
    gateReleaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.parameters, "gateRelease", gateRelease);
    tensionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.parameters, "gateTension", gateTension);
    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.parameters, "outputGain", outputGain);
    swingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.parameters, "gateSwing", gateSwing);

    randomizeGate.setColour (juce::TextButton::buttonColourId, mkui::colours::switchBody);
    randomizeGate.onClick = [this] { processor.randomizeGate(); };
    addAndMakeVisible (randomizeGate);
    addAndMakeVisible (gateGrid);
    const auto populate = [this] (juce::ComboBox& box, const char* id)
    {
        auto* parameter = processor.parameters.getParameter (id);
        for (int i = 0; i < parameter->getNumSteps(); ++i)
            box.addItem (parameter->getText (parameter->convertTo0to1 (static_cast<float> (i)), 64), i + 1);
        addAndMakeVisible (box);
    };
    populate (gateCycle, "gateCycle");
    populate (gateShape, "gateShape");
    populate (gateStereo, "gateStereoMode");
    stereoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processor.parameters, "gateStereoMode", gateStereo);
    gateFilter.setColour (juce::ToggleButton::textColourId, mkui::colours::text);
    addAndMakeVisible (gateFilter);
    filterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.parameters, "gateFilterOn", gateFilter);
    presetDescription.setColour (juce::Label::textColourId, mkui::colours::textDim);
    presetDescription.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (presetDescription);
    cycleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processor.parameters, "gateCycle", gateCycle);
    shapeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processor.parameters, "gateShape", gateShape);
    gateCycle.setTooltip ("Length of one gate envelope cycle; quarter note equals one beat");
    gateShape.setTooltip ("Gate12 segment type; Curve with zero tension is linear");
    gateRetrigger.setColour (juce::ToggleButton::textColourId, mkui::colours::text);
    addAndMakeVisible (gateRetrigger);
    retriggerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.parameters, "gateRetrigger", gateRetrigger);
    gateRetrigger.setTooltip ("Restart the gate at each MIDI note-on; off follows host song position");
    while (getNumChildComponents() > 0)
        panel.addAndMakeVisible (getChildComponent (0));
    viewport.setViewedComponent (&panel, false);
    viewport.setScrollBarsShown (false, false);
    viewport.setScrollOnDragMode (juce::Viewport::ScrollOnDragMode::nonHover);
    addAndMakeVisible (viewport);

    // Added last so it stays on top of the scaled panel.
    setResizable (true, true);
    setResizeLimits (480, 400, juce::roundToInt (designWidth * 2.0f), juce::roundToInt (designHeight * 2.0f));

    resized();
    timerCallback();
    startTimerHz (30);
}

MKNoiseRhythmMachineEditor::~MKNoiseRhythmMachineEditor()
{
    setLookAndFeel (nullptr);
}

void MKNoiseRhythmMachineEditor::styleSlider (juce::Slider& slider)
{
    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setViewportIgnoreDragFlag (true);
    slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 62, 20);
}

void MKNoiseRhythmMachineEditor::applyMetrics (const UiScale& ui)
{
    title.setFont (ui.font (20.0f, true));
    subtitle.setFont (ui.font (8.5f));
    layerHeading.setFont (ui.font (10.0f, true));
    effectsHeading.setFont (ui.font (10.0f, true));
    gateHeading.setFont (ui.font (10.0f, true));
    swingLabel.setFont (ui.font (9.0f, true));
    presetDescription.setFont (ui.font (8.0f));

    for (auto& label : layerLabels)
        label.setFont (ui.font (8.5f, true));
    for (auto& label : noiseSliderLabels)
        label.setFont (ui.font (8.0f));
    for (auto& label : gateControlLabels)
        label.setFont (ui.font (8.0f));

    const auto readoutWidth = juce::jmax (44, ui.pixels (66.0f));
    const auto readoutHeight = juce::jmax (10, ui.pixels (20.0f));

    for (auto& slider : voiceLevels)
        slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, readoutWidth, readoutHeight);
    for (auto& slider : voicePans)
        slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, readoutWidth, readoutHeight);
    for (auto& slider : voiceTunes)
        slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, readoutWidth, readoutHeight);
    for (auto& slider : { &gateSteps, &gateAttack, &gateRelease, &gateTension, &outputGain })
        slider->setTextBoxStyle (juce::Slider::TextBoxRight, false, readoutWidth, readoutHeight);
    for (auto& knob : effectKnobs)
        knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, readoutWidth, readoutHeight);
    gateSwing.setTextBoxStyle (juce::Slider::TextBoxBelow, false,
                               juce::jmax (40, ui.pixels (64.0f)), readoutHeight);
}

void MKNoiseRhythmMachineEditor::paint (juce::Graphics& g)
{
    g.fillAll (mkui::colours::chassis);
}

void MKNoiseRhythmMachineEditor::Panel::paint (juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat();
    const auto fitted = fitDesign (area.getWidth(), area.getHeight());

    // Extend the chassis behind the letterboxed margins so any window shape looks finished.
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff191e19), area.getX(), area.getY(),
                                             juce::Colour (0xff0f120f), area.getX(), area.getBottom(), false));
    g.fillAll();

    const juce::Rectangle<float> designArea { fitted.originX, fitted.originY,
                                              designWidth * fitted.scale,
                                              designHeight * fitted.scale };
    mkui::AnalogLookAndFeel::drawDrawable (g, mkui::AnalogLookAndFeel::background(), designArea);

    g.setColour (mkui::colours::textDim.withAlpha (0.85f));
    g.setFont (juce::Font (juce::FontOptions (juce::jmax (8.0f, 7.5f * fitted.scale))));
    g.drawText ("Gate12 curve engine by tiagolr / tilr - GPLv3",
                juce::roundToInt (designArea.getX() + 380.0f * fitted.scale),
                juce::roundToInt (designArea.getY() + 1034.0f * fitted.scale),
                juce::roundToInt (520.0f * fitted.scale), juce::roundToInt (16.0f * fitted.scale),
                juce::Justification::centredRight);
}

void MKNoiseRhythmMachineEditor::resized()
{
    viewport.setBounds (getLocalBounds());

    // The panel always fills the window and the design is scaled to fit inside it, so every
    // control stays visible in any window shape (iPad portrait and landscape included).
    panel.setSize (juce::jmax (1, viewport.getWidth()), juce::jmax (1, viewport.getHeight()));

    const auto fitted = fitDesign ((float) panel.getWidth(), (float) panel.getHeight());
    const UiScale ui { fitted.scale, { fitted.originX, fitted.originY } };
    applyMetrics (ui);

    title.setBounds (ui.bounds (46.0f, 30.0f, 420.0f, 24.0f));
    subtitle.setBounds (ui.bounds (46.0f, 55.0f, 430.0f, 12.0f));
    layerHeading.setBounds (ui.bounds (528.0f, 33.0f, 388.0f, 16.0f));

    for (int column = 0; column < 3; ++column)
        noiseSliderLabels[static_cast<size_t> (column)].setBounds (
            ui.bounds (layerColumnX[column], 58.0f, layerColumnWidth, 12.0f));

    for (int i = 0; i < 3; ++i)
    {
        const auto y = layerRowTop + layerRowPitch * static_cast<float> (i);
        layerLabels[static_cast<size_t> (i)].setBounds (ui.bounds (32.0f, y + 8.0f, 48.0f, 16.0f));
        voiceButtons[static_cast<size_t> (i)].setBounds (ui.bounds (84.0f, y + 5.0f, 54.0f, 22.0f));
        noiseTypes[static_cast<size_t> (i)].setBounds (ui.bounds (142.0f, y + 3.0f, 142.0f, 26.0f));
        voiceLevels[static_cast<size_t> (i)].setBounds (ui.bounds (layerColumnX[0], y, layerColumnWidth, 30.0f));
        voicePans[static_cast<size_t> (i)].setBounds (ui.bounds (layerColumnX[1], y, layerColumnWidth, 30.0f));
        voiceTunes[static_cast<size_t> (i)].setBounds (ui.bounds (layerColumnX[2], y, layerColumnWidth, 30.0f));
    }

    effectsHeading.setBounds (ui.bounds (46.0f, 290.0f, 420.0f, 18.0f));
    disableEffects.setBounds (ui.bounds (752.0f, 288.0f, 168.0f, 26.0f));

    for (int i = 0; i < 10; ++i)
    {
        const auto column = i % 5;
        const auto row = i / 5;
        const auto x = 24.0f + effectCellPitch * static_cast<float> (column);
        const auto y = effectRowTop[row];
        effectButtons[static_cast<size_t> (i)].setBounds (ui.bounds (x, y, effectCellWidth, 22.0f));
        effectKnobs[static_cast<size_t> (i)].setBounds (
            ui.bounds (x + 8.0f, y + 26.0f, effectCellWidth - 16.0f, 138.0f));
    }

    gateHeading.setBounds (ui.bounds (48.0f, 710.0f, 400.0f, 18.0f));
    presetDescription.setBounds (ui.bounds (456.0f, 710.0f, 460.0f, 18.0f));
    gateEnabled.setBounds (ui.bounds (24.0f, 736.0f, 108.0f, 26.0f));
    gatePreset.setBounds (ui.bounds (138.0f, 734.0f, 594.0f, 28.0f));
    randomizeGate.setBounds (ui.bounds (740.0f, 734.0f, 164.0f, 28.0f));
    gateCycle.setBounds (ui.bounds (24.0f, 770.0f, 202.0f, 28.0f));
    gateShape.setBounds (ui.bounds (234.0f, 770.0f, 202.0f, 28.0f));
    gateRetrigger.setBounds (ui.bounds (444.0f, 770.0f, 210.0f, 28.0f));
    gateSteps.setBounds (ui.bounds (662.0f, 770.0f, 242.0f, 28.0f));
    swingLabel.setBounds (ui.bounds (24.0f, 806.0f, 90.0f, 14.0f));
    gateSwing.setBounds (ui.bounds (24.0f, 820.0f, 90.0f, 148.0f));
    gateGrid.setBounds (ui.bounds (124.0f, 812.0f, 612.0f, 164.0f));

    const std::array<juce::Component*, 4> controls { &gateAttack, &gateRelease, &gateTension, &outputGain };
    for (size_t i = 0; i < controls.size(); ++i)
    {
        const auto y = gateControlTop + gateControlPitch * static_cast<float> (i);
        gateControlLabels[i].setBounds (ui.bounds (gateControlX, y, gateControlWidth, 12.0f));
        controls[i]->setBounds (ui.bounds (gateControlX, y + 12.0f, gateControlWidth, 30.0f));
    }

    gateFilter.setBounds (ui.bounds (24.0f, 1004.0f, 152.0f, 26.0f));
    gateStereo.setBounds (ui.bounds (184.0f, 1002.0f, 200.0f, 28.0f));
}

void MKNoiseRhythmMachineEditor::GateGrid::paint (juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat();
    mkui::AnalogLookAndFeel::drawDrawable (g, mkui::AnalogLookAndFeel::displayScreen(), area);

    const auto steps = processor.getGateStepCount();
    const auto plot = plotBounds();
    std::array<float, 32> levels {}, offsets {};

    for (int step = 0; step < steps; ++step)
    {
        levels[static_cast<size_t> (step)] = processor.getGateStep (step);
        offsets[static_cast<size_t> (step)] = processor.getGatePointOffset (step);
    }

    g.setColour (mkui::colours::screenGrid.withAlpha (0.6f));
    for (int step = 0; step < steps; ++step)
    {
        const auto x = plot.getX() + plot.getWidth() * static_cast<float> (step) / static_cast<float> (steps - 1);
        g.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());
    }

    const auto shape = juce::roundToInt (processor.parameters.getRawParameterValue ("gateShape")->load());
    const auto tension = processor.parameters.getRawParameterValue ("gateTension")->load();
    juce::Path curve;
    for (int sample = 0; sample <= 400; ++sample)
    {
        const auto phase = static_cast<float> (sample) / 400.0f;
        const auto level = MKNoiseRhythmMachineProcessor::evaluateGateCurve (
            previewPattern, levels, offsets, steps, shape, phase, tension);
        const auto x = plot.getX() + phase * plot.getWidth();
        const auto y = plot.getBottom() - level * plot.getHeight();
        if (sample == 0) curve.startNewSubPath (x, y);
        else curve.lineTo (x, y);
    }

    g.setColour (mkui::colours::screenTrace.withAlpha (0.16f));
    g.strokePath (curve, juce::PathStrokeType (6.0f));
    g.setColour (mkui::colours::screenTrace);
    g.strokePath (curve, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));

    for (int point = 0; point < steps; ++point)
    {
        const auto offset = point == 0 || point == steps - 1 ? 0.0f : offsets[static_cast<size_t> (point)];
        const auto x = plot.getX() + (point + offset) / static_cast<float> (steps - 1) * plot.getWidth();
        const auto y = plot.getBottom() - levels[static_cast<size_t> (point)] * plot.getHeight();
        const auto selected = point == selectedPoint;
        const auto diameter = selected ? 10.0f : 7.0f;
        g.setColour (selected ? mkui::colours::screenDot
                              : mkui::colours::screenTrace.withAlpha (0.9f));
        g.fillEllipse (x - diameter * 0.5f, y - diameter * 0.5f, diameter, diameter);
    }

    g.setColour (mkui::colours::cursor.withAlpha (0.9f));
    g.drawVerticalLine (juce::roundToInt (plot.getX() + processor.getGatePlaybackPhase() * plot.getWidth()),
                        plot.getY(), plot.getBottom());
}

void MKNoiseRhythmMachineEditor::GateGrid::setStepFromPosition (juce::Point<float> position)
{
    if (selectedPoint < 0)
        return;
    const auto steps = processor.getGateStepCount();
    const auto plot = plotBounds();
    const auto phase = juce::jlimit (0.0f, 1.0f, (position.x - plot.getX()) / plot.getWidth());
    const auto value = juce::jlimit (0.0f, 1.0f, (plot.getBottom() - position.y) / plot.getHeight());
    processor.setGateStep (selectedPoint, value);
    if (selectedPoint > 0 && selectedPoint < steps - 1)
    {
        auto* parameter = processor.parameters.getParameter (processor.gatePositionParameterID (selectedPoint));
        const auto freePositions = processor.parameters.getRawParameterValue ("gateUsePositions")->load() > 0.5f;
        const auto neighbour = [this, steps] (int point) {
            return (point + processor.getGatePointOffset (point)) / (steps - 1);
        };
        const auto lower = neighbour (selectedPoint - 1) + 0.000001f;
        const auto upper = juce::jmax (lower, neighbour (selectedPoint + 1) - 0.000001f);
        const auto time = freePositions
            ? juce::jlimit (lower, upper, phase)
            : juce::jlimit (-0.49f, 0.49f, phase * (steps - 1) - selectedPoint);
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (time));
    }
    repaint();
}

void MKNoiseRhythmMachineEditor::GateGrid::mouseDown (const juce::MouseEvent& event)
{
    const auto steps = processor.getGateStepCount();
    const auto plot = plotBounds();
    auto closest = std::numeric_limits<float>::max();
    for (int point = 0; point < steps; ++point)
    {
        const auto offset = point == 0 || point == steps - 1 ? 0.0f
            : processor.getGatePointOffset (point);
        const juce::Point<float> pos {
            plot.getX() + (point + offset) / (steps - 1) * plot.getWidth(),
            plot.getBottom() - processor.getGateStep (point) * plot.getHeight()
        };
        const auto distance = pos.getDistanceSquaredFrom (event.position);
        if (distance < closest)
        {
            closest = distance;
            selectedPoint = point;
        }
    }
    processor.parameters.getParameter ("gateStep" + juce::String (selectedPoint + 1).paddedLeft ('0', 2))->beginChangeGesture();
    processor.parameters.getParameter (processor.gatePositionParameterID (selectedPoint))->beginChangeGesture();
    setStepFromPosition (event.position);
}

void MKNoiseRhythmMachineEditor::GateGrid::mouseDrag (const juce::MouseEvent& event)
{
    setStepFromPosition (event.position);
}

void MKNoiseRhythmMachineEditor::GateGrid::mouseUp (const juce::MouseEvent&)
{
    if (selectedPoint < 0)
        return;
    processor.parameters.getParameter ("gateStep" + juce::String (selectedPoint + 1).paddedLeft ('0', 2))->endChangeGesture();
    processor.parameters.getParameter (processor.gatePositionParameterID (selectedPoint))->endChangeGesture();
    selectedPoint = -1;
}

juce::Rectangle<float> MKNoiseRhythmMachineEditor::GateGrid::plotBounds() const
{
    return getLocalBounds().toFloat().reduced (juce::jmax (6.0f, getHeight() * 0.075f));
}

void MKNoiseRhythmMachineEditor::timerCallback()
{
    gateGrid.repaint();
    const auto preset = juce::roundToInt (processor.parameters.getRawParameterValue ("gatePreset")->load());
    // A user selection is delivered asynchronously; do not overwrite it with the unchanged saved index.
    if (preset != lastSyncedPreset)
    {
        lastSyncedPreset = preset;
        gatePreset.setSelectedItemIndex (preset, juce::dontSendNotification);
        presetDescription.setText (processor.getGatePresetDescription(), juce::dontSendNotification);
    }
}