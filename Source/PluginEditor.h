// SPDX-FileCopyrightText: 2026 Disconnec audio / Pravda Audio
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"

class TheBureaucratAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                private juce::Timer
{
public:
    explicit TheBureaucratAudioProcessorEditor (TheBureaucratAudioProcessor&);
    ~TheBureaucratAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;
    void refreshVisualState();

private:
    class BitmapSlider final : public juce::Slider
    {
    public:
        BitmapSlider (juce::Image asset, float startDegrees, float endDegrees,
                      juce::Point<float> normalizedAssetPivot,
                      bool threePositionSelector = false);

        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseDoubleClick (const juce::MouseEvent&) override;

        std::function<void()> onDelayedReset;

    private:
        void chooseSelectorPosition (juce::Point<float>);

        juce::Image image;
        juce::Point<float> assetPivot;
        float startAngle = 0.0f;
        float endAngle = 0.0f;
        bool isThreePositionSelector = false;
    };

    class BitmapToggle final : public juce::Button
    {
    public:
        enum class Kind { planLever, gulagButton };

        BitmapToggle (const juce::String& name, juce::Image asset, Kind kindToUse);
        void paintButton (juce::Graphics&, bool highlighted, bool down) override;

    private:
        juce::Image image;
        Kind kind;
    };

    class NeedleMeter final : public juce::Component
    {
    public:
        NeedleMeter (juce::Image needleAsset, float minimumDegrees, float maximumDegrees);
        void setValue (float newValue);
        void paint (juce::Graphics&) override;

    private:
        juce::Image image;
        float value = 0.0f;
        float minimumAngle = 0.0f;
        float maximumAngle = 0.0f;
    };

    class Lamp final : public juce::Component
    {
    public:
        Lamp (juce::Image offAsset, juce::Image onAsset);
        void setActive (bool shouldBeActive);
        void paint (juce::Graphics&) override;

    private:
        juce::Image offImage;
        juce::Image onImage;
        bool active = false;
    };

    class StatusWindow final : public juce::Component
    {
    public:
        explicit StatusWindow (juce::Image statusAsset);
        void setActive (bool shouldBeActive);
        void paint (juce::Graphics&) override;

    private:
        juce::Image image;
        bool active = false;
    };

    class RackLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        RackLookAndFeel();
        juce::Font getComboBoxFont (juce::ComboBox&) override;
        juce::Font getPopupMenuFont() override;
        void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                           int buttonX, int buttonY, int buttonWidth, int buttonHeight,
                           juce::ComboBox&) override;
        void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    };

    class BrandInfoButton final : public juce::Button
    {
    public:
        BrandInfoButton();
        void paintButton (juce::Graphics&, bool highlighted, bool down) override;
    };

    class AboutOverlay final : public juce::Component
    {
    public:
        AboutOverlay();
        void paint (juce::Graphics&) override;
        void resized() override;
        void mouseDown (const juce::MouseEvent&) override;

    private:
        juce::Rectangle<float> getDialogBounds() const noexcept;
        juce::TextButton closeButton { "Close" };
    };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    static juce::Image loadAsset (const char* data, int dataSize);
    static float parameterValue (const juce::AudioProcessorValueTreeState&, const char* id);
    float getPanelScale() const noexcept;
    juce::Rectangle<float> getPanelBounds() const noexcept;
    juce::Rectangle<int> scaledBounds (juce::Rectangle<float>) const noexcept;
    void configureSlider (BitmapSlider&, double minimum, double maximum, double interval,
                          const juce::String& accessibleName);
    void showDelayedResetNotice (BitmapSlider&, double resetValue);
    void timerCallback() override;

    TheBureaucratAudioProcessor& audioProcessor;
    juce::Image panelBackground;

    BitmapSlider ironCurtain;
    BitmapSlider queue;
    BitmapSlider redTape;
    BitmapSlider censor;
    BitmapSlider output;
    BitmapSlider loyalty;
    BitmapToggle plan;
    BitmapToggle gulag;
    NeedleMeter conformityMeter;
    NeedleMeter loyaltyMeter;
    Lamp surveillanceLamp;
    StatusWindow overfulfilledStatus;
    RackLookAndFeel rackLookAndFeel;
    juce::ComboBox presetMenu;
    BrandInfoButton brandInfoButton;
    AboutOverlay aboutOverlay;
    juce::Label resetNotice;

    std::unique_ptr<SliderAttachment> ironAttachment;
    std::unique_ptr<SliderAttachment> queueAttachment;
    std::unique_ptr<SliderAttachment> redTapeAttachment;
    std::unique_ptr<SliderAttachment> censorAttachment;
    std::unique_ptr<SliderAttachment> outputAttachment;
    std::unique_ptr<SliderAttachment> loyaltyAttachment;
    std::unique_ptr<ButtonAttachment> planAttachment;
    std::unique_ptr<ButtonAttachment> gulagAttachment;

    BitmapSlider* frozenSlider = nullptr;
    double frozenResetValue = 0.0;
    juce::uint32 resetDeadline = 0;
    int blinkCounter = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TheBureaucratAudioProcessorEditor)
};
