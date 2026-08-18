// SPDX-FileCopyrightText: 2026 Disconnec audio / Pravda Audio
// SPDX-License-Identifier: AGPL-3.0-only

#include "DspEngine.h"

#include <cmath>

namespace
{
constexpr auto twoPi = juce::MathConstants<float>::twoPi;
constexpr std::array<float, 3> queueWetAmounts { 0.08f, 0.24f, 0.46f };

float exponentialMap (float normalised, float low, float high) noexcept
{
    return low * std::pow (high / low, juce::jlimit (0.0f, 1.0f, normalised));
}

float queueWetTarget (int choice, bool overFulfilled) noexcept
{
    const auto index = static_cast<std::size_t> (juce::jlimit (0, 2, choice));
    return juce::jlimit (0.0f, 0.72f,
                        queueWetAmounts[index] * (overFulfilled ? 1.25f : 1.0f));
}
}

void BureaucratDspEngine::prepare (double sampleRate, int maximumBlockSize, int channels)
{
    juce::ignoreUnused (maximumBlockSize);
    currentSampleRate = juce::jmax (8000.0, sampleRate);
    preparedChannels = juce::jlimit (1, 2, channels);

    const auto queueSize = static_cast<std::size_t> (std::ceil (currentSampleRate * 0.65)) + 4u;
    const auto gulagSize = static_cast<std::size_t> (std::ceil (currentSampleRate * 1.6)) + 4u;

    for (auto& line : queueLines)
        line.assign (queueSize, 0.0f);
    for (auto& line : gulagLines)
        line.assign (gulagSize, 0.0f);

    ironSmoother.reset (currentSampleRate, 0.025);
    redTapeSmoother.reset (currentSampleRate, 0.030);
    censorSmoother.reset (currentSampleRate, 0.035);
    queueWetSmoother.reset (currentSampleRate, 0.040);
    outputGainSmoother.reset (currentSampleRate, 0.030);
    gulagAmountSmoother.reset (currentSampleRate, 0.090);
    surveillanceMixSmoother.reset (currentSampleRate, 0.045);
    loyaltyAmountSmoother.reset (currentSampleRate, 0.045);

    ironSmoother.setCurrentAndTargetValue (4.0f);
    redTapeSmoother.setCurrentAndTargetValue (0.35f);
    censorSmoother.setCurrentAndTargetValue (0.28f);
    queueWetSmoother.setCurrentAndTargetValue (queueWetAmounts[1]);
    outputGainSmoother.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (-2.0f));
    gulagAmountSmoother.setCurrentAndTargetValue (0.0f);
    surveillanceMixSmoother.setCurrentAndTargetValue (0.0f);
    loyaltyAmountSmoother.setCurrentAndTargetValue (0.0f);
    reset();
}

void BureaucratDspEngine::reset()
{
    for (auto& line : queueLines)
        std::fill (line.begin(), line.end(), 0.0f);
    for (auto& line : gulagLines)
        std::fill (line.begin(), line.end(), 0.0f);

    channelStates = {};
    channelStates[0].randomState = 0x13579bdfu;
    channelStates[1].randomState = 0x2468ace0u;
    channelStates[0].queueDelaySamples = channelStates[0].queueTargetSamples = static_cast<float> (currentSampleRate * 0.014);
    channelStates[1].queueDelaySamples = channelStates[1].queueTargetSamples = static_cast<float> (currentSampleRate * 0.018);

    surveillanceHoldSamples = 0;
    conformityMeter.store (0.0f);
    surveillanceActive.store (false);
}

float BureaucratDspEngine::nextNoise (juce::uint32& state) noexcept
{
    state = state * 1664525u + 1013904223u;
    return static_cast<float> ((state >> 8u) & 0x00ffffffu) / 8388608.0f - 1.0f;
}

float BureaucratDspEngine::saturate (float input, float iron) noexcept
{
    const auto normalisedIron = juce::jlimit (0.0f, 1.0f, iron / 11.0f);
    const auto drive = 1.0f + iron * 0.62f;
    const auto bias = 0.028f * normalisedIron;
    const auto shaped = std::tanh ((input + bias + 0.035f * std::sin (input * 3.1f)) * drive);
    const auto normaliser = 1.0f / std::tanh (drive);
    return (shaped * normaliser - std::tanh (bias * drive) * normaliser) * 0.92f;
}

