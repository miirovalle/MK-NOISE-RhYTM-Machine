#include "AnalogLookAndFeel.h"
#include <AnalogAssets.h>

namespace
{
std::unique_ptr<juce::Drawable> loadDrawable (const void* data, size_t size)
{
    if (auto drawable = juce::Drawable::createFromImageData (data, size))
        return drawable;

    jassertfalse;
    return std::make_unique<juce::DrawableComposite>();
}

constexpr float dropdownBodyWidth = 180.0f;
constexpr float dropdownBodyHeight = 26.0f;
constexpr float dropdownArrowStart = 156.0f;

constexpr float knobDesignWidth = 120.0f;
constexpr float knobCapDesignWidth = 80.0f;

constexpr float hSliderDesignWidth = 160.0f;
constexpr float hSliderDesignHeight = 32.0f;
constexpr float hSliderTrackStart = 8.0f;
constexpr float hSliderTrackLength = 144.0f;
constexpr float hSliderThumbDesignWidth = 28.0f;
constexpr float hSliderThumbDesignHeight = 36.0f;

constexpr float vSliderDesignWidth = 36.0f;
constexpr float vSliderDesignHeight = 166.0f;
constexpr float vSliderTrackStart = 10.0f;
constexpr float vSliderTrackLength = 146.0f;
constexpr float vSliderThumbDesignHeight = 32.0f;
}

mkui::AnalogLookAndFeel::AnalogLookAndFeel()
    : knobBackground (loadDrawable (AnalogAssets::knob_bg_svg, (size_t) AnalogAssets::knob_bg_svgSize)),
      knobCap (loadDrawable (AnalogAssets::knob_cap_svg, (size_t) AnalogAssets::knob_cap_svgSize)),
      hSliderBackground (loadDrawable (AnalogAssets::hslider_bg_svg, (size_t) AnalogAssets::hslider_bg_svgSize)),
      hSliderThumb (loadDrawable (AnalogAssets::hslider_thumb_svg, (size_t) AnalogAssets::hslider_thumb_svgSize)),
      vSliderBackground (loadDrawable (AnalogAssets::vslider_bg_svg, (size_t) AnalogAssets::vslider_bg_svgSize)),
      vSliderThumb (loadDrawable (AnalogAssets::vslider_thumb_svg, (size_t) AnalogAssets::vslider_thumb_svgSize)),
      toggleOn (loadDrawable (AnalogAssets::toggle_on_svg, (size_t) AnalogAssets::toggle_on_svgSize)),
      toggleOff (loadDrawable (AnalogAssets::toggle_off_svg, (size_t) AnalogAssets::toggle_off_svgSize)),
      dropdown (loadDrawable (AnalogAssets::dropdown_svg, (size_t) AnalogAssets::dropdown_svgSize)),
      readout (loadDrawable (AnalogAssets::readout_svg, (size_t) AnalogAssets::readout_svgSize))
{
    setColour (juce::Label::textColourId, colours::text);
    setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);

    setColour (juce::ToggleButton::textColourId, colours::text);
    setColour (juce::ToggleButton::tickColourId, colours::textBright);
    setColour (juce::ToggleButton::tickDisabledColourId, colours::textDim);

    setColour (juce::Slider::textBoxTextColourId, colours::screenTrace);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, colours::switchBody);

    setColour (juce::ComboBox::textColourId, colours::text);
    setColour (juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::focusedOutlineColourId, juce::Colours::transparentBlack);

    setColour (juce::TextButton::textColourOffId, colours::textBright);
    setColour (juce::TextButton::textColourOnId, colours::textBright);

    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1b211b));
    setColour (juce::PopupMenu::textColourId, colours::text);
    setColour (juce::PopupMenu::headerTextColourId, colours::textDim);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colours::switchBody);
    setColour (juce::PopupMenu::highlightedTextColourId, colours::textBright);

    setColour (juce::ScrollBar::thumbColourId, juce::Colour (0xff4e594e));
    setColour (juce::ScrollBar::trackColourId, juce::Colour (0xff12160f));

    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff141814));
    setColour (juce::TextEditor::textColourId, colours::screenTrace);
    setColour (juce::TextEditor::highlightColourId, colours::switchBody);
    setColour (juce::TextEditor::outlineColourId, juce::Colour (0xff2c342c));

    setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xff1b211b));
    setColour (juce::TooltipWindow::textColourId, colours::text);
    setColour (juce::TooltipWindow::outlineColourId, juce::Colour (0xff2e362e));
}

