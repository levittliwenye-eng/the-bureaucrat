// SPDX-FileCopyrightText: 2026 Disconnec audio / Pravda Audio
// SPDX-License-Identifier: AGPL-3.0-only

#include "ParameterIds.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <iostream>

namespace
{
bool saveSnapshot (juce::Component& component, const juce::File& file)
{
    auto image = component.createComponentSnapshot (component.getLocalBounds(), true, 1.0f);
    if (file.existsAsFile() && ! file.deleteFile())
    {
        std::cerr << "FAIL: could not replace snapshot " << file.getFullPathName() << '\n';
        return false;
    }

    auto stream = file.createOutputStream();
    if (stream == nullptr)
        return false;

    juce::PNGImageFormat format;
    return format.writeImageToStream (image, *stream);
}

void dispatchMessages (int milliseconds = 150)
{
    juce::Thread::sleep (milliseconds);
    juce::Timer::callPendingTimersSynchronously();
}

bool expectParameter (const juce::AudioProcessorValueTreeState& state,
                      const char* id, float expected)
{
    const auto* value = state.getRawParameterValue (id);
    const auto passed = value != nullptr && std::abs (value->load() - expected) < 0.01f;
    if (! passed)
        std::cerr << "FAIL: control did not write parameter " << id << '\n';
    return passed;
}

bool verifyFactoryPrograms (TheBureaucratAudioProcessor& processor)
{
    const juce::StringArray expectedNames {
        "Factory Default", "Approved Broadcast", "Iron Quota", "Breadline Shuffle",
        "Red Tape Chamber", "Censored Radio", "Five-Year Collapse", "Surveillance State",
        "Siberian Exile", "Absolute Loyalty", "Dissident Underground"
    };

    if (processor.getNumPrograms() != expectedNames.size())
    {
        std::cerr << "FAIL: unexpected factory program count\n";
        return false;
    }

    bool passed = true;
    for (int index = 0; index < expectedNames.size(); ++index)
    {
        processor.setCurrentProgram (index);
        passed &= processor.getCurrentProgram() == index;
        passed &= processor.getProgramName (index) == expectedNames[index];

        for (auto* parameter : processor.getParameters())
        {
            const auto value = parameter->getValue();
            if (! std::isfinite (value) || value < 0.0f || value > 1.0f)
            {
                std::cerr << "FAIL: preset parameter is outside its normalized range\n";
                passed = false;
            }
        }
    }

    processor.setCurrentProgram (8);
    passed &= expectParameter (processor.getState(), BureaucratParameters::queue, 2.0f);
    passed &= expectParameter (processor.getState(), BureaucratParameters::gulag, 1.0f);
    passed &= expectParameter (processor.getState(), BureaucratParameters::output, -6.0f);

    juce::MemoryBlock savedPreset;
    processor.getStateInformation (savedPreset);
    TheBureaucratAudioProcessor restored;
    restored.setStateInformation (savedPreset.getData(), static_cast<int> (savedPreset.getSize()));
    passed &= restored.getCurrentProgram() == 8;
    passed &= restored.getProgramName (restored.getCurrentProgram()) == "Siberian Exile";
    passed &= expectParameter (restored.getState(), BureaucratParameters::gulag, 1.0f);

    processor.setCurrentProgram (0);
    return passed;
}

juce::Component* child (juce::Component& parent, const char* id)
{
    if (auto* result = parent.findChildWithID (id))
        return result;

    std::cerr << "FAIL: missing control " << id << '\n';
    return nullptr;
}

bool expectBounds (juce::Component& parent, const char* id, juce::Rectangle<int> expected)
{
    auto* component = child (parent, id);
    const auto passed = component != nullptr && component->getBounds() == expected;
    if (! passed)
    {
        const auto actual = component != nullptr ? component->getBounds()
                                                 : juce::Rectangle<int>();
        std::cerr << "FAIL: control geometry drifted for " << id
                  << " expected " << expected.toString()
                  << " actual " << actual.toString() << '\n';
    }
    return passed;
}

void clickSelector (juce::Slider& slider, juce::Point<float> position)
{
    const auto now = juce::Time::getCurrentTime();
    juce::MouseEvent event (juce::Desktop::getInstance().getMainMouseSource(),
                            position,
                            juce::ModifierKeys::leftButtonModifier,
                            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                            &slider, &slider, now, position, now, 1, false);
    slider.mouseDown (event);
    slider.mouseUp (event);
}

void doubleClickSlider (juce::Slider& slider)
{
    const auto now = juce::Time::getCurrentTime();
    const auto position = slider.getLocalBounds().toFloat().getCentre();
    juce::MouseEvent event (juce::Desktop::getInstance().getMainMouseSource(),
                            position,
                            juce::ModifierKeys::leftButtonModifier,
                            0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                            &slider, &slider, now, position, now, 2, false);
    slider.mouseDoubleClick (event);
}
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    TheBureaucratAudioProcessor processor;
    processor.setPlayConfigDetails (2, 2, 48000.0, 512);
    processor.prepareToPlay (48000.0, 512);
    bool passed = verifyFactoryPrograms (processor);
    TheBureaucratAudioProcessorEditor editor (processor);
    editor.setVisible (true);
    dispatchMessages();
    editor.refreshVisualState();

