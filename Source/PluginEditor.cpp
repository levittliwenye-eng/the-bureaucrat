// SPDX-FileCopyrightText: 2026 Disconnec audio / Pravda Audio
// SPDX-License-Identifier: AGPL-3.0-only

#include "PluginEditor.h"

#include "BinaryData.h"
#include "ParameterIds.h"

#include <array>
#include <cmath>

namespace
{
constexpr int editorWidth = 1280;
constexpr int editorHeight = 560;

constexpr juce::Point<float> largeKnobPivot { 562.0f / 1126.0f, 553.0f / 1126.0f };
constexpr juce::Point<float> smallKnobPivot { 486.5f / 974.0f, 459.0f / 974.0f };
constexpr juce::Point<float> selectorPivot { 399.5f / 800.0f, 374.5f / 800.0f };
constexpr juce::Point<float> centredPivot { 0.5f, 0.5f };
constexpr juce::Point<float> gulagPivot { 562.5f / 1126.0f, 552.0f / 1126.0f };
constexpr juce::Point<float> lampOffPivot { 497.0f / 1000.0f, 476.5f / 1000.0f };
constexpr juce::Point<float> lampOnPivot { 515.0f / 1034.0f, 506.0f / 1034.0f };

juce::AffineTransform centredTransform (const juce::Image& image,
                                        juce::Rectangle<float> destination,
                                        float angle,
                                        float scaleMultiplier = 1.0f,
                                        juce::Point<float> normalizedSourcePivot = centredPivot)
{
    const auto sourcePivot = juce::Point<float> (image.getWidth() * normalizedSourcePivot.x,
                                                 image.getHeight() * normalizedSourcePivot.y);
    const auto scale = juce::jmin (destination.getWidth() / static_cast<float> (image.getWidth()),
                                   destination.getHeight() / static_cast<float> (image.getHeight()))
                     * scaleMultiplier;
    const auto targetCentre = destination.getCentre();

    return juce::AffineTransform::translation (-sourcePivot.x, -sourcePivot.y)
        .scaled (scale)
        .rotated (angle)
        .translated (targetCentre.x, targetCentre.y);
}

void drawMountingHardware (juce::Graphics& graphics, juce::Rectangle<float> bounds,
                           bool selector)
{
    graphics.saveState();
    const auto centre = bounds.getCentre();
    const auto diameter = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto collarDiameter = diameter * (selector ? 0.98f : 0.985f);
    const auto collar = juce::Rectangle<float> (collarDiameter, collarDiameter)
                            .withCentre (centre);

    juce::ColourGradient metal (juce::Colour (0xff817b68),
                                centre.translated (-diameter * 0.18f, -diameter * 0.22f),
                                juce::Colour (0xff20221d),
                                centre.translated (diameter * 0.20f, diameter * 0.24f), false);
    const auto collarWidth = juce::jmax (2.25f, diameter * 0.038f);
    graphics.setGradientFill (metal);
    graphics.drawEllipse (collar.reduced (collarWidth * 0.5f), collarWidth);
    graphics.setColour (juce::Colour (0xe0090a08));
    graphics.drawEllipse (collar.reduced (collarWidth), juce::jmax (1.0f, diameter * 0.012f));
    graphics.setColour (juce::Colour (0x807f765f));
    graphics.drawEllipse (collar.reduced (collarWidth * 0.25f), 1.0f);
    graphics.restoreState();
}

void drawRetainingScrew (juce::Graphics& graphics, juce::Rectangle<float> bounds)
{
    graphics.saveState();
    const auto centre = bounds.getCentre();
    const auto diameter = juce::jlimit (6.0f, 10.0f,
                                       juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.075f);
    const auto screw = juce::Rectangle<float> (diameter, diameter).withCentre (centre);

    juce::ColourGradient metal (juce::Colour (0xff797462), screw.getTopLeft(),
                                juce::Colour (0xff20211c), screw.getBottomRight(), false);
    graphics.setGradientFill (metal);
    graphics.fillEllipse (screw);
    graphics.setColour (juce::Colour (0xe0070807));
    graphics.drawEllipse (screw, 1.0f);
    graphics.drawLine ({ centre.x - diameter * 0.22f, centre.y + diameter * 0.12f,
                         centre.x + diameter * 0.22f, centre.y - diameter * 0.12f },
                       juce::jmax (1.0f, diameter * 0.12f));
    graphics.restoreState();
}
}

