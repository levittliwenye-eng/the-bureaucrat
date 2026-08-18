// SPDX-FileCopyrightText: 2026 Disconnec audio / Pravda Audio
// SPDX-License-Identifier: AGPL-3.0-only

#include "PluginProcessor.h"
#include "ParameterIds.h"
#include "PluginEditor.h"

#include <array>

namespace
{
struct FactoryPreset
{
    const char* name;
    float ironCurtain;
    float planFulfillment;
    float queue;
    float redTape;
    float censor;
    float gulag;
    float output;
    float loyaltyReport;
};

constexpr std::array factoryPresets {
    FactoryPreset { "Factory Default",       4.0f, 0.0f, 1.0f, 35.0f, 28.0f, 0.0f,  -2.0f, 2.0f },
    FactoryPreset { "Approved Broadcast",    2.6f, 0.0f, 0.0f, 12.0f, 12.0f, 0.0f,  -1.0f, 2.0f },
    FactoryPreset { "Iron Quota",            8.4f, 0.0f, 0.0f, 28.0f, 18.0f, 0.0f,  -5.0f, 2.0f },
    FactoryPreset { "Breadline Shuffle",     3.5f, 0.0f, 2.0f, 42.0f, 20.0f, 0.0f,  -3.0f, 2.0f },
    FactoryPreset { "Red Tape Chamber",      4.2f, 0.0f, 1.0f, 78.0f, 18.0f, 0.0f,  -4.0f, 2.0f },
    FactoryPreset { "Censored Radio",        5.0f, 0.0f, 0.0f, 35.0f, 78.0f, 0.0f,  -3.0f, 1.0f },
    FactoryPreset { "Five-Year Collapse",    7.2f, 1.0f, 1.0f, 55.0f, 48.0f, 0.0f,  -7.0f, 1.0f },
    FactoryPreset { "Surveillance State",    9.2f, 1.0f, 0.0f, 62.0f, 70.0f, 0.0f,  -9.0f, 0.0f },
    FactoryPreset { "Siberian Exile",        3.8f, 0.0f, 2.0f, 52.0f, 42.0f, 1.0f,  -6.0f, 1.0f },
    FactoryPreset { "Absolute Loyalty",      2.0f, 0.0f, 0.0f,  8.0f,  8.0f, 0.0f,   0.0f, 2.0f },
    FactoryPreset { "Dissident Underground", 6.5f, 1.0f, 2.0f, 70.0f, 60.0f, 1.0f, -10.0f, 0.0f }
};

constexpr auto currentProgramProperty = "factoryProgram";

float valueOf (const juce::AudioProcessorValueTreeState& state, const char* id) noexcept
{
    if (const auto* value = state.getRawParameterValue (id))
        return value->load();

    return 0.0f;
}

void setPlainValue (juce::AudioProcessorValueTreeState& state, const char* id, float value)
{
    if (auto* parameter = state.getParameter (id))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
}
}

TheBureaucratAudioProcessor::TheBureaucratAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout TheBureaucratAudioProcessor::createParameterLayout()
{
    using namespace BureaucratParameters;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> result;

    result.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ironCurtain, 1 }, "Iron Curtain",
        juce::NormalisableRange<float> (0.0f, 11.0f, 0.01f), 4.0f));
    result.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { planFulfillment, 1 }, "Five-Year Plan: Overfulfilled", false));
    result.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { queue, 1 }, "Queue",
        juce::StringArray { "Promising", "Indefinite", "Come Back Tomorrow" }, 1));
    result.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { redTape, 1 }, "Red Tape",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 35.0f));
    result.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { censor, 1 }, "Censor",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 28.0f));
    result.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { gulag, 1 }, "Send to Gulag", false));
    result.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { output, 1 }, "Output",
        juce::NormalisableRange<float> (-18.0f, 12.0f, 0.01f), -2.0f));
    result.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { loyaltyReport, 1 }, "Loyalty Report",
        juce::StringArray { "Re-Education", "Under Review", "Absolute Loyalty" }, 2));

    return { result.begin(), result.end() };
}

int TheBureaucratAudioProcessor::getNumPrograms()
{
    return static_cast<int> (factoryPresets.size());
}

int TheBureaucratAudioProcessor::getCurrentProgram()
{
    return currentProgram.load();
}

void TheBureaucratAudioProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, getNumPrograms()))
        return;

    const auto& preset = factoryPresets[static_cast<std::size_t> (index)];
    currentProgram.store (index);

    using namespace BureaucratParameters;
    setPlainValue (parameters, ironCurtain, preset.ironCurtain);
    setPlainValue (parameters, planFulfillment, preset.planFulfillment);
    setPlainValue (parameters, queue, preset.queue);
    setPlainValue (parameters, redTape, preset.redTape);
    setPlainValue (parameters, censor, preset.censor);
    setPlainValue (parameters, gulag, preset.gulag);
    setPlainValue (parameters, output, preset.output);
    setPlainValue (parameters, loyaltyReport, preset.loyaltyReport);
}

const juce::String TheBureaucratAudioProcessor::getProgramName (int index)
{
    if (! juce::isPositiveAndBelow (index, getNumPrograms()))
        return {};

    return factoryPresets[static_cast<std::size_t> (index)].name;
}

void TheBureaucratAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    dsp.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
}

bool TheBureaucratAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    return input == output
        && (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo());
}

BureaucratDspEngine::Parameters TheBureaucratAudioProcessor::readParameters() const noexcept
{
    using namespace BureaucratParameters;
    BureaucratDspEngine::Parameters snapshot;
    snapshot.ironCurtain = valueOf (parameters, ironCurtain);
    snapshot.overFulfilled = valueOf (parameters, planFulfillment) >= 0.5f;
    snapshot.queue = juce::roundToInt (valueOf (parameters, queue));
    snapshot.redTape = valueOf (parameters, redTape);
    snapshot.censor = valueOf (parameters, censor);
    snapshot.gulag = valueOf (parameters, gulag) >= 0.5f;
    snapshot.outputDb = valueOf (parameters, output);
    snapshot.loyaltyReport = juce::roundToInt (valueOf (parameters, loyaltyReport));
    return snapshot;
}

void TheBureaucratAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());

    dsp.process (buffer, readParameters());
}

juce::AudioProcessorEditor* TheBureaucratAudioProcessor::createEditor()
{
    return new TheBureaucratAudioProcessorEditor (*this);
}

void TheBureaucratAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    state.setProperty (currentProgramProperty, currentProgram.load(), nullptr);
    juce::MemoryOutputStream stream (destData, false);
    state.writeToStream (stream);
}

void TheBureaucratAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0)
        return;

    const auto state = juce::ValueTree::readFromData (data, static_cast<std::size_t> (sizeInBytes));
    if (state.isValid() && state.hasType (parameters.state.getType()))
    {
        parameters.replaceState (state);
        const auto restoredProgram = static_cast<int> (state.getProperty (currentProgramProperty, 0));
        currentProgram.store (juce::jlimit (0, getNumPrograms() - 1, restoredProgram));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TheBureaucratAudioProcessor();
}