float BureaucratDspEngine::onePoleCoefficient (float cutoff, double sampleRate) noexcept
{
    const auto safeCutoff = juce::jlimit (10.0f, static_cast<float> (sampleRate * 0.45), cutoff);
    return 1.0f - std::exp (-twoPi * safeCutoff / static_cast<float> (sampleRate));
}

float BureaucratDspEngine::readFractionalDelay (const std::vector<float>& line, float readPosition) noexcept
{
    const auto size = static_cast<int> (line.size());
    if (size < 2)
        return 0.0f;

    while (readPosition < 0.0f)
        readPosition += static_cast<float> (size);
    while (readPosition >= static_cast<float> (size))
        readPosition -= static_cast<float> (size);

    const auto indexA = static_cast<int> (readPosition);
    const auto indexB = (indexA + 1) % size;
    const auto fraction = readPosition - static_cast<float> (indexA);
    return line[static_cast<std::size_t> (indexA)] * (1.0f - fraction)
         + line[static_cast<std::size_t> (indexB)] * fraction;
}

float BureaucratDspEngine::processQueue (int channel, float input, int queueChoice,
                                         bool overFulfilled, float wetAmount) noexcept
{
    auto& state = channelStates[static_cast<std::size_t> (channel)];
    auto& line = queueLines[static_cast<std::size_t> (channel)];
    if (line.empty())
        return input;

    static constexpr std::array<float, 3> baseMilliseconds { 7.0f, 24.0f, 76.0f };
    static constexpr std::array<float, 3> jitterMilliseconds { 2.0f, 17.0f, 64.0f };
    const auto choice = juce::jlimit (0, 2, queueChoice);

    const auto retargetInterval = juce::jmax (32, static_cast<int> (currentSampleRate * (0.035 + choice * 0.018)));
    if (++state.queueRetargetCounter >= retargetInterval)
    {
        state.queueRetargetCounter = 0;
        const auto random01 = nextNoise (state.randomState) * 0.5f + 0.5f;
        const auto chaos = overFulfilled ? 1.75f : 1.0f;
        const auto milliseconds = baseMilliseconds[static_cast<std::size_t> (choice)]
                                + random01 * jitterMilliseconds[static_cast<std::size_t> (choice)] * chaos;
        state.queueTargetSamples = static_cast<float> (currentSampleRate * milliseconds * 0.001);
    }

    state.queueDelaySamples += (state.queueTargetSamples - state.queueDelaySamples) * 0.0009f;
    const auto readPosition = static_cast<float> (state.queueWritePosition) - state.queueDelaySamples;
    const auto delayed = readFractionalDelay (line, readPosition);
    line[static_cast<std::size_t> (state.queueWritePosition)] = input;
    state.queueWritePosition = (state.queueWritePosition + 1) % static_cast<int> (line.size());

    return input + (delayed - input) * juce::jlimit (0.0f, 0.72f, wetAmount);
}

float BureaucratDspEngine::processGulagRead (int channel) const noexcept
{
    const auto& line = gulagLines[static_cast<std::size_t> (channel)];
    const auto& state = channelStates[static_cast<std::size_t> (channel)];
    if (line.empty())
        return 0.0f;

    const auto delaySeconds = channel == 0 ? 0.84 : 1.17;
    const auto delaySamples = static_cast<int> (currentSampleRate * delaySeconds);
    const auto size = static_cast<int> (line.size());
    const auto readPosition = (state.gulagWritePosition - delaySamples + size) % size;
    return line[static_cast<std::size_t> (readPosition)];
}