    passed &= expectBounds (editor, "ironCurtain", { 74, 157, 126, 126 });
    passed &= expectBounds (editor, "queue", { 550, 168, 104, 104 });
    passed &= expectBounds (editor, "redTape", { 348, 355, 108, 108 });
    passed &= expectBounds (editor, "censor", { 562, 355, 108, 108 });
    passed &= expectBounds (editor, "output", { 839, 398, 68, 68 });
    passed &= expectBounds (editor, "loyaltyReport", { 1075, 351, 92, 92 });
    passed &= expectBounds (editor, "planFulfillment", { 339, 164, 96, 112 });
    passed &= expectBounds (editor, "gulag", { 817, 198, 112, 112 });
    passed &= expectBounds (editor, "conformityMeter", { 46, 329, 188, 126 });
    passed &= expectBounds (editor, "loyaltyMeter", { 1029, 145, 184, 124 });
    passed &= expectBounds (editor, "surveillanceLamp", { 117, 480, 44, 44 });
    passed &= expectBounds (editor, "presetMenu", { 918, 52, 176, 22 });
    passed &= expectBounds (editor, "brandInfoButton", { 145, 13, 243, 50 });

    const auto destination = argc > 1 ? juce::File (juce::String::fromUTF8 (argv[1]))
                                      : juce::File::getSpecialLocation (juce::File::tempDirectory)
                                            .getChildFile ("the-bureaucrat-ui");
    destination.createDirectory();
    passed &= saveSnapshot (editor, destination.getChildFile ("default.png"));

    editor.setSize (1600, 700);
    dispatchMessages();
    passed &= expectBounds (editor, "ironCurtain", { 92, 196, 158, 158 });
    passed &= expectBounds (editor, "queue", { 688, 210, 130, 130 });
    passed &= expectBounds (editor, "loyaltyReport", { 1344, 439, 115, 115 });
    passed &= saveSnapshot (editor, destination.getChildFile ("resized.png"));
    editor.setSize (1280, 560);
    dispatchMessages();

    auto* queue = dynamic_cast<juce::Slider*> (child (editor, "queue"));
    auto* loyalty = dynamic_cast<juce::Slider*> (child (editor, "loyaltyReport"));
    auto* plan = dynamic_cast<juce::Button*> (child (editor, "planFulfillment"));
    auto* gulag = dynamic_cast<juce::Button*> (child (editor, "gulag"));
    auto* presetMenu = dynamic_cast<juce::ComboBox*> (child (editor, "presetMenu"));
    auto* brandInfo = dynamic_cast<juce::Button*> (child (editor, "brandInfoButton"));

    if (presetMenu != nullptr)
    {
        passed &= presetMenu->getNumItems() == processor.getNumPrograms();
        presetMenu->setSelectedId (9, juce::sendNotificationSync);
        passed &= processor.getCurrentProgram() == 8;
        passed &= expectParameter (processor.getState(), BureaucratParameters::gulag, 1.0f);
        passed &= expectParameter (processor.getState(), BureaucratParameters::queue, 2.0f);
        dispatchMessages();
        passed &= saveSnapshot (editor, destination.getChildFile ("preset-siberian.png"));
        presetMenu->setSelectedId (1, juce::sendNotificationSync);
    }

    if (brandInfo != nullptr)
    {
        brandInfo->onClick();
        dispatchMessages();
        auto* overlay = child (editor, "aboutOverlay");
        passed &= overlay != nullptr && overlay->isVisible();
        passed &= saveSnapshot (editor, destination.getChildFile ("about.png"));
        if (overlay != nullptr)
            if (auto* close = dynamic_cast<juce::Button*> (overlay->findChildWithID ("aboutClose")))
                close->onClick();
        dispatchMessages();
        passed &= overlay != nullptr && ! overlay->isVisible();
    }