TheBureaucratAudioProcessorEditor::BitmapSlider::BitmapSlider (
    juce::Image asset, float startDegrees, float endDegrees,
    juce::Point<float> normalizedAssetPivot, bool threePositionSelector)
    : image (std::move (asset)),
      assetPivot (normalizedAssetPivot),
      startAngle (juce::degreesToRadians (startDegrees)),
      endAngle (juce::degreesToRadians (endDegrees)),
      isThreePositionSelector (threePositionSelector)
{
    setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    auto juceStartAngle = startAngle;
    auto juceEndAngle = endAngle;
    while (juceStartAngle < 0.0f)
        juceStartAngle += juce::MathConstants<float>::twoPi;
    while (juceEndAngle <= juceStartAngle)
        juceEndAngle += juce::MathConstants<float>::twoPi;
    setRotaryParameters (juceStartAngle, juceEndAngle, true);
    setVelocityBasedMode (false);
    setScrollWheelEnabled (true);
    setWantsKeyboardFocus (true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void TheBureaucratAudioProcessorEditor::BitmapSlider::paint (juce::Graphics& graphics)
{
    if (! image.isValid())
        return;

    const auto proportion = static_cast<float> (valueToProportionOfLength (getValue()));
    const auto angle = startAngle + proportion * (endAngle - startAngle);
    drawMountingHardware (graphics, getLocalBounds().toFloat(), isThreePositionSelector);
    graphics.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    graphics.drawImageTransformed (
        image, centredTransform (image, getLocalBounds().toFloat(), angle, 1.0f, assetPivot));
    drawRetainingScrew (graphics, getLocalBounds().toFloat());
}

void TheBureaucratAudioProcessorEditor::BitmapSlider::chooseSelectorPosition (juce::Point<float> point)
{
    if (! isThreePositionSelector)
        return;

    const auto centre = getLocalBounds().toFloat().getCentre();
    const auto relative = point - centre;
    const auto angle = std::atan2 (relative.x, -relative.y);
    const auto choice = angle < -0.25f ? 0.0 : (angle > 0.25f ? 2.0 : 1.0);
    setValue (choice, juce::sendNotificationSync);
}

void TheBureaucratAudioProcessorEditor::BitmapSlider::mouseDown (const juce::MouseEvent& event)
{
    juce::Slider::mouseDown (event);
    chooseSelectorPosition (event.position);
}

void TheBureaucratAudioProcessorEditor::BitmapSlider::mouseDrag (const juce::MouseEvent& event)
{
    juce::Slider::mouseDrag (event);
    if (isThreePositionSelector)
        setValue (std::round (getValue()), juce::sendNotificationSync);
}

void TheBureaucratAudioProcessorEditor::BitmapSlider::mouseDoubleClick (const juce::MouseEvent& event)
{
    juce::ignoreUnused (event);
    if (onDelayedReset != nullptr)
        onDelayedReset();
}

TheBureaucratAudioProcessorEditor::BitmapToggle::BitmapToggle (
    const juce::String& name, juce::Image asset, Kind kindToUse)
    : juce::Button (name), image (std::move (asset)), kind (kindToUse)
{
    setClickingTogglesState (true);
    setWantsKeyboardFocus (true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void TheBureaucratAudioProcessorEditor::BitmapToggle::paintButton (
    juce::Graphics& graphics, bool highlighted, bool down)
{
    if (! image.isValid())
        return;

    auto bounds = getLocalBounds().toFloat();
    float angle = 0.0f;
    float scale = 1.0f;
    auto pivot = centredPivot;

    if (kind == Kind::planLever)
    {
        const auto isOverfulfilled = getToggleState();
        angle = isOverfulfilled ? juce::MathConstants<float>::pi : 0.0f;
    }
    else
    {
        pivot = gulagPivot;
        if (down || getToggleState())
        {
            scale = getToggleState() ? 0.94f : 0.97f;
            bounds.translate (0.0f, getToggleState() ? 2.0f : 1.0f);
        }
    }

    graphics.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    graphics.setOpacity (highlighted ? 1.0f : 0.98f);
    graphics.drawImageTransformed (image, centredTransform (image, bounds, angle, scale, pivot));

    if (kind == Kind::gulagButton && getToggleState())
    {
        const auto diameter = juce::jmin (getWidth(), getHeight()) * 0.68f;
        const auto activeRing = juce::Rectangle<float> (diameter, diameter)
                                    .withCentre (getLocalBounds().toFloat().getCentre())
                                    .translated (0.0f, 2.0f);
        graphics.setColour (juce::Colour (0xd0a52c25));
        graphics.drawEllipse (activeRing, juce::jmax (2.0f, diameter * 0.035f));
    }

    if (hasKeyboardFocus (true))
    {
        const auto focusDot = juce::Rectangle<float> (7.0f, 7.0f).withPosition (5.0f, 5.0f);
        graphics.setColour (juce::Colour (0xff9e2d25));
        graphics.fillEllipse (focusDot);
        graphics.setColour (juce::Colour (0xffd4c197));
        graphics.drawEllipse (focusDot, 1.0f);
    }
}

TheBureaucratAudioProcessorEditor::NeedleMeter::NeedleMeter (
    juce::Image needleAsset, float minimumDegrees, float maximumDegrees)
    : image (std::move (needleAsset)),
      minimumAngle (juce::degreesToRadians (minimumDegrees)),
      maximumAngle (juce::degreesToRadians (maximumDegrees))
{
    setInterceptsMouseClicks (false, false);
}

void TheBureaucratAudioProcessorEditor::NeedleMeter::setValue (float newValue)
{
    newValue = juce::jlimit (0.0f, 1.0f, newValue);
    if (std::abs (value - newValue) > 0.001f)
    {
        value = newValue;
        repaint();
    }
}

void TheBureaucratAudioProcessorEditor::NeedleMeter::paint (juce::Graphics& graphics)
{
    if (! image.isValid())
        return;

    const auto angle = minimumAngle + value * (maximumAngle - minimumAngle);
    const auto sourcePivot = juce::Point<float> (image.getWidth() * 0.5f, image.getHeight() * 0.92f);
    const auto targetPivot = juce::Point<float> (getWidth() * 0.5f, getHeight() * 0.79f);
    const auto visibleNeedleLength = sourcePivot.y;
    const auto targetNeedleLength = getHeight() * 0.58f;
    const auto scale = targetNeedleLength / visibleNeedleLength;
    const auto transform = juce::AffineTransform::translation (-sourcePivot.x, -sourcePivot.y)
                               .scaled (scale)
                               .rotated (angle)
                               .translated (targetPivot.x, targetPivot.y);

    graphics.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    graphics.drawImageTransformed (image, transform);
}

TheBureaucratAudioProcessorEditor::Lamp::Lamp (juce::Image offAsset, juce::Image onAsset)
    : offImage (std::move (offAsset)), onImage (std::move (onAsset))
{
    setInterceptsMouseClicks (false, false);
}

void TheBureaucratAudioProcessorEditor::Lamp::setActive (bool shouldBeActive)
{
    if (active != shouldBeActive)
    {
        active = shouldBeActive;
        repaint();
    }
}

void TheBureaucratAudioProcessorEditor::Lamp::paint (juce::Graphics& graphics)
{
    const auto& selected = active ? onImage : offImage;
    if (selected.isValid())
    {
        const auto pivot = active ? lampOnPivot : lampOffPivot;
        const auto sourceDiameter = selected.getWidth()
                                  * (active ? 977.0f / 1034.0f : 918.0f / 1000.0f);
        const auto scale = 40.5f / sourceDiameter;
        const auto sourcePivot = juce::Point<float> (selected.getWidth() * pivot.x,
                                                     selected.getHeight() * pivot.y);
        const auto targetPivot = getLocalBounds().toFloat().getCentre();
        const auto transform = juce::AffineTransform::translation (-sourcePivot.x, -sourcePivot.y)
                                   .scaled (scale)
                                   .translated (targetPivot.x, targetPivot.y);
        graphics.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        graphics.drawImageTransformed (selected, transform);
    }
}

TheBureaucratAudioProcessorEditor::StatusWindow::StatusWindow (juce::Image statusAsset)
    : image (std::move (statusAsset))
{
    setInterceptsMouseClicks (false, false);
}

void TheBureaucratAudioProcessorEditor::StatusWindow::setActive (bool shouldBeActive)
{
    if (active != shouldBeActive)
    {
        active = shouldBeActive;
        repaint();
    }
}

void TheBureaucratAudioProcessorEditor::StatusWindow::paint (juce::Graphics& graphics)
{
    if (active && image.isValid())
    {
        graphics.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        graphics.drawImageWithin (image, 0, 0, getWidth(), getHeight(),
                                  juce::RectanglePlacement::centred, false);
    }
}

TheBureaucratAudioProcessorEditor::RackLookAndFeel::RackLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff171914));
    setColour (juce::ComboBox::textColourId, juce::Colour (0xffdfcfa7));
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff766b50));
    setColour (juce::ComboBox::focusedOutlineColourId, juce::Colour (0xff766b50));
    setColour (juce::ComboBox::arrowColourId, juce::Colour (0xffd4c197));
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff171914));
    setColour (juce::PopupMenu::textColourId, juce::Colour (0xffdfcfa7));
    setColour (juce::PopupMenu::headerTextColourId, juce::Colour (0xffb8a77f));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff6e2922));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xffffe6b0));
}