void BureaucratDspEngine::writeGulag (int channel, float input, float localDelayed,
                                     float crossDelayed, float amount) noexcept
{
    auto& line = gulagLines[static_cast<std::size_t> (channel)];
    auto& state = channelStates[static_cast<std::size_t> (channel)];
    if (line.empty())
        return;

    const auto lowCoefficient = onePoleCoefficient (3600.0f, currentSampleRate);
    state.gulagLowPass += lowCoefficient * (localDelayed - state.gulagLowPass);

    const auto highCoefficient = onePoleCoefficient (170.0f, currentSampleRate);
    state.gulagHighPassMemory += highCoefficient * (state.gulagLowPass - state.gulagHighPassMemory);
    const auto filtered = state.gulagLowPass - state.gulagHighPassMemory;

    const auto feedback = (filtered * 0.91f + crossDelayed * 0.055f) * amount;
    line[static_cast<std::size_t> (state.gulagWritePosition)] = juce::jlimit (-1.25f, 1.25f, input + feedback);
    state.gulagWritePosition = (state.gulagWritePosition + 1) % static_cast<int> (line.size());
}

void BureaucratDspEngine::process (juce::AudioBuffer<float>& buffer,
                                   const Parameters& parameters) noexcept
{
    const auto channels = juce::jmin (preparedChannels, buffer.getNumChannels());
    const auto samples = buffer.getNumSamples();
    if (channels <= 0 || samples <= 0)
        return;

    ironSmoother.setTargetValue (juce::jlimit (0.0f, 11.0f, parameters.ironCurtain));
    redTapeSmoother.setTargetValue (juce::jlimit (0.0f, 1.0f, parameters.redTape / 100.0f));
    censorSmoother.setTargetValue (juce::jlimit (0.0f, 1.0f, parameters.censor / 100.0f));
    queueWetSmoother.setTargetValue (queueWetTarget (parameters.queue, parameters.overFulfilled));
    outputGainSmoother.setTargetValue (juce::Decibels::decibelsToGain (parameters.outputDb));
    gulagAmountSmoother.setTargetValue (parameters.gulag ? 1.0f : 0.0f);
    const auto loyaltyChoice = juce::jlimit (0, 2, parameters.loyaltyReport);
    loyaltyAmountSmoother.setTargetValue (static_cast<float> (2 - loyaltyChoice) * 0.5f);

    float inputPeak = 0.0f;
    for (int channel = 0; channel < channels; ++channel)
    {
        const auto* read = buffer.getReadPointer (channel);
        for (int sample = 0; sample < samples; ++sample)
            inputPeak = juce::jmax (inputPeak, std::abs (read[sample]));
    }

    if (inputPeak > 0.82f || parameters.ironCurtain > 8.8f || parameters.overFulfilled)
        surveillanceHoldSamples = static_cast<int> (currentSampleRate * 0.24);
    else
        surveillanceHoldSamples = juce::jmax (0, surveillanceHoldSamples - samples);

    const auto watched = surveillanceHoldSamples > 0;
    surveillanceActive.store (watched);
    surveillanceMixSmoother.setTargetValue (watched ? 1.0f : 0.0f);

    std::array<float*, 2> writePointers { nullptr, nullptr };
    for (int channel = 0; channel < channels; ++channel)
        writePointers[static_cast<std::size_t> (channel)] = buffer.getWritePointer (channel);

    for (int sample = 0; sample < samples; ++sample)
    {
        const auto iron = ironSmoother.getNextValue();
        const auto redTape = redTapeSmoother.getNextValue();
        const auto censor = censorSmoother.getNextValue();
        const auto queueWet = queueWetSmoother.getNextValue();
        const auto outputGain = outputGainSmoother.getNextValue();
        const auto gulagAmount = gulagAmountSmoother.getNextValue();
        const auto watchMix = surveillanceMixSmoother.getNextValue();
        const auto loyaltyAmount = loyaltyAmountSmoother.getNextValue();
        const auto inputGain = juce::Decibels::decibelsToGain (-3.0f + iron * 1.75f);

        std::array<float, 2> processed { 0.0f, 0.0f };
        std::array<float, 2> delayed { 0.0f, 0.0f };

        for (int channel = 0; channel < channels; ++channel)
        {
            auto& state = channelStates[static_cast<std::size_t> (channel)];
            const auto dry = writePointers[static_cast<std::size_t> (channel)][sample];
            auto value = saturate (dry * inputGain, iron);

            if (parameters.overFulfilled)
            {
                const auto factor = 2 + juce::roundToInt (iron / 11.0f * 6.0f);
                if (state.decimationCounter == 0)
                    state.heldSample = value;
                state.decimationCounter = (state.decimationCounter + 1) % factor;

                const auto bits = juce::jlimit (5, 10, 10 - juce::roundToInt (iron / 11.0f * 5.0f));
                const auto levels = static_cast<float> (1 << bits);
                value = std::round (state.heldSample * levels) / levels;
                value += nextNoise (state.randomState) * 0.0065f;
            }

            value = processQueue (channel, value, parameters.queue,
                                  parameters.overFulfilled, queueWet);

            const auto allPassCoefficient = 0.18f + redTape * 0.62f;
            const auto allPass = -allPassCoefficient * value + state.allPassMemory;
            state.allPassMemory = value + allPassCoefficient * allPass;
            const auto paperwork = allPass - dry * (0.08f + redTape * 0.22f);
            value += (paperwork - value) * redTape;

            const auto cutoff = exponentialMap (censor, 18000.0f, 620.0f);
            state.censorLowPass += onePoleCoefficient (cutoff, currentSampleRate)
                                   * (value - state.censorLowPass);
            value = state.censorLowPass;

            state.watchHighPassMemory += onePoleCoefficient (120.0f, currentSampleRate)
                                         * (value - state.watchHighPassMemory);
            const auto highPassed = value - state.watchHighPassMemory;
            state.watchLowPass += onePoleCoefficient (4400.0f, currentSampleRate)
                                  * (highPassed - state.watchLowPass);
            value += (state.watchLowPass - value) * (watchMix * 0.88f);

            const auto loyaltyLowCutoff = exponentialMap (loyaltyAmount, 18000.0f, 950.0f);
            state.loyaltyLowPass += onePoleCoefficient (loyaltyLowCutoff, currentSampleRate)
                                  * (value - state.loyaltyLowPass);
            const auto loyaltyHighCutoff = 28.0f + loyaltyAmount * 152.0f;
            state.loyaltyHighPassMemory += onePoleCoefficient (loyaltyHighCutoff, currentSampleRate)
                                         * (state.loyaltyLowPass - state.loyaltyHighPassMemory);
            const auto restricted = state.loyaltyLowPass - state.loyaltyHighPassMemory;
            const auto loyaltyDrive = 1.0f + loyaltyAmount * 2.5f;
            const auto reEducated = std::tanh (restricted * loyaltyDrive)
                                  / loyaltyDrive;
            value += (reEducated - value) * loyaltyAmount;

            const auto dcBlocked = value - state.dcInput + 0.995f * state.dcOutput;
            state.dcInput = value;
            state.dcOutput = dcBlocked;
            processed[static_cast<std::size_t> (channel)] = dcBlocked;
            delayed[static_cast<std::size_t> (channel)] = processGulagRead (channel);
        }

        for (int channel = 0; channel < channels; ++channel)
        {
            const auto other = channels > 1 ? 1 - channel : channel;
            writeGulag (channel, processed[static_cast<std::size_t> (channel)],
                        delayed[static_cast<std::size_t> (channel)],
                        delayed[static_cast<std::size_t> (other)], gulagAmount);

            auto value = processed[static_cast<std::size_t> (channel)] * (1.0f - gulagAmount)
                       + delayed[static_cast<std::size_t> (channel)] * gulagAmount;
            value *= outputGain;
            value = std::tanh (value * 1.04f) / std::tanh (1.04f);
            writePointers[static_cast<std::size_t> (channel)][sample] = value;
        }
    }

    for (int channel = channels; channel < buffer.getNumChannels(); ++channel)
        buffer.clear (channel, 0, samples);

    const auto release = std::exp (-static_cast<float> (samples) / static_cast<float> (currentSampleRate * 0.28));
    conformityMeter.store (juce::jmax (inputPeak, conformityMeter.load() * release));
}
