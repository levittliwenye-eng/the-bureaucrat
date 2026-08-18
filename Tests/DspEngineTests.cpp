// SPDX-FileCopyrightText: 2026 Disconnec audio / Pravda Audio
// SPDX-License-Identifier: AGPL-3.0-only

#include "DspEngine.h"

#include <cmath>
#include <iostream>

namespace
{
constexpr double sampleRate = 48000.0;
constexpr int blockSize = 512;

void fillSine (juce::AudioBuffer<float>& buffer, float amplitude, float frequency,
               int& phaseSample, double rate = sampleRate)
{
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto value = amplitude * std::sin (juce::MathConstants<double>::twoPi
                                                  * frequency * phaseSample++ / rate);
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.setSample (channel, sample, static_cast<float> (value));
    }
}

bool isFinite (const juce::AudioBuffer<float>& buffer)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            if (! std::isfinite (buffer.getSample (channel, sample)))
                return false;
    return true;
}

float peak (const juce::AudioBuffer<float>& buffer)
{
    float result = 0.0f;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        result = juce::jmax (result, buffer.getMagnitude (channel, 0, buffer.getNumSamples()));
    return result;
}

float renderLoyaltyBand (int choice)
{
    BureaucratDspEngine engine;
    engine.prepare (sampleRate, blockSize, 2);

    BureaucratDspEngine::Parameters parameters;
    parameters.ironCurtain = 0.0f;
    parameters.queue = 0;
    parameters.redTape = 0.0f;
    parameters.censor = 0.0f;
    parameters.outputDb = 0.0f;
    parameters.loyaltyReport = choice;

    juce::AudioBuffer<float> buffer (2, blockSize);
    int phaseSample = 0;
    for (int block = 0; block < 32; ++block)
    {
        fillSine (buffer, 0.22f, 6000.0f, phaseSample);
        engine.process (buffer, parameters);
    }

    return buffer.getRMSLevel (0, 0, buffer.getNumSamples());
}

float queueSwitchJump()
{
    BureaucratDspEngine engine;
    engine.prepare (sampleRate, blockSize, 2);

    BureaucratDspEngine::Parameters parameters;
    parameters.ironCurtain = 0.0f;
    parameters.queue = 0;
    parameters.redTape = 0.0f;
    parameters.censor = 0.0f;
    parameters.outputDb = 0.0f;
    parameters.loyaltyReport = 2;

    juce::AudioBuffer<float> buffer (2, blockSize);
    int phaseSample = 0;
    for (int block = 0; block < 48; ++block)
    {
        fillSine (buffer, 0.22f, 330.0f, phaseSample);
        engine.process (buffer, parameters);
    }

    const auto previousSample = buffer.getSample (0, blockSize - 1);
    parameters.queue = 2;
    fillSine (buffer, 0.22f, 330.0f, phaseSample);
    engine.process (buffer, parameters);
    return std::abs (buffer.getSample (0, 0) - previousSample);
}

double measureGulagTailSeconds (float& finalPeak)
{
    constexpr auto tailBlockSize = 4096;
    BureaucratDspEngine engine;
    engine.prepare (sampleRate, tailBlockSize, 1);

    BureaucratDspEngine::Parameters parameters;
    parameters.ironCurtain = 0.0f;
    parameters.queue = 0;
    parameters.redTape = 0.0f;
    parameters.censor = 0.0f;
    parameters.gulag = true;
    parameters.outputDb = 0.0f;
    parameters.loyaltyReport = 2;

    juce::AudioBuffer<float> buffer (1, tailBlockSize);
    buffer.clear();
    buffer.setSample (0, 0, 0.8f);
    engine.process (buffer, parameters);

    double lastAudibleSeconds = 0.0;
    const auto maximumBlocks = static_cast<int> (std::ceil (
        BureaucratDspEngine::maximumTailSeconds * sampleRate / tailBlockSize));
    for (int block = 1; block <= maximumBlocks; ++block)
    {
        buffer.clear();
        engine.process (buffer, parameters);
        finalPeak = peak (buffer);
        if (finalPeak > 0.0001f)
            lastAudibleSeconds = block * tailBlockSize / sampleRate;
    }

    return lastAudibleSeconds;
}