juce::Font TheBureaucratAudioProcessorEditor::RackLookAndFeel::getComboBoxFont (
    juce::ComboBox& box)
{
    auto font = juce::Font (juce::FontOptions (juce::jmax (10.0f, box.getHeight() * 0.48f),
                                                juce::Font::bold));
    font.setTypefaceName ("Helvetica Neue");
    return font;
}

juce::Font TheBureaucratAudioProcessorEditor::RackLookAndFeel::getPopupMenuFont()
{
    auto font = juce::Font (juce::FontOptions (13.0f));
    font.setTypefaceName ("Helvetica Neue");
    return font;
}

void TheBureaucratAudioProcessorEditor::RackLookAndFeel::drawComboBox (
    juce::Graphics& graphics, int width, int height, bool isButtonDown,
    int buttonX, int buttonY, int buttonWidth, int buttonHeight, juce::ComboBox& box)
{
    juce::ignoreUnused (buttonY, buttonHeight);
    const auto bounds = juce::Rectangle<float> (0.5f, 0.5f,
                                                static_cast<float> (width - 1),
                                                static_cast<float> (height - 1));
    graphics.setColour (findColour (juce::ComboBox::backgroundColourId)
                            .brighter (isButtonDown ? 0.08f : 0.0f));
    graphics.fillRoundedRectangle (bounds, 2.0f);
    graphics.setColour (findColour (juce::ComboBox::outlineColourId)
                            .brighter (box.isMouseOverOrDragging() ? 0.18f : 0.0f));
    graphics.drawRoundedRectangle (bounds, 2.0f, 1.0f);

    const auto centreX = buttonX + buttonWidth * 0.5f;
    const auto centreY = height * 0.48f;
    juce::Path arrow;
    arrow.startNewSubPath (centreX - 4.0f, centreY - 2.0f);
    arrow.lineTo (centreX, centreY + 2.0f);
    arrow.lineTo (centreX + 4.0f, centreY - 2.0f);
    graphics.setColour (findColour (juce::ComboBox::arrowColourId));
    graphics.strokePath (arrow, juce::PathStrokeType (1.4f));
}

