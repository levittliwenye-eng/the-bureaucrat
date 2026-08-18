// SPDX-FileCopyrightText: 2026 Disconnec audio / Pravda Audio
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <array>
#include <atomic>
#include <vector>

class BureaucratDspEngine
{
public:
    static constexpr double maximumTailSeconds = 180.0;

    struct Parameters
    {
        float ironCurtain = 4.0f;
        bool overFulfilled = false;
        int queue = 1;
        float redTape = 35.0f;
        float censor = 28.0f;
        bool gulag = false;
        float outputDb = -2.0f;
        int loyaltyReport = 2;
    };

    void prepare (double sampleRate, int maximumBlockSize, int channels);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, const Parameters& parameters) noexcept;

    float getConformityMeter() const noexcept { return conformityMeter.load(); }
    bool isSurveillanceActive() const noexcept { return surveillanceActive.load(); }

private:
    struct ChannelState
    {
        float dcInput = 0.0f;
        float dcOutput = 0.0f;
        float allPassMemory = 0.0f;
        float censorLowPass = 0.0f;
        float watchHighPassMemory = 0.0f;
        float watchLowPass = 0.0f;
        float loyaltyLowPass = 0.0f;
        float loyaltyHighPassMemory = 0.0f;
        float gulagLowPass = 0.0f;
        float gulagHighPassMemory = 0.0f;
        float heldSample = 0.0f;
        int decimationCounter = 0;
        int queueWritePosition = 0;
        int gulagWritePosition = 0;
        int queueRetargetCounter = 0;
        float queueDelaySamples = 240.0f;
        float queueTargetSamples = 240.0f;
        juce::uint32 randomState = 0x12345678u;
    };

    static float saturate (float input, float iron) noexcept;
    static float nextNoise (juce::uint32& state) noexcept;
    static float onePoleCoefficient (float cutoff, double sampleRate) noexcept;
    static float readFractionalDelay (const std::vector<float>& line, float readPosition) noexcept;

    float processQueue (int channel, float input, int queueChoice, bool overFulfilled,
                        float wetAmount) noexcept;
    float processGulagRead (int channel) const noexcept;
    void writeGulag (int channel, float input, float localDelayed, float crossDelayed,
                     float amount) noexcept;

    std::array<ChannelState, 2> channelStates;
    std::array<std::vector<float>, 2> queueLines;
    std::array<std::vector<float>, 2> gulagLines;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> ironSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> redTapeSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> censorSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> queueWetSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> outputGainSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> gulagAmountSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> surveillanceMixSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> loyaltyAmountSmoother;

    std::atomic<float> conformityMeter { 0.0f };
    std::atomic<bool> surveillanceActive { false };

    double currentSampleRate = 44100.0;
    int preparedChannels = 2;
    int surveillanceHoldSamples = 0;
};