mkui::AnalogLookAndFeel::~AnalogLookAndFeel() = default;

void mkui::AnalogLookAndFeel::drawDrawable (juce::Graphics& g, juce::Drawable& drawable,
                                            juce::Rectangle<float> destination)
{
    const auto designWidth = drawable.getWidth();
    const auto designHeight = drawable.getHeight();

    if (designWidth <= 0.0f || designHeight <= 0.0f || destination.isEmpty())
        return;

    const auto transform = juce::AffineTransform::scale (destination.getWidth() / designWidth,
                                                        destination.getHeight() / designHeight)
                               .translated (destination.getX(), destination.getY());
    drawable.draw (g, 1.0f, transform);
}

juce::Drawable& mkui::AnalogLookAndFeel::background()
{
    static const auto drawable = loadDrawable (AnalogAssets::background_svg,
                                               (size_t) AnalogAssets::background_svgSize);
    return *drawable;
}

juce::Drawable& mkui::AnalogLookAndFeel::displayScreen()
{
    static const auto drawable = loadDrawable (AnalogAssets::waveform_display_svg,
                                               (size_t) AnalogAssets::waveform_display_svgSize);
    return *drawable;
}

void mkui::AnalogLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                                float sliderPos, float rotaryStartAngle,
                                                float rotaryEndAngle, juce::Slider&)
{
    const juce::Rectangle<float> area { (float) x, (float) y, (float) width, (float) height };
    const auto dialSize = juce::jmin (area.getWidth(), area.getHeight());

    if (dialSize < 6.0f)
        return;

    const auto dial = juce::Rectangle<float> (dialSize, dialSize).withCentre (area.getCentre());
    drawDrawable (g, *knobBackground, dial);

    const auto cap = juce::Rectangle<float> (dialSize * knobCapDesignWidth / knobDesignWidth,
                                             dialSize * knobCapDesignWidth / knobDesignWidth)
                         .withCentre (area.getCentre());
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    juce::Graphics::ScopedSaveState rotation (g);
    g.addTransform (juce::AffineTransform::rotation (angle, cap.getCentreX(), cap.getCentreY()));
    drawDrawable (g, *knobCap, cap);
}

void mkui::AnalogLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                                float sliderPos, float, float,
                                                juce::Slider::SliderStyle style, juce::Slider& slider)
{
    const juce::Rectangle<float> area { (float) x, (float) y, (float) width, (float) height };

    if (area.isEmpty())
        return;

    if (style == juce::Slider::LinearVertical)
    {
        const auto scale = juce::jmin (area.getHeight() / vSliderDesignHeight,
                                       area.getWidth() / vSliderDesignWidth);
        if (scale <= 0.0f)
            return;

        const auto background = juce::Rectangle<float> (vSliderDesignWidth * scale,
                                                        vSliderDesignHeight * scale)
                                    .withCentre (area.getCentre());
        drawDrawable (g, *vSliderBackground, background);

        const auto proportion = juce::jlimit (0.0f, 1.0f, (sliderPos - area.getY()) / area.getHeight());
        const auto thumbCentre = juce::Point<float> (
            background.getCentreX(),
            background.getY() + (vSliderTrackStart + proportion * vSliderTrackLength) * scale);
        const auto thumb = juce::Rectangle<float> (vSliderDesignWidth * scale,
                                                   vSliderThumbDesignHeight * scale)
                               .withCentre (thumbCentre);
        drawDrawable (g, *vSliderThumb, thumb);
        return;
    }

    if (style != juce::Slider::LinearHorizontal)
    {
        juce::LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, 0.0f, 0.0f, style, slider);
        return;
    }

    drawDrawable (g, *hSliderBackground, area);

    const auto thumbScale = area.getHeight() / hSliderDesignHeight;
    const auto widthScale = area.getWidth() / hSliderDesignWidth;
    const auto proportion = juce::jlimit (0.0f, 1.0f, (sliderPos - area.getX()) / area.getWidth());
    const auto thumbCentre = juce::Point<float> (
        area.getX() + (hSliderTrackStart + proportion * hSliderTrackLength) * widthScale,
        area.getCentreY());
    const auto thumb = juce::Rectangle<float> (hSliderThumbDesignWidth * thumbScale,
                                               hSliderThumbDesignHeight * thumbScale)
                           .withCentre (thumbCentre);
    drawDrawable (g, *hSliderThumb, thumb);
}