void TheBureaucratAudioProcessorEditor::RackLookAndFeel::positionComboBoxText (
    juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (7, 0, box.getWidth() - 28, box.getHeight());
    label.setFont (getComboBoxFont (box));
    label.setJustificationType (juce::Justification::centredLeft);
}

TheBureaucratAudioProcessorEditor::BrandInfoButton::BrandInfoButton()
    : juce::Button ("About Pravda Audio")
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setTitle ("About THE BUREAUCRAT, Pravda Audio, and Disconnec audio");
    setWantsKeyboardFocus (false);
}

void TheBureaucratAudioProcessorEditor::BrandInfoButton::paintButton (
    juce::Graphics& graphics, bool highlighted, bool down)
{
    if (! highlighted && ! down)
        return;

    graphics.setColour (juce::Colour (down ? 0x24d4c197 : 0x12d4c197));
    graphics.fillRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 2.0f);
}

TheBureaucratAudioProcessorEditor::AboutOverlay::AboutOverlay()
{
    setComponentID ("aboutOverlay");
    setVisible (false);
    setInterceptsMouseClicks (true, true);

    closeButton.setButtonText (juce::String::fromUTF8 ("关闭 / CLOSE"));
    closeButton.setComponentID ("aboutClose");
    closeButton.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    closeButton.setWantsKeyboardFocus (false);
    closeButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2b2e27));
    closeButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff3b3e34));
    closeButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffdfcfa7));
    closeButton.onClick = [this] { setVisible (false); };
    addAndMakeVisible (closeButton);
}

juce::Rectangle<float> TheBureaucratAudioProcessorEditor::AboutOverlay::getDialogBounds() const noexcept
{
    const auto scale = juce::jmin (getWidth() / static_cast<float> (editorWidth),
                                   getHeight() / static_cast<float> (editorHeight));
    return juce::Rectangle<float> (660.0f * scale, 354.0f * scale)
        .withCentre (getLocalBounds().toFloat().getCentre());
}

void TheBureaucratAudioProcessorEditor::AboutOverlay::paint (juce::Graphics& graphics)
{
    graphics.fillAll (juce::Colours::black.withAlpha (0.72f));
    const auto dialog = getDialogBounds();
    const auto scale = dialog.getWidth() / 660.0f;

    graphics.setColour (juce::Colours::black.withAlpha (0.62f));
    graphics.fillRoundedRectangle (dialog.translated (6.0f * scale, 7.0f * scale), 4.0f * scale);
    graphics.setColour (juce::Colour (0xff252820));
    graphics.fillRoundedRectangle (dialog, 4.0f * scale);
    graphics.setColour (juce::Colour (0xff8c7c59));
    graphics.drawRoundedRectangle (dialog.reduced (0.75f * scale), 4.0f * scale,
                                   juce::jmax (1.0f, 1.5f * scale));

    const auto content = dialog.reduced (34.0f * scale, 24.0f * scale);
    graphics.setColour (juce::Colour (0xffdfcfa7));
    auto titleFont = juce::Font (juce::FontOptions (28.0f * scale, juce::Font::bold));
    titleFont.setTypefaceName ("Helvetica Neue");
    graphics.setFont (titleFont);
    graphics.drawFittedText ("THE BUREAUCRAT",
                             content.withHeight (38.0f * scale).toNearestInt(),
                             juce::Justification::centredLeft, 1);

    auto smallBold = juce::Font (juce::FontOptions (11.0f * scale, juce::Font::bold));
    smallBold.setTypefaceName ("Helvetica Neue");
    graphics.setFont (smallBold);
    graphics.setColour (juce::Colour (0xffb74b3f));
    graphics.drawFittedText ("VERSION 1.0.0  /  OPEN-SOURCE RELEASE IN PREPARATION",
                             content.withTrimmedTop (39.0f * scale)
                                    .withHeight (18.0f * scale).toNearestInt(),
                             juce::Justification::centredLeft, 1);

    graphics.setColour (juce::Colour (0x88766b50));
    graphics.drawHorizontalLine (juce::roundToInt (content.getY() + 68.0f * scale),
                                 content.getX(), content.getRight());

    auto bodyFont = juce::Font (juce::FontOptions (16.0f * scale));
    bodyFont.setTypefaceName ("Songti SC");
    graphics.setFont (bodyFont);
    graphics.setColour (juce::Colour (0xffe6d8b7));
    const auto productText = juce::String::fromUTF8 (
        "真理音频 / PRAVDA AUDIO\n"
        "A DISCONNEC AUDIO IMPRINT\n"
        "信号饱和与干预效果器\n"
        "8 个可自动化参数  ·  11 个工厂预设\n"
        "AU / VST3  ·  macOS 11+");
    graphics.drawFittedText (productText,
                             content.withTrimmedTop (79.0f * scale)
                                    .withHeight (82.0f * scale).toNearestInt(),
                             juce::Justification::topLeft, 5, 0.92f);

    graphics.setFont (smallBold);
    graphics.setColour (juce::Colour (0xffb8a77f));
    graphics.drawFittedText (juce::String::fromUTF8 ("开源说明 / OPEN-SOURCE INFORMATION"),
                             content.withTrimmedTop (166.0f * scale)
                                    .withHeight (18.0f * scale).toNearestInt(),
                             juce::Justification::centredLeft, 1);

    graphics.setFont (bodyFont.withHeight (14.5f * scale));
    graphics.setColour (juce::Colour (0xffd2c4a3));
    const auto openSourceText = juce::String::fromUTF8 (
        "本项目正在准备公开源代码。\n"
        "最终仓库地址与开源许可证将随源码发布。\n"
        "Built with JUCE 8.0.11, Steinberg VST3 SDK and Apple Audio Unit SDK.\n"
        "无激活  ·  无遥测  ·  无网络访问");
    graphics.drawFittedText (openSourceText,
                             content.withTrimmedTop (190.0f * scale)
                                    .withHeight (86.0f * scale).toNearestInt(),
                             juce::Justification::topLeft, 4, 0.92f);

    graphics.setFont (smallBold.withHeight (10.5f * scale));
    graphics.setColour (juce::Colour (0xff8f846b));
    graphics.drawFittedText ("Copyright 2026 Disconnec audio",
                             content.withTrimmedTop (282.0f * scale)
                                    .withHeight (18.0f * scale).toNearestInt(),
                             juce::Justification::centredLeft, 1);
}