bool stressSampleRate (double rate, int testBlockSize)
{
    BureaucratDspEngine engine;
    engine.prepare (rate, testBlockSize, 2);

    BureaucratDspEngine::Parameters parameters;
    parameters.ironCurtain = 11.0f;
    parameters.overFulfilled = true;
    parameters.queue = 2;
    parameters.redTape = 100.0f;
    parameters.censor = 100.0f;
    parameters.gulag = true;
    parameters.outputDb = 12.0f;
    parameters.loyaltyReport = 0;

    juce::AudioBuffer<float> buffer (2, testBlockSize);
    int phaseSample = 0;
    for (int block = 0; block < 12; ++block)
    {
        fillSine (buffer, 0.72f, 87.0f, phaseSample, rate);
        engine.process (buffer, parameters);
        if (! isFinite (buffer))
            return false;
    }

    return true;
}

bool expect (bool condition, const char* message)
{
    if (! condition)
        std::cerr << "FAIL: " << message << '\n';
    return condition;
}
}

int main()
{
    BureaucratDspEngine engine;
    engine.prepare (sampleRate, blockSize, 2);
    juce::AudioBuffer<float> buffer (2, blockSize);
    int phaseSample = 0;
    bool passed = true;

    BureaucratDspEngine::Parameters normal;
    fillSine (buffer, 0.18f, 220.0f, phaseSample);
    engine.process (buffer, normal);
    passed &= expect (isFinite (buffer), "default processing produced a non-finite sample");
    passed &= expect (peak (buffer) > 0.001f, "default processing unexpectedly muted the signal");

    BureaucratDspEngine::Parameters extreme;
    extreme.ironCurtain = 11.0f;
    extreme.overFulfilled = true;
    extreme.queue = 2;
    extreme.redTape = 100.0f;
    extreme.censor = 100.0f;
    extreme.gulag = true;
    extreme.outputDb = 12.0f;
    extreme.loyaltyReport = 0;

    for (int block = 0; block < 220; ++block)
    {
        fillSine (buffer, 0.72f, 87.0f, phaseSample);
        engine.process (buffer, extreme);
        passed &= expect (isFinite (buffer), "extreme processing produced a non-finite sample");
        if (! passed)
            break;
    }

    passed &= expect (engine.isSurveillanceActive(), "surveillance did not activate under overload");

    const auto reEducateLevel = renderLoyaltyBand (0);
    const auto underReviewLevel = renderLoyaltyBand (1);
    const auto absoluteLoyaltyLevel = renderLoyaltyBand (2);
    passed &= expect (reEducateLevel < underReviewLevel * 0.75f,
                      "re-education did not narrow the signal more than under review");
    passed &= expect (underReviewLevel < absoluteLoyaltyLevel * 0.85f,
                      "under review did not audibly differ from absolute loyalty");
    passed &= expect (queueSwitchJump() < 0.05f,
                      "Queue detent change produced a discontinuity");

    bool heardTail = false;
    for (int block = 0; block < 180; ++block)
    {
        buffer.clear();
        engine.process (buffer, extreme);
        heardTail = heardTail || peak (buffer) > 0.0001f;
        passed &= expect (isFinite (buffer), "Gulag tail produced a non-finite sample");
    }
    passed &= expect (heardTail, "Gulag delay produced no audible tail");

    float finalTailPeak = 0.0f;
    const auto measuredTailSeconds = measureGulagTailSeconds (finalTailPeak);
    passed &= expect (measuredTailSeconds > 12.0,
                      "Gulag tail no longer extends beyond the former 12-second report");
    passed &= expect (measuredTailSeconds < BureaucratDspEngine::maximumTailSeconds
                      && finalTailPeak < 0.0001f,
                      "Gulag tail outlived the duration reported to the host");

    passed &= expect (stressSampleRate (44100.0, 64), "44.1 kHz stress path failed");
    passed &= expect (stressSampleRate (48000.0, 512), "48 kHz stress path failed");
    passed &= expect (stressSampleRate (96000.0, 137), "96 kHz stress path failed");
    passed &= expect (stressSampleRate (192000.0, 4096), "192 kHz stress path failed");

    if (! passed)
        return 1;

    std::cout << "PASS: DSP defaults, rates, smoothed selectors, surveillance, and long Gulag tail\n";
    return 0;
}
