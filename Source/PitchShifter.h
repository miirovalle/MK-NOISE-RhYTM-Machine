#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <vector>

class PitchShifter
{
public:
    void prepare (double sampleRate)
    {
        window = static_cast<float> (sampleRate * 0.06);
        history.assign (static_cast<size_t> (std::ceil (window)) + 8, 0.0f);
        writePosition = 0;
        phase = 0.0f;
        activeMix.reset (sampleRate, 0.01);
        activeMix.setCurrentAndTargetValue (0.0f);
        antiAliasState = 0.0f;
        rate = static_cast<float> (sampleRate);
    }

    float process (float input, float semitones) noexcept
    {
        const auto active = std::abs (semitones) > 0.001f;
        activeMix.setTargetValue (active ? 1.0f : 0.0f);
        if (! active && ! activeMix.isSmoothing())
        {
            history[static_cast<size_t> (writePosition)] = antiAliasState = input;
            if (++writePosition == static_cast<int> (history.size()))
                writePosition = 0;
            return input;
        }
        const auto ratio = std::pow (2.0f, semitones / 12.0f);
        const auto cutoff = rate * 0.45f / juce::jmax (1.0f, ratio);
        const auto coefficient = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * cutoff / rate);
        antiAliasState += coefficient * (input - antiAliasState);
        history[static_cast<size_t> (writePosition)] = ratio > 1.0f ? antiAliasState : input;

        const auto secondPhase = phase < 0.5f ? phase + 0.5f : phase - 0.5f;
        const auto weight = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * phase);
        const auto shifted = weight * read (4.0f + window * phase)
                           + (1.0f - weight) * read (4.0f + window * secondPhase);
        phase += (1.0f - ratio) / window;
        phase -= std::floor (phase);
        if (++writePosition == static_cast<int> (history.size()))
            writePosition = 0;
        return input + activeMix.getNextValue() * (shifted - input);
    }

private:
    float read (float delay) const noexcept
    {
        auto position = static_cast<float> (writePosition) - delay;
        if (position < 0.0f)
            position += static_cast<float> (history.size());
        const auto first = static_cast<size_t> (position) % history.size();
        const auto fraction = position - std::floor (position);
        return history[first] + fraction * (history[(first + 1) % history.size()] - history[first]);
    }

    std::vector<float> history;
    juce::SmoothedValue<float> activeMix;
    int writePosition = 0;
    float phase = 0.0f, window = 2646.0f, rate = 44100.0f, antiAliasState = 0.0f;
};