void TheBureaucratAudioProcessorEditor::AboutOverlay::resized()
{
    const auto dialog = getDialogBounds();
    const auto scale = dialog.getWidth() / 660.0f;
    const auto buttonWidth = 112.0f * scale;
    const auto buttonHeight = 27.0f * scale;
    closeButton.setBounds (juce::Rectangle<float> (buttonWidth, buttonHeight)
                               .withPosition (dialog.getRight() - 24.0f * scale - buttonWidth,
                                              dialog.getBottom() - 20.0f * scale - buttonHeight)
                               .toNearestInt());
}

void TheBureaucratAudioProcessorEditor::AboutOverlay::mouseDown (
    const juce::MouseEvent& event)
{
    if (! getDialogBounds().contains (event.position))
        setVisible (false);
}

juce::Image TheBureaucratAudioProcessorEditor::loadAsset (const char* data, int dataSize)
{
    return juce::ImageCache::getFromMemory (data, dataSize);
}

float TheBureaucratAudioProcessorEditor::parameterValue (
    const juce::AudioProcessorValueTreeState& state, const char* id)
{
    if (const auto* value = state.getRawParameterValue (id))
        return value->load();
    return 0.0f;
}

TheBureaucratAudioProcessorEditor::TheBureaucratAudioProcessorEditor (
    TheBureaucratAudioProcessor& processorToUse)
    : AudioProcessorEditor (&processorToUse),
      audioProcessor (processorToUse),
      panelBackground (loadAsset (BinaryData::panelbackground1280_png,
                                  BinaryData::panelbackground1280_pngSize)),
      ironCurtain (loadAsset (BinaryData::knoblarge_png, BinaryData::knoblarge_pngSize),
                   -135.0f, 135.0f, largeKnobPivot),
      queue (loadAsset (BinaryData::selector_png, BinaryData::selector_pngSize),
             -125.0f, 125.0f, selectorPivot, true),
      redTape (loadAsset (BinaryData::knobsmall_png, BinaryData::knobsmall_pngSize),
               -135.0f, 135.0f, smallKnobPivot),
      censor (loadAsset (BinaryData::knobsmall_png, BinaryData::knobsmall_pngSize),
              -135.0f, 135.0f, smallKnobPivot),
      output (loadAsset (BinaryData::knobsmall_png, BinaryData::knobsmall_pngSize),
              -135.0f, 135.0f, smallKnobPivot),
      loyalty (loadAsset (BinaryData::selector_png, BinaryData::selector_pngSize),
               -109.0f, 109.0f, selectorPivot, true),
      plan ("Five-Year Plan", loadAsset (BinaryData::planlever_png, BinaryData::planlever_pngSize),
            BitmapToggle::Kind::planLever),
      gulag ("Send to Gulag", loadAsset (BinaryData::gulagbutton_png, BinaryData::gulagbutton_pngSize),
             BitmapToggle::Kind::gulagButton),
      conformityMeter (loadAsset (BinaryData::meterneedle_png, BinaryData::meterneedle_pngSize), -49.0f, 49.0f),
      loyaltyMeter (loadAsset (BinaryData::meterneedle_png, BinaryData::meterneedle_pngSize), -49.0f, 49.0f),
      surveillanceLamp (loadAsset (BinaryData::lampoff_png, BinaryData::lampoff_pngSize),
                        loadAsset (BinaryData::lampon_png, BinaryData::lampon_pngSize)),
      overfulfilledStatus (loadAsset (BinaryData::statuson_png, BinaryData::statuson_pngSize))
{
    setSize (editorWidth, editorHeight);
    setResizable (true, false);
    setResizeLimits (960, 420, 1920, 840);
    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio (static_cast<double> (editorWidth) / editorHeight);
    setOpaque (true);

    configureSlider (ironCurtain, 0.0, 11.0, 0.01, "Iron Curtain saturation");
    configureSlider (queue, 0.0, 2.0, 1.0, "Queue three-position selector");
    configureSlider (redTape, 0.0, 100.0, 0.01, "Red Tape dry wet mix");
    configureSlider (censor, 0.0, 100.0, 0.01, "Censor filter cutoff");
    configureSlider (output, -18.0, 12.0, 0.01, "Output level");
    configureSlider (loyalty, 0.0, 2.0, 1.0, "Loyalty status three-position selector");
    queue.textFromValueFunction = [] (double value)
    {
        static constexpr std::array<const char*, 3> labels {
            "Promising", "Indefinite", "Come Back Tomorrow"
        };
        return juce::String (labels[static_cast<std::size_t> (
            juce::jlimit (0, 2, juce::roundToInt (value)))]);
    };
    loyalty.textFromValueFunction = [] (double value)
    {
        static constexpr std::array<const char*, 3> labels {
            "Re-Education", "Under Review", "Absolute Loyalty"
        };
        return juce::String (labels[static_cast<std::size_t> (
            juce::jlimit (0, 2, juce::roundToInt (value)))]);
    };

    ironCurtain.setComponentID ("ironCurtain");
    queue.setComponentID ("queue");
    redTape.setComponentID ("redTape");
    censor.setComponentID ("censor");
    output.setComponentID ("output");
    loyalty.setComponentID ("loyaltyReport");
    plan.setComponentID ("planFulfillment");
    gulag.setComponentID ("gulag");
    conformityMeter.setComponentID ("conformityMeter");
    loyaltyMeter.setComponentID ("loyaltyMeter");
    surveillanceLamp.setComponentID ("surveillanceLamp");
    presetMenu.setComponentID ("presetMenu");
    brandInfoButton.setComponentID ("brandInfoButton");

    plan.setTitle ("Normal or overfulfilled processing mode");
    gulag.setTitle ("Freezing feedback delay");

    presetMenu.setLookAndFeel (&rackLookAndFeel);
    presetMenu.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    presetMenu.setTitle ("Factory preset selector");
    for (int program = 0; program < audioProcessor.getNumPrograms(); ++program)
        presetMenu.addItem (audioProcessor.getProgramName (program), program + 1);
    presetMenu.setSelectedId (audioProcessor.getCurrentProgram() + 1,
                              juce::dontSendNotification);
    presetMenu.onChange = [this]
    {
        const auto selectedProgram = presetMenu.getSelectedId() - 1;
        if (juce::isPositiveAndBelow (selectedProgram, audioProcessor.getNumPrograms()))
            audioProcessor.setCurrentProgram (selectedProgram);
    };

    brandInfoButton.onClick = [this]
    {
        aboutOverlay.setVisible (true);
        aboutOverlay.toFront (false);
    };

    for (auto* component : { static_cast<juce::Component*> (&ironCurtain),
                             static_cast<juce::Component*> (&queue),
                             static_cast<juce::Component*> (&redTape),
                             static_cast<juce::Component*> (&censor),
                             static_cast<juce::Component*> (&output),
                             static_cast<juce::Component*> (&loyalty),
                             static_cast<juce::Component*> (&plan),
                             static_cast<juce::Component*> (&gulag),
                             static_cast<juce::Component*> (&conformityMeter),
                             static_cast<juce::Component*> (&loyaltyMeter),
                             static_cast<juce::Component*> (&surveillanceLamp),
                             static_cast<juce::Component*> (&overfulfilledStatus),
                             static_cast<juce::Component*> (&presetMenu),
                             static_cast<juce::Component*> (&brandInfoButton) })
        addAndMakeVisible (*component);

    addChildComponent (aboutOverlay);

    ironCurtain.onDelayedReset = [this] { showDelayedResetNotice (ironCurtain, 4.0); };
    queue.onDelayedReset = [this] { showDelayedResetNotice (queue, 1.0); };
    redTape.onDelayedReset = [this] { showDelayedResetNotice (redTape, 35.0); };
    censor.onDelayedReset = [this] { showDelayedResetNotice (censor, 28.0); };
    output.onDelayedReset = [this] { showDelayedResetNotice (output, -2.0); };
    loyalty.onDelayedReset = [this] { showDelayedResetNotice (loyalty, 2.0); };

    resetNotice.setText (juce::String::fromUTF8 (
        "由于经办人已休假，该操作需要重新提交 3 份申请表，请稍后再试。"),
        juce::dontSendNotification);
    resetNotice.setJustificationType (juce::Justification::centred);
    resetNotice.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold))
                             .withTypefaceStyle ("Bold"));
    resetNotice.setColour (juce::Label::backgroundColourId, juce::Colour (0xf21a1b17));
    resetNotice.setColour (juce::Label::textColourId, juce::Colour (0xffe4d2a7));
    resetNotice.setColour (juce::Label::outlineColourId, juce::Colour (0xff9e2d25));
    resetNotice.setInterceptsMouseClicks (false, false);
    addChildComponent (resetNotice);

    auto& state = audioProcessor.getState();
    ironAttachment = std::make_unique<SliderAttachment> (
        state, BureaucratParameters::ironCurtain, this->ironCurtain);
    queueAttachment = std::make_unique<SliderAttachment> (state, BureaucratParameters::queue, this->queue);
    redTapeAttachment = std::make_unique<SliderAttachment> (
        state, BureaucratParameters::redTape, this->redTape);
    censorAttachment = std::make_unique<SliderAttachment> (
        state, BureaucratParameters::censor, this->censor);
    outputAttachment = std::make_unique<SliderAttachment> (
        state, BureaucratParameters::output, this->output);
    loyaltyAttachment = std::make_unique<SliderAttachment> (
        state, BureaucratParameters::loyaltyReport, this->loyalty);
    planAttachment = std::make_unique<ButtonAttachment> (
        state, BureaucratParameters::planFulfillment, plan);
    gulagAttachment = std::make_unique<ButtonAttachment> (state, BureaucratParameters::gulag, this->gulag);

    refreshVisualState();
    startTimerHz (30);
}

