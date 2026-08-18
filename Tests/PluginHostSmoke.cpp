// SPDX-FileCopyrightText: 2026 Disconnec audio / Pravda Audio
// SPDX-License-Identifier: AGPL-3.0-only

#include <juce_audio_utils/juce_audio_utils.h>

#include <cmath>
#include <iostream>

namespace
{
constexpr double sampleRate = 48000.0;
constexpr int blockSize = 512;

bool fail (const juce::String& message)
{
    std::cerr << "FAIL: " << message << '\n';
    return false;
}

bool processAudio (juce::AudioPluginInstance& instance)
{
    instance.setPlayConfigDetails (2, 2, sampleRate, blockSize);
    instance.prepareToPlay (sampleRate, blockSize);

    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;
    int phase = 0;
    float outputPeak = 0.0f;

    for (int block = 0; block < 32; ++block)
    {
        for (int sample = 0; sample < blockSize; ++sample)
        {
            const auto value = static_cast<float> (
                0.2 * std::sin (juce::MathConstants<double>::twoPi * 220.0
                                * static_cast<double> (phase++) / sampleRate));
            buffer.setSample (0, sample, value);
            buffer.setSample (1, sample, value);
        }

        instance.processBlock (buffer, midi);
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            outputPeak = juce::jmax (
                outputPeak, buffer.getMagnitude (channel, 0, buffer.getNumSamples()));
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
                if (! std::isfinite (buffer.getSample (channel, sample)))
                    return fail ("hosted processing produced a non-finite sample");
        }
    }

    instance.releaseResources();
    return outputPeak > 0.001f || fail ("hosted processing unexpectedly muted the signal");
}

bool verifyState (juce::AudioPluginInstance& instance)
{
    const auto parameters = instance.getParameters();
    const juce::StringArray expected {
        "Iron Curtain", "Five-Year Plan: Overfulfilled", "Queue", "Red Tape",
        "Censor", "Send to Gulag", "Output", "Loyalty Report"
    };

    for (const auto& name : expected)
    {
        const auto found = std::any_of (parameters.begin(), parameters.end(),
                                       [&] (const auto* candidate)
                                       {
                                           return candidate->getName (128) == name;
                                       });
        if (! found)
            return fail ("missing hosted parameter: " + name);
    }

    auto* parameter = *std::find_if (parameters.begin(), parameters.end(),
                                    [] (const auto* candidate)
                                    {
                                        return candidate->getName (128) == "Iron Curtain";
                                    });
    const auto original = parameter->getValue();
    juce::MemoryBlock state;
    instance.getStateInformation (state);
    if (state.isEmpty())
        return fail ("plug-in returned an empty state block");

    parameter->setValueNotifyingHost (original < 0.5f ? 1.0f : 0.0f);
    instance.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
    return std::abs (parameter->getValue() - original) < 0.001f
        || fail ("plug-in state did not restore the parameter value");
}

bool verifyPrograms (juce::AudioPluginInstance& instance)
{
    if (instance.getNumPrograms() != 11)
        return fail ("host did not expose all 11 factory programs");

    const juce::StringArray expected {
        "Factory Default", "Approved Broadcast", "Iron Quota", "Breadline Shuffle",
        "Red Tape Chamber", "Censored Radio", "Five-Year Collapse", "Surveillance State",
        "Siberian Exile", "Absolute Loyalty", "Dissident Underground"
    };

    for (int index = 0; index < expected.size(); ++index)
        if (instance.getProgramName (index) != expected[index])
            return fail ("host factory program name mismatch at index " + juce::String (index));

    instance.setCurrentProgram (8);
    if (instance.getCurrentProgram() != 8)
        return fail ("host could not select the Siberian Exile program");

    instance.setCurrentProgram (0);
    return instance.getCurrentProgram() == 0
        || fail ("host could not restore the Factory Default program");
}

bool verifyEditor (juce::AudioPluginInstance& instance)
{
    std::unique_ptr<juce::AudioProcessorEditor> editor (instance.createEditorIfNeeded());
    if (editor == nullptr)
        return fail ("hosted plug-in did not create its editor");

    return (editor->getWidth() == 1280 && editor->getHeight() == 560)
        || fail ("unexpected editor size "
                 + juce::String (editor->getWidth()) + "x"
                 + juce::String (editor->getHeight()));
}

bool testFormat (juce::AudioPluginFormatManager& manager,
                 const juce::String& formatName,
                 const juce::String& path)
{
    auto* format = [&]() -> juce::AudioPluginFormat*
    {
        for (auto* candidate : manager.getFormats())
            if (candidate->getName() == formatName)
                return candidate;
        return nullptr;
    }();

    if (format == nullptr)
        return fail ("host format is unavailable: " + formatName);

    juce::OwnedArray<juce::PluginDescription> descriptions;
    format->findAllTypesForFile (descriptions, path);

    const juce::PluginDescription* description = nullptr;
    for (const auto* candidate : descriptions)
        if (candidate->name == "THE BUREAUCRAT"
            && candidate->manufacturerName == "Disconnec audio")
            description = candidate;

    if (description == nullptr)
    {
        for (const auto* candidate : descriptions)
            std::cerr << "SCAN: " << formatName << " name=" << candidate->name
                      << " manufacturer=" << candidate->manufacturerName
                      << " identifier=" << candidate->fileOrIdentifier << '\n';
        return fail (formatName + " scan did not find Disconnec audio: THE BUREAUCRAT");
    }

    juce::String error;
    auto instance = manager.createPluginInstance (*description, sampleRate, blockSize, error);
    if (instance == nullptr)
        return fail (formatName + " failed to instantiate: " + error);

    const auto passed = verifyPrograms (*instance)
                     && verifyState (*instance)
                     && processAudio (*instance)
                     && verifyEditor (*instance);
    if (passed)
        std::cout << "PASS: " << formatName << " scanned, instantiated, processed, "
                  << "restored state, and created its editor\n";
    return passed;
}
}

int main (int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: TheBureaucratPluginHostSmoke <VST3 bundle>\n";
        return 2;
    }

    juce::ScopedJuceInitialiser_GUI gui;
    juce::AudioPluginFormatManager manager;
    juce::addDefaultFormatsToManager (manager);

    return testFormat (manager, "VST3", juce::String::fromUTF8 (argv[1])) ? 0 : 1;
}