    if (queue != nullptr)
    {
        clickSelector (*queue, { 4.0f, 4.0f });
        passed &= expectParameter (processor.getState(), BureaucratParameters::queue, 0.0f);
        dispatchMessages();
        passed &= saveSnapshot (editor, destination.getChildFile ("queue-left.png"));
        clickSelector (*queue, { queue->getWidth() * 0.5f, 4.0f });
        passed &= expectParameter (processor.getState(), BureaucratParameters::queue, 1.0f);
        clickSelector (*queue, { static_cast<float> (queue->getWidth() - 4), 4.0f });
        doubleClickSlider (*queue);
        dispatchMessages();
        passed &= expectParameter (processor.getState(), BureaucratParameters::queue, 2.0f);
        passed &= saveSnapshot (editor, destination.getChildFile ("reset-notice.png"));
        dispatchMessages (1000);
        passed &= expectParameter (processor.getState(), BureaucratParameters::queue, 1.0f);
        clickSelector (*queue, { static_cast<float> (queue->getWidth() - 4), 4.0f });
    }
    if (loyalty != nullptr)
    {
        clickSelector (*loyalty, { static_cast<float> (loyalty->getWidth() - 4), 4.0f });
        passed &= expectParameter (processor.getState(), BureaucratParameters::loyaltyReport, 2.0f);
        clickSelector (*loyalty, { loyalty->getWidth() * 0.5f, 4.0f });
        passed &= expectParameter (processor.getState(), BureaucratParameters::loyaltyReport, 1.0f);
        dispatchMessages();
        passed &= saveSnapshot (editor, destination.getChildFile ("loyalty-centre.png"));
        clickSelector (*loyalty, { 4.0f, 4.0f });
        doubleClickSlider (*loyalty);
        dispatchMessages (1150);
        passed &= expectParameter (processor.getState(), BureaucratParameters::loyaltyReport, 2.0f);
        clickSelector (*loyalty, { 4.0f, 4.0f });
    }
    if (plan != nullptr)
        plan->setToggleState (true, juce::sendNotificationSync);
    if (gulag != nullptr)
        gulag->setToggleState (true, juce::sendNotificationSync);

    dispatchMessages();
    passed &= expectParameter (processor.getState(), BureaucratParameters::queue, 2.0f);
    passed &= expectParameter (processor.getState(), BureaucratParameters::loyaltyReport, 0.0f);
    passed &= expectParameter (processor.getState(), BureaucratParameters::planFulfillment, 1.0f);
    passed &= expectParameter (processor.getState(), BureaucratParameters::gulag, 1.0f);

    juce::AudioBuffer<float> signal (2, 512);
    for (int sample = 0; sample < signal.getNumSamples(); ++sample)
    {
        const auto value = sample % 2 == 0 ? 0.95f : -0.95f;
        signal.setSample (0, sample, value);
        signal.setSample (1, sample, value);
    }
    juce::MidiBuffer midi;
    processor.processBlock (signal, midi);
    dispatchMessages();
    editor.refreshVisualState();
    passed &= saveSnapshot (editor, destination.getChildFile ("active.png"));

    auto* iron = dynamic_cast<juce::Slider*> (child (editor, "ironCurtain"));
    auto* output = dynamic_cast<juce::Slider*> (child (editor, "output"));
    if (iron != nullptr)
        iron->setValue (7.25, juce::sendNotificationSync);
    if (output != nullptr)
        output->setValue (3.0, juce::sendNotificationSync);

    juce::MemoryBlock savedState;
    processor.getStateInformation (savedState);
    TheBureaucratAudioProcessor restoredProcessor;
    restoredProcessor.setStateInformation (savedState.getData(),
                                            static_cast<int> (savedState.getSize()));
    passed &= expectParameter (restoredProcessor.getState(), BureaucratParameters::ironCurtain, 7.25f);
    passed &= expectParameter (restoredProcessor.getState(), BureaucratParameters::queue, 2.0f);
    passed &= expectParameter (restoredProcessor.getState(), BureaucratParameters::output, 3.0f);
    passed &= expectParameter (restoredProcessor.getState(), BureaucratParameters::loyaltyReport, 0.0f);
    passed &= expectParameter (restoredProcessor.getState(), BureaucratParameters::planFulfillment, 1.0f);
    passed &= expectParameter (restoredProcessor.getState(), BureaucratParameters::gulag, 1.0f);

    juce::MemoryBlock invalidState;
    juce::MemoryOutputStream invalidStream (invalidState, false);
    juce::ValueTree ("NOT_PARAMETERS").writeToStream (invalidStream);
    restoredProcessor.setStateInformation (invalidState.getData(),
                                            static_cast<int> (invalidState.getSize()));
    restoredProcessor.setStateInformation (nullptr, 0);
    passed &= expectParameter (restoredProcessor.getState(), BureaucratParameters::ironCurtain, 7.25f);
    passed &= expectParameter (restoredProcessor.getState(), BureaucratParameters::queue, 2.0f);

    passed &= std::abs (processor.getTailLengthSeconds()
                        - BureaucratDspEngine::maximumTailSeconds) < 0.01;

    if (! passed)
        return 1;

    std::cout << "PASS: factory programs, UI scaling, selectors, state restore, and controls work\n";
    return 0;
}