void TheBureaucratAudioProcessorEditor::configureSlider (
    BitmapSlider& slider, double minimum, double maximum, double interval,
    const juce::String& accessibleName)
{
    slider.setRange (minimum, maximum, interval);
    slider.setName (accessibleName);
    slider.setTitle (accessibleName);
    slider.setMouseDragSensitivity (interval >= 1.0 ? 80 : 180);
}

void TheBureaucratAudioProcessorEditor::paint (juce::Graphics& graphics)
{
    graphics.fillAll (juce::Colour (0xff11120f));
    if (panelBackground.isValid())
        graphics.drawImage (panelBackground, getPanelBounds());

    const auto scale = getPanelScale();
    const auto brandPlate = scaledBounds ({ 145.0f, 13.0f, 243.0f, 50.0f }).toFloat();
    graphics.setColour (juce::Colour (0xff171914));
    graphics.fillRoundedRectangle (brandPlate, 2.0f * scale);
    graphics.setColour (juce::Colour (0xff766b50));
    graphics.drawRoundedRectangle (brandPlate.reduced (0.75f * scale),
                                   2.0f * scale, juce::jmax (1.0f, scale));

    auto brandFont = juce::Font (juce::FontOptions (15.0f * scale, juce::Font::bold));
    brandFont.setTypefaceName ("Songti SC");
    graphics.setFont (brandFont);
    graphics.setColour (juce::Colour (0xffdfcfa7));
    graphics.drawFittedText (juce::String::fromUTF8 ("真理音频 / PRAVDA AUDIO"),
                             scaledBounds ({ 154.0f, 18.0f, 224.0f, 21.0f }),
                             juce::Justification::centredLeft, 1);

    auto subtitleFont = juce::Font (juce::FontOptions (8.5f * scale));
    subtitleFont.setTypefaceName ("Helvetica Neue");
    graphics.setFont (subtitleFont);
    graphics.setColour (juce::Colour (0xffb8a77f));
    graphics.drawFittedText ("A DISCONNEC AUDIO IMPRINT",
                             scaledBounds ({ 154.0f, 40.0f, 224.0f, 14.0f }),
                             juce::Justification::centredLeft, 1);

    graphics.setColour (juce::Colour (0xff8e8163));
    graphics.drawEllipse (scaledBounds ({ 367.0f, 19.0f, 12.0f, 12.0f }).toFloat(),
                          juce::jmax (0.8f, scale));
    graphics.setFont (juce::Font (juce::FontOptions (8.0f * scale, juce::Font::bold)));
    graphics.drawFittedText ("i", scaledBounds ({ 367.0f, 18.0f, 12.0f, 13.0f }),
                             juce::Justification::centred, 1);

    auto presetLabelFont = juce::Font (juce::FontOptions (8.0f * scale, juce::Font::bold));
    presetLabelFont.setTypefaceName ("Helvetica Neue");
    graphics.setFont (presetLabelFont);
    graphics.setColour (juce::Colour (0xffa99670));
    graphics.drawFittedText ("PRESET", scaledBounds ({ 873.0f, 54.0f, 43.0f, 18.0f }),
                             juce::Justification::centredLeft, 1);

    auto chineseFont = juce::Font (juce::FontOptions (20.0f * getPanelScale(),
                                                      juce::Font::bold));
    chineseFont.setTypefaceName ("Songti SC");
    graphics.setFont (chineseFont);
    graphics.setColour (juce::Colour (0xffd4c197));
    graphics.drawFittedText (juce::String::fromUTF8 ("信号饱和与干预效果器"),
                             scaledBounds ({ 440.0f, 58.0f, 400.0f, 20.0f }),
                             juce::Justification::centred, 1);
}

