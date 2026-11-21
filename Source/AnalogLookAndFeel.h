#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace mkui
{
namespace colours
{
inline const juce::Colour chassis     { 0xff0f120f };
inline const juce::Colour text        { 0xffc8d4c8 };
inline const juce::Colour textBright  { 0xffe6ede6 };
inline const juce::Colour textDim     { 0xff93a493 };
inline const juce::Colour screenGrid  { 0xff293429 };
inline const juce::Colour screenTrace { 0xffc8e0c8 };
inline const juce::Colour screenDot   { 0xffe6f5e6 };
inline const juce::Colour cursor      { 0xfff18b4c };
inline const juce::Colour switchBody  { 0xff465246 };
}

/** Hardware-styled look and feel that draws the controls from the analog_plugin_svgs artwork. */
class AnalogLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    AnalogLookAndFeel();
    ~AnalogLookAndFeel() override;

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void drawLabel (juce::Graphics&, juce::Label&) override;
    juce::Font getLabelFont (juce::Label&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool highlighted, bool down) override;
    void drawCornerResizer (juce::Graphics&, int width, int height, bool isMouseOver,
                            bool isMouseDragging) override;

    /** Chassis and plate artwork for the editor background. */
    static juce::Drawable& background();
    /** CRT screen and grid artwork behind the editable gate curve. */
    static juce::Drawable& displayScreen();

    static void drawDrawable (juce::Graphics&, juce::Drawable&, juce::Rectangle<float> destination);

private:
    void drawSliderReadout (juce::Graphics&, juce::Rectangle<float>);

    std::unique_ptr<juce::Drawable> knobBackground, knobCap, hSliderBackground, hSliderThumb,
        vSliderBackground, vSliderThumb, toggleOn, toggleOff, dropdown, readout;
};
}
