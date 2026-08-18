// SPDX-FileCopyrightText: 2026 Disconnec audio / Pravda Audio
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

#include "DspEngine.h"

class TheBureaucratAudioProcessor final : public juce::AudioProcessor
{
public:
    TheBureaucratAudioProcessor();
    ~TheBureaucratAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override { dsp.reset(); }
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "THE BUREAUCRAT"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return BureaucratDspEngine::maximumTailSeconds; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getState() noexcept { return parameters; }
    const juce::AudioProcessorValueTreeState& getState() const noexcept { return parameters; }

    float getConformityMeter() const noexcept { return dsp.getConformityMeter(); }
    bool isSurveillanceActive() const noexcept { return dsp.isSurveillanceActive(); }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    BureaucratDspEngine::Parameters readParameters() const noexcept;

    juce::AudioProcessorValueTreeState parameters;
    BureaucratDspEngine dsp;
    std::atomic<int> currentProgram { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TheBureaucratAudioProcessor)
};