void TheBureaucratAudioProcessorEditor::paintOverChildren (juce::Graphics& graphics)
{
    const auto drawGlass = [&graphics] (juce::Rectangle<float> face)
    {
        graphics.saveState();
        graphics.reduceClipRegion (face.toNearestInt());
        juce::ColourGradient reflection (juce::Colours::white.withAlpha (0.075f),
                                         face.getTopLeft(),
                                         juce::Colours::white.withAlpha (0.0f),
                                         face.getCentre(), false);
        graphics.setGradientFill (reflection);
        graphics.fillRoundedRectangle (face, 7.0f);
        graphics.restoreState();
    };

    drawGlass (scaledBounds ({ 49.0f, 343.0f, 184.0f, 85.0f }).toFloat());
    drawGlass (scaledBounds ({ 1034.0f, 153.0f, 180.0f, 96.0f }).toFloat());

}

void TheBureaucratAudioProcessorEditor::resized()
{
    ironCurtain.setBounds (scaledBounds ({ 74.0f, 157.0f, 126.0f, 126.0f }));
    conformityMeter.setBounds (scaledBounds ({ 46.0f, 329.0f, 188.0f, 126.0f }));
    surveillanceLamp.setBounds (scaledBounds ({ 117.0f, 480.0f, 44.0f, 44.0f }));

    plan.setBounds (scaledBounds ({ 339.0f, 164.0f, 96.0f, 112.0f }));
    queue.setBounds (scaledBounds ({ 550.0f, 168.0f, 104.0f, 104.0f }));
    redTape.setBounds (scaledBounds ({ 348.0f, 355.0f, 108.0f, 108.0f }));
    censor.setBounds (scaledBounds ({ 562.0f, 355.0f, 108.0f, 108.0f }));

    gulag.setBounds (scaledBounds ({ 817.0f, 198.0f, 112.0f, 112.0f }));
    output.setBounds (scaledBounds ({ 839.0f, 398.0f, 68.0f, 68.0f }));

    loyaltyMeter.setBounds (scaledBounds ({ 1029.0f, 145.0f, 184.0f, 124.0f }));
    loyalty.setBounds (scaledBounds ({ 1075.0f, 351.0f, 92.0f, 92.0f }));
    overfulfilledStatus.setBounds (scaledBounds ({ 1110.0f, 34.0f, 84.0f, 28.0f }));
    presetMenu.setBounds (scaledBounds ({ 918.0f, 52.0f, 176.0f, 22.0f }));
    brandInfoButton.setBounds (scaledBounds ({ 145.0f, 13.0f, 243.0f, 50.0f }));
    aboutOverlay.setBounds (getLocalBounds());

    resetNotice.setBounds (scaledBounds ({ 335.0f, 246.0f, 610.0f, 68.0f }));
    resetNotice.setFont (juce::Font (juce::FontOptions (16.0f * getPanelScale(),
                                                        juce::Font::bold))
                             .withTypefaceStyle ("Bold"));
}