void mkui::AnalogLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                                bool highlighted, bool down)
{
    const auto area = button.getLocalBounds().toFloat();

    if (area.isEmpty())
        return;

    const auto boxSize = juce::jlimit (10.0f, 26.0f, area.getHeight());
    const auto box = juce::Rectangle<float> (boxSize, boxSize)
                         .withCentre ({ area.getX() + boxSize * 0.5f, area.getCentreY() });
    const auto isOn = button.getToggleState();

    drawDrawable (g, isOn ? *toggleOn : *toggleOff, box);

    if (button.getButtonText().isNotEmpty())
    {
        auto colour = button.findColour (juce::ToggleButton::textColourId);

        if (isOn)
            colour = colour.brighter (0.3f);

        g.setColour (colour);
        g.setFont (juce::Font (juce::FontOptions (juce::jmin (14.0f, boxSize * 0.64f), juce::Font::bold)));
        g.drawFittedText (button.getButtonText(),
                          area.withTrimmedLeft (box.getRight() - area.getX() + boxSize * 0.3f).toNearestInt(),
                          juce::Justification::centredLeft, 1, 0.8f);
    }

    if (highlighted || down)
    {
        g.setColour (colours::textBright.withAlpha (down ? 0.4f : 0.18f));
        g.drawRoundedRectangle (box.expanded (1.5f), 4.0f, 1.0f);
    }
}

void mkui::AnalogLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                                            int, int, int, int, juce::ComboBox& box)
{
    const auto area = juce::Rectangle<float> ((float) width, (float) height).reduced (0.5f);

    if (area.isEmpty())
        return;

    const auto scale = area.getHeight() / dropdownBodyHeight;
    const auto arrowWidth = juce::jmin (area.getWidth() * 0.5f, (dropdownBodyWidth - dropdownArrowStart) * scale);
    const auto arrowArea = area.withTrimmedLeft (area.getWidth() - arrowWidth);

    {
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (area.withTrimmedRight (arrowWidth).getSmallestIntegerContainer());
        drawDrawable (g, *dropdown, area);
    }

    {
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (arrowArea.getSmallestIntegerContainer());
        const auto transform = juce::AffineTransform::scale (scale)
                                   .translated (arrowArea.getX() - dropdownArrowStart * scale, area.getY());
        dropdown->draw (g, 1.0f, transform);
    }

    if (isButtonDown || box.isMouseOver (true))
    {
        g.setColour (colours::textBright.withAlpha (isButtonDown ? 0.12f : 0.06f));
        g.fillRoundedRectangle (area, 3.0f);
    }

    if (box.hasKeyboardFocus (false))
    {
        g.setColour (colours::textBright.withAlpha (0.45f));
        g.drawRoundedRectangle (area, 3.0f, 1.0f);
    }
}

void mkui::AnalogLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    const auto inset = juce::roundToInt (box.getHeight() * 0.35f);
    const auto arrowSpace = juce::roundToInt (box.getHeight() * 0.85f);
    label.setBounds (inset, 1, box.getWidth() - inset - arrowSpace, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

juce::Font mkui::AnalogLookAndFeel::getComboBoxFont (juce::ComboBox& box)
{
    return juce::Font (juce::FontOptions (juce::jmax (9.0f, (float) box.getHeight() * 0.46f)));
}

juce::PopupMenu::Options mkui::AnalogLookAndFeel::getOptionsForComboBoxPopupMenu (juce::ComboBox& box,
                                                                                 juce::Label& label)
{
    auto options = juce::LookAndFeel_V4::getOptionsForComboBoxPopupMenu (box, label)
                       .withStandardItemHeight (juce::jmax (28, box.getHeight()));

    // AUv3 app extensions cannot create windows, so the menu is hosted inside the editor instead of
    // a desktop popup. This is JUCE's supported way for AUv3 hosts and also keeps the menu
    // clipped to the plugin window on the desktop builds.
    if (auto* topLevel = box.getTopLevelComponent())
        options = options.withParentComponent (topLevel);

    return options;
}

juce::Font mkui::AnalogLookAndFeel::getPopupMenuFont()
{
    return juce::Font (juce::FontOptions (15.0f));
}

void mkui::AnalogLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    if (dynamic_cast<juce::Slider*> (label.getParentComponent()) != nullptr && ! label.isBeingEdited())
    {
        drawSliderReadout (g, label.getLocalBounds().toFloat());

        auto colour = label.findColour (juce::Label::textColourId);

        if (! label.isEnabled())
            colour = colour.withAlpha (0.5f);

        g.setColour (colour);
        g.setFont (getLabelFont (label));
        g.drawFittedText (label.getText(), label.getLocalBounds().reduced (3, 0),
                          label.getJustificationType(), 1, 0.8f);
        return;
    }

    juce::LookAndFeel_V4::drawLabel (g, label);
}

juce::Font mkui::AnalogLookAndFeel::getLabelFont (juce::Label& label)
{
    if (dynamic_cast<juce::Slider*> (label.getParentComponent()) != nullptr)
        return juce::Font (juce::FontOptions (juce::jmax (9.0f, (float) label.getHeight() * 0.62f)));

    return juce::LookAndFeel_V4::getLabelFont (label);
}

void mkui::AnalogLookAndFeel::drawSliderReadout (juce::Graphics& g, juce::Rectangle<float> area)
{
    drawDrawable (g, *readout, area.reduced (0.5f));
}

void mkui::AnalogLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                    const juce::Colour& backgroundColour,
                                                    bool highlighted, bool down)
{
    auto area = button.getLocalBounds().toFloat().reduced (0.5f);

    if (area.isEmpty())
        return;

    auto base = backgroundColour.isTransparent() ? colours::switchBody : backgroundColour;

    if (! button.isEnabled())
        base = base.withMultipliedSaturation (0.3f).darker (0.3f);
    else if (down)
        base = base.darker (0.2f);
    else if (highlighted)
        base = base.brighter (0.12f);

    g.setGradientFill (juce::ColourGradient::vertical (base.brighter (0.16f), area.getY(),
                                                       base.darker (0.24f), area.getBottom()));
    g.fillRoundedRectangle (area, 3.0f);
    g.setColour (juce::Colour (0xff1e241e));
    g.drawRoundedRectangle (area, 3.0f, 1.2f);
}

void mkui::AnalogLookAndFeel::drawCornerResizer (juce::Graphics& g, int width, int height,
                                                 bool isMouseOver, bool isMouseDragging)
{
    g.setColour (colours::textDim.withAlpha (isMouseDragging ? 0.9f : (isMouseOver ? 0.6f : 0.35f)));

    for (int line = 1; line <= 3; ++line)
    {
        const auto offset = 3.0f * (float) line;
        g.drawLine ((float) width - 3.0f - offset, (float) height - 3.0f,
                    (float) width - 3.0f, (float) height - 3.0f - offset, 1.2f);
    }
}