void TheBureaucratAudioProcessorEditor::showDelayedResetNotice (
    BitmapSlider& slider, double resetValue)
{
    if (frozenSlider != nullptr)
        frozenSlider->setEnabled (true);

    frozenSlider = &slider;
    frozenResetValue = resetValue;
    frozenSlider->setEnabled (false);
    resetDeadline = juce::Time::getMillisecondCounter() + 1000u;
    resetNotice.setVisible (true);
    resetNotice.toFront (false);
}

void TheBureaucratAudioProcessorEditor::timerCallback()
{
    if (resetNotice.isVisible()
        && static_cast<juce::int32> (juce::Time::getMillisecondCounter() - resetDeadline) >= 0)
    {
        resetNotice.setVisible (false);
        if (frozenSlider != nullptr)
        {
            frozenSlider->setEnabled (true);
            frozenSlider->setValue (frozenResetValue, juce::sendNotificationSync);
            frozenSlider = nullptr;
        }
    }

    refreshVisualState();
}

void TheBureaucratAudioProcessorEditor::refreshVisualState()
{
    const auto currentPreset = audioProcessor.getCurrentProgram() + 1;
    if (presetMenu.getSelectedId() != currentPreset)
        presetMenu.setSelectedId (currentPreset, juce::dontSendNotification);

    const auto surveillance = audioProcessor.isSurveillanceActive();
    const auto blinkOn = ! surveillance || ((++blinkCounter / 3) % 2 == 0);
    surveillanceLamp.setActive (surveillance && blinkOn);
    conformityMeter.setValue (juce::jlimit (0.0f, 1.0f,
        std::sqrt (audioProcessor.getConformityMeter()) * 0.92f));
    loyaltyMeter.setValue (juce::jlimit (0.0f, 1.0f,
        parameterValue (audioProcessor.getState(), BureaucratParameters::loyaltyReport) * 0.5f));

    const auto planIsActive = parameterValue (audioProcessor.getState(),
                                               BureaucratParameters::planFulfillment) >= 0.5f;
    overfulfilledStatus.setActive (planIsActive);
}

float TheBureaucratAudioProcessorEditor::getPanelScale() const noexcept
{
    return juce::jmin (getWidth() / static_cast<float> (editorWidth),
                       getHeight() / static_cast<float> (editorHeight));
}

juce::Rectangle<float> TheBureaucratAudioProcessorEditor::getPanelBounds() const noexcept
{
    const auto scale = getPanelScale();
    return juce::Rectangle<float> (editorWidth * scale, editorHeight * scale)
        .withCentre (getLocalBounds().toFloat().getCentre());
}

juce::Rectangle<int> TheBureaucratAudioProcessorEditor::scaledBounds (
    juce::Rectangle<float> designBounds) const noexcept
{
    const auto panel = getPanelBounds();
    const auto scale = getPanelScale();
    return juce::Rectangle<float> (panel.getX() + designBounds.getX() * scale,
                                   panel.getY() + designBounds.getY() * scale,
                                   designBounds.getWidth() * scale,
                                   designBounds.getHeight() * scale)
        .toNearestInt();
}
