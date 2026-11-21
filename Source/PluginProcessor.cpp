#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "GatePresets.h"
#include <cmath>
#include <limits>

namespace
{
constexpr int voiceCount = 3;
constexpr int gateStepCount = 32;
constexpr int effectCount = 10;
constexpr const char* effectIds[] = {
    "pitchOn", "phaserOn", "autoFilterOn", "chorusOn", "flangerOn",
    "modulationOn", "autoPanOn", "delayOn", "stereoShaperOn", "compressorOn"
};
constexpr const char* effectNames[] = {
    "Pitch Shifter", "Phaser", "Auto Filter", "Chorus", "Flanger",
    "Modulation", "Auto Pan", "Delay", "Stereo Shaper", "Glue Compressor"
};
constexpr double cycleBeats[] = { 8.0, 4.0, 2.0, 1.0, 0.5, 0.25, 0.125,
                                  1.0 / 3.0, 1.0 / 6.0, 0.75, 0.375,
                                  1.0 / 12.0, 0.0625, 2.0 / 3.0, 1.5 };
const juce::StringArray cycleNames {
    "2 bars", "1 bar", "2 beats", "1/4", "1/8", "1/16", "1/32",
    "1/8 triplet", "1/16 triplet", "1/8 dotted", "1/16 dotted",
    "1/12 beat", "1/16 beat", "2/3 beat", "1.5 beats"
};

float onePoleCoefficient (float cutoff, double sampleRate) noexcept
{
    const auto safeCutoff = juce::jlimit (10.0f, static_cast<float> (sampleRate * 0.45), cutoff);
    return 1.0f - std::exp (-juce::MathConstants<float>::twoPi * safeCutoff
                            / static_cast<float> (sampleRate));
}

float timeCoefficient (float milliseconds, double sampleRate) noexcept
{
    const auto seconds = juce::jmax (0.0001f, milliseconds * 0.001f);
    return 1.0f - std::exp (-1.0f / (seconds * static_cast<float> (sampleRate)));
}
}

juce::StringArray MKNoiseRhythmMachineProcessor::getGatePresetNames()
{
    return {
        "2 Beat Gate", "1 Beat Gate", "1/2 Bt Gate", "1/3 Bt Gate", "1/4 Bt Gate",
        "1/6 Bt Gate", "1/8 Bt Gate", "1/12 Bt Gate", "1/16 Bt Gate",
        "Trance Gt 1", "Trance Gt 2", "Trance Gt 3", "Trance Gt 4",
        "Saw Gate 1", "Saw Gate 2", "1st Step", "1st Stp Fade", "2 Step Gate",
        "Off Beat", "1/4 Dynamic", "1/4 Swing", "1/4 Gate Out", "1/4 Gate In",
        "Spd Up Gate", "Slw Dn Gate", "End Fade", "End Gate", "Fade In", "Fade Out",
        "Fade Out In", "Tremelo Slw", "Tremelo Fst", "Sidechain", "Drum Loop",
        "Copter", "AM", "1/4 Classic Gate", "1/4 Trance Gate", "1/4 Offbeat Gate",
        "1/4 Sidechain Pump", "1/4 Deep Pump", "1/4 Soft Pump", "1/4 Hard Pump",
        "1/4 Alternating Gate", "1/4 Pulse Gate", "1/8 Classic Gate", "1/8 Trance Gate",
        "1/8 Offbeat Gate", "1/8 Sidechain Pump", "1/8 Deep Pump", "1/8 Soft Pump",
        "1/8 Hard Pump", "1/8 Swing Gate", "1/8 Alternating Gate", "1/8 Pulse Gate",
        "1/16 Classic Gate", "1/16 Trance Gate", "1/16 Offbeat Gate",
        "1/16 Sidechain Pump", "1/16 Deep Pump", "1/16 Soft Pump", "1/16 Hard Pump",
        "1/16 Swing Gate", "1/16 Alternating Gate", "1/16 Pulse Gate",
        "1/32 Classic Gate", "1/32 Trance Gate", "1/32 Stutter Gate",
        "1/32 Sidechain Pump", "1/32 Rapid Chopper", "1/32 Machine Gun Gate",
        "1/32 Micro Gate", "1/32 Glitch Gate", "1/32 Alternating Gate", "1/32 Pulse Gate",
        "1/8 Triplet Gate", "1/8 Triplet Trance", "1/8 Triplet Pump",
        "1/8 Triplet Chopper", "1/8 Triplet Swing", "1/8 Triplet Alternating",
        "1/8 Triplet Stutter", "1/8 Triplet Pulse", "1/8 Triplet Filter Gate",
        "1/8 Triplet Glitch", "1/16 Triplet Gate", "1/16 Triplet Trance",
        "1/16 Triplet Pump", "1/16 Triplet Chopper", "1/16 Triplet Swing",
        "1/16 Triplet Alternating", "1/16 Triplet Stutter", "1/16 Triplet Pulse",
        "1/16 Triplet Filter Gate", "1/16 Triplet Glitch", "1/8 Dotted Gate",
        "1/8 Dotted Trance", "1/8 Dotted Pump", "1/8 Dotted Chopper",
        "1/8 Dotted Swing", "1/8 Dotted Alternating", "1/8 Dotted Stutter",
        "1/8 Dotted Pulse", "1/8 Dotted Filter Gate", "1/8 Dotted Glitch",
        "1/16 Dotted Gate", "1/16 Dotted Trance", "1/16 Dotted Pump",
        "1/16 Dotted Chopper", "1/16 Dotted Swing", "1/16 Dotted Alternating",
        "1/16 Dotted Stutter", "1/16 Dotted Pulse", "1/16 Dotted Filter Gate",
        "1/16 Dotted Glitch", "Euphoric Trance Gate", "Uplifting Trance Gate",
        "Classic Supersaw Gate", "Festival Pump", "Progressive Gate", "Psytrance Gate",
        "Goa Trance Gate", "Reverse Sidechain", "Ghost Sidechain", "Rhythmic Ducking",
        "Sawtooth Gate", "Square Wave Gate", "Ramp Gate", "Fade In Gate",
        "Fade Out Gate", "Tremolo Gate", "Stereo Alternating Gate", "Ping Pong Gate",
        "Random Rhythm Gate", "Experimental Glitch Gate"
    };
}

juce::AudioProcessorValueTreeState::ParameterLayout
MKNoiseRhythmMachineProcessor::createParameterLayout()
{
    using Range = juce::NormalisableRange<float>;
    using Float = juce::AudioParameterFloat;
    using Bool = juce::AudioParameterBool;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const juce::StringArray noiseTypes {
        "White Noise", "Pink Noise", "Vinyl 1930", "Vinyl 1950", "Vinyl 1960",
        "Mechanical", "Electrical"
    };

    for (int voice = 0; voice < voiceCount; ++voice)
    {
        const auto suffix = juce::String (voice + 1);
        layout.add (std::make_unique<Bool> (juce::ParameterID { "voiceOn" + suffix, 1 },
                                            "Noise " + suffix + " On", voice == 0));
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "voiceType" + suffix, 1 }, "Noise " + suffix + " Type",
            noiseTypes, voice));
        layout.add (std::make_unique<Float> (juce::ParameterID { "voiceGain" + suffix, 1 },
                                             "Noise " + suffix + " Level", Range (-60.0f, 6.0f, 0.1f), -9.0f));
        layout.add (std::make_unique<Float> (juce::ParameterID { "voicePan" + suffix, 1 },
                                             "Noise " + suffix + " Pan", Range (-1.0f, 1.0f, 0.01f), 0.0f));
        layout.add (std::make_unique<Float> (
            juce::ParameterID { "voiceTune" + suffix, 1 }, "Noise " + suffix + " Pitch Shifter",
            Range (-24.0f, 24.0f, 0.1f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("st")));
    }

    layout.add (std::make_unique<Bool> (juce::ParameterID { "gateOn", 1 }, "Trance Gate On", true));
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "gateSteps", 1 }, "Gate Steps", 8, 32, 16));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "gatePreset", 1 }, "Gate Preset", getGatePresetNames(), 0));
    layout.add (std::make_unique<Float> (
        juce::ParameterID { "gateAttack", 1 }, "Gate Attack", Range (0.1f, 100.0f, 0.1f, 0.45f),
        2.0f, juce::AudioParameterFloatAttributes().withLabel ("ms")));
    layout.add (std::make_unique<Float> (
        juce::ParameterID { "gateRelease", 1 }, "Gate Release", Range (0.1f, 200.0f, 0.1f, 0.45f),
        15.0f, juce::AudioParameterFloatAttributes().withLabel ("ms")));
    for (int step = 0; step < gateStepCount; ++step)
        layout.add (std::make_unique<Float> (juce::ParameterID { gateStepParameterID (step), 1 },
                                             "Gate Step " + juce::String (step + 1),
                                             Range (0.0f, 1.0f, 0.001f), step < 8 ? 1.0f : 0.0f));

    for (int effect = 0; effect < effectCount; ++effect)
        layout.add (std::make_unique<Bool> (juce::ParameterID { effectIds[effect], 1 },
                                            effectNames[effect], false));
    layout.add (std::make_unique<Float> (juce::ParameterID { "effectAmount", 1 }, "Effect Amount",
                                         Range (0.0f, 1.0f, 0.001f), 0.35f));
    layout.add (std::make_unique<Float> (
        juce::ParameterID { "effectRate", 1 }, "Modulation Rate", Range (0.03f, 8.0f, 0.01f, 0.4f),
        0.3f, juce::AudioParameterFloatAttributes().withLabel ("Hz")));
    layout.add (std::make_unique<Float> (
        juce::ParameterID { "delayTime", 1 }, "Delay Time", Range (20.0f, 1500.0f, 1.0f, 0.4f),
        250.0f, juce::AudioParameterFloatAttributes().withLabel ("ms")));
    layout.add (std::make_unique<Float> (juce::ParameterID { "stereoWidth", 1 }, "Stereo Width",
                                         Range (0.0f, 2.0f, 0.01f), 1.0f));
    layout.add (std::make_unique<Float> (
        juce::ParameterID { "outputGain", 1 }, "Output", Range (-24.0f, 6.0f, 0.1f), -3.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));
    // Append new parameters to preserve the ordering of existing host automation.
    for (int effect = 0; effect < effectCount; ++effect)
        layout.add (std::make_unique<Float> (
            juce::ParameterID { effectAmountParameterID (effect), 1 },
            juce::String (effectNames[effect]) + " Amount",
            effect == 0 ? Range (-12.0f, 12.0f, 0.1f) : Range (0.0f, 1.0f, 0.001f),
            effect == 0 ? 0.0f : 0.35f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "gateCycle", 1 }, "Gate Cycle", cycleNames, 2));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "gateShape", 1 }, "Gate Curve Shape",
        juce::StringArray { "Hold", "Curve", "S-curve", "Pulse", "Wave", "Triangle",
                            "Stairs", "Smooth Stairs", "Half Sine" }, 0));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "gateRetrigger", 1 }, "Gate MIDI Retrigger", false));
    for (int point = 0; point < gateStepCount; ++point)
        layout.add (std::make_unique<Float> (
            juce::ParameterID { gateOffsetParameterID (point), 1 },
            "Gate Point " + juce::String (point + 1) + " Position",
            Range (-0.49f, 0.49f, 0.001f), 0.0f));
    layout.add (std::make_unique<Float> (
        juce::ParameterID { "gateTension", 1 }, "Gate Curve Tension",
        Range (-1.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<Bool> (
        juce::ParameterID { "gateUsePositions", 1 }, "Gate Free Point Positions", false));
    for (int point = 0; point < gateStepCount; ++point)
        layout.add (std::make_unique<Float> (
            juce::ParameterID { "gateX" + juce::String (point + 1).paddedLeft ('0', 2), 1 },
            "Gate Point " + juce::String (point + 1) + " Time", Range (0.0f, 1.0f, 0.00001f),
            static_cast<float> (point) / 31.0f));
    layout.add (std::make_unique<Bool> (
        juce::ParameterID { "gateFilterOn", 1 }, "Gate Lowpass Filter", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "gateStereoMode", 1 }, "Gate Stereo",
        juce::StringArray { "Linked", "Alternating", "Ping Pong" }, 0));
    layout.add (std::make_unique<Float> (
        juce::ParameterID { "gateSwing", 1 }, "Gate Swing", Range (0.0f, 100.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));
    return layout;
}

MKNoiseRhythmMachineProcessor::MKNoiseRhythmMachineProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "MKNoiseRhythmMachineState", createParameterLayout())
{
    for (int voice = 0; voice < voiceCount; ++voice)
        voices[static_cast<size_t> (voice)].randomState = 0x9e3779b9u + static_cast<uint32_t> (voice * 0x85ebca6bu);
    applyGatePreset (0);
}

void MKNoiseRhythmMachineProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    const auto modulationLength = static_cast<size_t> (std::ceil (sampleRate * 0.12)) + 4;
    const auto delayLength = static_cast<size_t> (std::ceil (sampleRate * 1.6)) + 4;
    for (auto& channel : modulationBuffer)
        channel.assign (modulationLength, 0.0f);
    for (auto& channel : flangerBuffer)
        channel.assign (modulationLength, 0.0f);
    for (auto& channel : delayBuffer)
        channel.assign (delayLength, 0.0f);
    phaserState = {};
    pitchFilterState = {};
    autoFilterState = {};
    compressorEnvelope = {};
    lfoPhase = gatePhase = 0.0;
    modulationWritePosition = delayWritePosition = 0;
    smoothedGate = 0.0f;
    smoothedGateRight = 0.0f;
    gateFilterState = {};
    heldNotes = {};
    heldNoteCount = 0;
    noteEnvelope.reset (sampleRate, 0.005);
    noteEnvelope.setCurrentAndTargetValue (0.0f);
    gateSwingSmoothing.reset (sampleRate, 0.02);
    gateSwingSmoothing.setCurrentAndTargetValue (parameters.getRawParameterValue ("gateSwing")->load());
    for (size_t effect = 0; effect < effectAmountSmoothing.size(); ++effect)
    {
        effectAmountSmoothing[effect].reset (sampleRate, 0.02);
        effectAmountSmoothing[effect].setCurrentAndTargetValue (
            parameters.getRawParameterValue (effectAmountParameterID (static_cast<int> (effect)))->load());
    }
    gatePlaybackPhase.store (0.0f);
    for (size_t voice = 0; voice < pitchShifters.size(); ++voice)
    {
        pitchShifters[voice].prepare (sampleRate);
        pitchSmoothing[voice].reset (sampleRate, 0.02);
        pitchSmoothing[voice].setCurrentAndTargetValue (
            parameters.getRawParameterValue ("voiceTune" + juce::String (voice + 1))->load());
    }
}

void MKNoiseRhythmMachineProcessor::releaseResources() {}

bool MKNoiseRhythmMachineProcessor::isBusesLayoutSupported (const BusesLayout& layout) const
{
    const auto outputSet = layout.getMainOutputChannelSet();
    return outputSet == juce::AudioChannelSet::mono() || outputSet == juce::AudioChannelSet::stereo();
}

juce::String MKNoiseRhythmMachineProcessor::gateStepParameterID (int step)
{
    return "gateStep" + juce::String (step + 1).paddedLeft ('0', 2);
}

void MKNoiseRhythmMachineProcessor::setGateStep (int step, float value)
{
    if (step < 0 || step >= gateStepCount)
        return;
    if (auto* parameter = parameters.getParameter (gateStepParameterID (step)))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (juce::jlimit (0.0f, 1.0f, value)));
}

float MKNoiseRhythmMachineProcessor::getGateStep (int step) const
{
    if (step < 0 || step >= gateStepCount)
        return 0.0f;
    if (const auto* value = parameters.getRawParameterValue (gateStepParameterID (step)))
        return value->load();
    return 0.0f;
}

int MKNoiseRhythmMachineProcessor::getGateStepCount() const
{
    return juce::roundToInt (parameters.getRawParameterValue ("gateSteps")->load());
}

void MKNoiseRhythmMachineProcessor::setGatePointCount (int count)
{
    count = juce::jlimit (8, 32, count);
    if (parameters.getRawParameterValue ("gateUsePositions")->load() > 0.5f && count != getGateStepCount())
    {
        std::vector<GatePreset::Point> points;
        const auto oldCount = getGateStepCount();
        const auto shape = juce::roundToInt (parameters.getRawParameterValue ("gateShape")->load());
        for (int i = 0; i < oldCount; ++i)
            points.push_back ({ (i + getGatePointOffset (i)) / (oldCount - 1), getGateStep (i) });
        while (static_cast<int> (points.size()) < count)
        {
            size_t longest = 0;
            for (size_t i = 1; i + 1 < points.size(); ++i)
                if (points[i + 1].x - points[i].x > points[longest + 1].x - points[longest].x)
                    longest = i;
            const auto first = points[longest], second = points[longest + 1];
            std::array<float, 32> levels {}, offsets {};
            for (size_t i = 0; i < points.size(); ++i)
            {
                levels[i] = points[i].y;
                offsets[i] = points[i].x * static_cast<float> (points.size() - 1) - static_cast<float> (i);
            }
            const auto x = (first.x + second.x) * 0.5f;
            Pattern evaluator (4);
            const auto y = evaluateGateCurve (evaluator, levels, offsets, static_cast<int> (points.size()),
                shape, x, parameters.getRawParameterValue ("gateTension")->load());
            points.insert (points.begin() + static_cast<std::ptrdiff_t> (longest + 1), { x, y });
        }
        while (static_cast<int> (points.size()) > count)
        {
            size_t leastImportant = 1;
            auto smallestError = std::numeric_limits<float>::max();
            for (size_t i = 1; i + 1 < points.size(); ++i)
            {
                const auto first = points[i - 1], current = points[i], next = points[i + 1];
                const auto predicted = shape == 0 ? first.y
                    : first.y + (next.y - first.y) * (current.x - first.x) / (next.x - first.x);
                const auto error = std::abs (predicted - current.y) * (next.x - first.x);
                if (error < smallestError) { smallestError = error; leastImportant = i; }
            }
            points.erase (points.begin() + static_cast<std::ptrdiff_t> (leastImportant));
        }
        for (int i = 0; i < count; ++i)
        {
            setGateStep (i, points[static_cast<size_t> (i)].y);
            auto* parameter = parameters.getParameter (gatePositionParameterID (i));
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (points[static_cast<size_t> (i)].x));
        }
    }
    auto* parameter = parameters.getParameter ("gateSteps");
    parameter->beginChangeGesture();
    parameter->setValueNotifyingHost (parameter->convertTo0to1 (static_cast<float> (count)));
    parameter->endChangeGesture();
}

juce::String MKNoiseRhythmMachineProcessor::effectAmountParameterID (int effect)
{
    return juce::String (effectIds[effect]).dropLastCharacters (2) + "Amount";
}

juce::String MKNoiseRhythmMachineProcessor::gateOffsetParameterID (int point)
{
    return "gateOffset" + juce::String (point + 1).paddedLeft ('0', 2);
}

juce::String MKNoiseRhythmMachineProcessor::gatePositionParameterID (int point) const
{
    return parameters.getRawParameterValue ("gateUsePositions")->load() > 0.5f
        ? "gateX" + juce::String (point + 1).paddedLeft ('0', 2) : gateOffsetParameterID (point);
}

float MKNoiseRhythmMachineProcessor::getGatePointOffset (int point) const
{
    if (parameters.getRawParameterValue ("gateUsePositions")->load() > 0.5f)
    {
        const auto count = getGateStepCount();
        if (point == 0 || point >= count - 1) return 0.0f;
        auto previous = 0.0f;
        // Host automation may cross points; keep the evaluator's binary search ordered.
        for (int i = 1; i <= point; ++i)
        {
            const auto requested = parameters.getRawParameterValue (
                "gateX" + juce::String (i + 1).paddedLeft ('0', 2))->load();
            previous = juce::jlimit (previous + 0.000001f,
                                    1.0f - static_cast<float> (count - 1 - i) * 0.000001f, requested);
        }
        return previous * static_cast<float> (count - 1) - static_cast<float> (point);
    }
    return parameters.getRawParameterValue (gateOffsetParameterID (point))->load();
}

juce::String MKNoiseRhythmMachineProcessor::getGatePresetDescription() const
{
    const auto names = getGatePresetNames();
    return makeGatePreset (names[juce::jlimit (0, names.size() - 1, juce::roundToInt (
        parameters.getRawParameterValue ("gatePreset")->load()))]).description;
}

float MKNoiseRhythmMachineProcessor::evaluateGateCurve (Pattern& evaluator, const std::array<float, 32>& levels,
                                                       const std::array<float, 32>& offsets,
                                                       int count, int shape, float phase, float tension)
{
    const auto x = [&offsets, count] (int point)
    {
        if (point == 0) return 0.0;
        if (point == count - 1) return 1.0;
        return (static_cast<double> (point) + offsets[static_cast<size_t> (point)])
             / static_cast<double> (count - 1);
    };
    phase = juce::jlimit (0.0f, 1.0f, phase);
    int low = 0, high = count;
    while (low <= high)
    {
        const auto segmentIndex = (low + high) / 2;
        const auto first = segmentIndex == 0 ? count - 1 : segmentIndex - 1;
        const auto second = segmentIndex == count ? 0 : segmentIndex;
        const auto x1 = segmentIndex == 0 ? x (first) - 1.0 : x (first);
        const auto x2 = segmentIndex == count ? x (second) + 1.0 : x (second);
        if (phase < x1)
            high = segmentIndex - 1;
        else if (phase > x2)
            low = segmentIndex + 1;
        else
        {
            const auto start = static_cast<size_t> (first);
            if (shape == 0)
                return levels[start];
            // Call Gate12's original segment functions without its per-sample mutex.
            if (x1 == x2)
                return levels[static_cast<size_t> (second)];
            const Segment segment { x1, x2, levels[start],
                                     levels[static_cast<size_t> (second)], tension, 0.0, shape };
            double value = levels[start];
            switch (shape)
            {
                case PointType::Curve: value = evaluator.get_y_curve (segment, phase); break;
                case PointType::SCurve: value = evaluator.get_y_scurve (segment, phase); break;
                case PointType::Pulse: value = evaluator.get_y_pulse (segment, phase); break;
                case PointType::Wave: value = evaluator.get_y_wave (segment, phase); break;
                case PointType::Triangle: value = evaluator.get_y_triangle (segment, phase); break;
                case PointType::Stairs: value = evaluator.get_y_stairs (segment, phase); break;
                case PointType::SmoothSt: value = evaluator.get_y_smooth_stairs (segment, phase); break;
                case PointType::HalfSine: value = evaluator.get_y_half_sine (segment, phase); break;
            }
            return static_cast<float> (juce::jlimit (0.0, 1.0, value));
        }
    }
    return levels[static_cast<size_t> (count - 1)];
}

double MKNoiseRhythmMachineProcessor::swingGatePosition (double beats, float amount) noexcept
{
    if (amount == 0.0f)
        return beats;
    const auto pair = beats * 2.0;
    const auto start = std::floor (pair);
    const auto position = pair - start;
    const auto midpoint = 0.5 + static_cast<double> (amount) / 600.0;
    const auto straightPosition = position < midpoint
        ? position * 0.5 / midpoint
        : 0.5 + (position - midpoint) * 0.5 / (1.0 - midpoint);
    return (start + straightPosition) * 0.5;
}

void MKNoiseRhythmMachineProcessor::applyGatePreset (int presetIndex)
{
    const auto names = getGatePresetNames();
    const auto boundedIndex = juce::jlimit (0, names.size() - 1, presetIndex);
    const auto preset = makeGatePreset (names[boundedIndex]);
    auto set = [this] (const juce::String& id, float value)
    {
        auto* parameter = parameters.getParameter (id);
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
    };
    set ("gatePreset", static_cast<float> (boundedIndex));
    set ("gateSteps", static_cast<float> (preset.points.size()));
    set ("gateCycle", static_cast<float> (preset.cycle));
    set ("gateShape", static_cast<float> (preset.shape));
    set ("gateAttack", preset.attackMs);
    set ("gateRelease", preset.releaseMs);
    set ("gateTension", 0.0f);
    set ("gateFilterOn", preset.filter ? 1.0f : 0.0f);
    set ("gateStereoMode", static_cast<float> (preset.stereo));
    set ("gateUsePositions", 1.0f);
    set ("gateOn", 1.0f);
    for (int step = 0; step < gateStepCount; ++step)
    {
        const auto& point = preset.points[juce::jmin (static_cast<size_t> (step), preset.points.size() - 1)];
        setGateStep (step, point.y);
        set (gatePositionParameterID (step), point.x);
        set (gateOffsetParameterID (step), 0.0f);
    }
}

void MKNoiseRhythmMachineProcessor::randomizeGate()
{
    uint32_t random = 0xa341316cu ^ static_cast<uint32_t> (juce::Time::getMillisecondCounter());
    for (int step = 0; step < gateStepCount; ++step)
    {
        random ^= random << 13;
        random ^= random >> 17;
        random ^= random << 5;
        const auto unit = static_cast<float> (random & 0xffffu) / 65535.0f;
        const auto value = unit < 0.42f ? 0.0f : (unit < 0.74f ? 0.35f + unit * 0.4f : 1.0f);
        setGateStep (step, value);
    }
}

float MKNoiseRhythmMachineProcessor::nextNoise (VoiceState& state) noexcept
{
    auto x = state.randomState;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state.randomState = x;
    return static_cast<float> (x >> 8) * (2.0f / 16777215.0f) - 1.0f;
}

float MKNoiseRhythmMachineProcessor::generateVoiceSample (VoiceState& state, int type, float tune) noexcept
{
    const auto white = nextNoise (state);
    state.pinkA = 0.99765f * state.pinkA + white * 0.0990460f;
    state.pinkB = 0.96300f * state.pinkB + white * 0.2965164f;
    state.pinkC = 0.57000f * state.pinkC + white * 1.0526913f;
    const auto pink = (state.pinkA + state.pinkB + state.pinkC + white * 0.1848f) * 0.25f;
    const auto tuned = std::pow (2.0f, tune / 12.0f);
    const auto lowCutoff = (type == 2 ? 1800.0f : (type == 3 ? 7000.0f : 12500.0f)) * tuned;
    const auto lowCoefficient = onePoleCoefficient (lowCutoff, currentSampleRate);
    state.lowState += lowCoefficient * (white - state.lowState);
    const auto highPassed = white - state.lowState;
    auto sample = white;

    switch (type)
    {
        case 1:
            sample = pink * 1.6f;
            break;
        case 2:
            sample = (state.lowState * 0.85f + highPassed * 0.22f) * 1.65f;
            break;
        case 3:
            sample = (state.lowState * 0.95f + highPassed * 0.14f) * 1.4f;
            break;
        case 4:
            sample = (state.lowState * 0.9f + highPassed * 0.2f) * 1.5f;
            break;
        case 5:
        case 6:
        {
            const auto baseFrequency = type == 5 ? 92.0f : 60.0f;
            const auto frequency = juce::jlimit (20.0f, static_cast<float> (currentSampleRate * 0.2),
                                                 baseFrequency * tuned);
            state.phase += juce::MathConstants<double>::twoPi * frequency / currentSampleRate;
            if (state.phase >= juce::MathConstants<double>::twoPi)
                state.phase -= juce::MathConstants<double>::twoPi;
            const auto hum = std::sin (static_cast<float> (state.phase))
                           + 0.35f * std::sin (static_cast<float> (state.phase * 2.0));
            sample = type == 5 ? (pink * 0.35f + hum * 0.45f) : (white * 0.20f + hum * 0.65f);
            break;
        }
        default:
            break;
    }

    if (type >= 2 && type <= 4)
    {
        if (--state.crackleCountdown <= 0)
        {
            const auto chance = type == 2 ? 0.012f : (type == 3 ? 0.005f : 0.002f);
            state.crackleCountdown = 1 + static_cast<int> ((1.0f - nextNoise (state) * 0.5f) * 900.0f);
            state.crackle = nextNoise (state) > (1.0f - chance * 2.0f) ? nextNoise (state) * 0.75f : 0.0f;
        }
        sample += state.crackle;
    }

    return juce::jlimit (-1.0f, 1.0f, sample * 0.45f);
}

float MKNoiseRhythmMachineProcessor::readDelay (const std::vector<float>& buffer,
                                                 float delaySamples,
                                                 int writePosition) const noexcept
{
    auto position = static_cast<float> (writePosition) - delaySamples;
    while (position < 0.0f)
        position += static_cast<float> (buffer.size());
    const auto first = static_cast<size_t> (position) % buffer.size();
    const auto second = (first + 1) % buffer.size();
    const auto fraction = position - std::floor (position);
    return buffer[first] + fraction * (buffer[second] - buffer[first]);
}

void MKNoiseRhythmMachineProcessor::updateControls (Controls& c)
{
    const auto read = [this] (const juce::String& id)
    {
        return parameters.getRawParameterValue (id)->load();
    };
    for (int voice = 0; voice < voiceCount; ++voice)
    {
        const auto suffix = juce::String (voice + 1);
        c.voiceOn[static_cast<size_t> (voice)] = read ("voiceOn" + suffix) > 0.5f;
        c.voiceType[static_cast<size_t> (voice)] = juce::roundToInt (read ("voiceType" + suffix));
        c.voiceGain[static_cast<size_t> (voice)] = juce::Decibels::decibelsToGain (read ("voiceGain" + suffix));
        c.voicePan[static_cast<size_t> (voice)] = read ("voicePan" + suffix);
        c.voiceTune[static_cast<size_t> (voice)] = read ("voiceTune" + suffix);
        pitchSmoothing[static_cast<size_t> (voice)].setTargetValue (c.voiceTune[static_cast<size_t> (voice)]);
    }
    c.gateOn = read ("gateOn") > 0.5f;
    c.gateSteps = juce::jlimit (8, gateStepCount, juce::roundToInt (read ("gateSteps")));
    c.gateAttack = read ("gateAttack");
    c.gateRelease = read ("gateRelease");
    c.gateShape = juce::roundToInt (read ("gateShape"));
    c.gateTension = read ("gateTension");
    c.gateCycleBeats = cycleBeats[juce::jlimit (0, 14, juce::roundToInt (read ("gateCycle")))];
    c.gateRetrigger = read ("gateRetrigger") > 0.5f;
    gateSwingSmoothing.setTargetValue (read ("gateSwing"));
    c.gateFilterOn = read ("gateFilterOn") > 0.5f;
    c.gateStereoMode = juce::roundToInt (read ("gateStereoMode"));
    const auto freePositions = read ("gateUsePositions") > 0.5f;
    auto previousTime = 0.0f;
    for (int step = 0; step < gateStepCount; ++step)
    {
        c.gate[static_cast<size_t> (step)] = read (gateStepParameterID (step));
        if (! freePositions)
            c.gateOffsets[static_cast<size_t> (step)] = read (gateOffsetParameterID (step));
        else if (step > 0 && step < c.gateSteps - 1)
        {
            previousTime = juce::jlimit (previousTime + 0.000001f,
                1.0f - static_cast<float> (c.gateSteps - 1 - step) * 0.000001f,
                read ("gateX" + juce::String (step + 1).paddedLeft ('0', 2)));
            c.gateOffsets[static_cast<size_t> (step)] = previousTime * (c.gateSteps - 1) - step;
        }
        else
            c.gateOffsets[static_cast<size_t> (step)] = 0.0f;
    }
    for (int effect = 0; effect < effectCount; ++effect)
    {
        c.effectOn[static_cast<size_t> (effect)] = read (effectIds[effect]) > 0.5f;
        effectAmountSmoothing[static_cast<size_t> (effect)].setTargetValue (
            read (effectAmountParameterID (effect)));
    }
    c.effectRate = read ("effectRate");
    c.delayTime = read ("delayTime");
    c.stereoWidth = read ("stereoWidth");
    c.outputGain = juce::Decibels::decibelsToGain (read ("outputGain"));
}

void MKNoiseRhythmMachineProcessor::handleMidiMessage (const juce::MidiMessage& message)
{
    if (! (message.isNoteOnOrOff() || message.isAllNotesOff() || message.isAllSoundOff()))
        return;

    auto& channelNotes = heldNotes[static_cast<size_t> (message.getChannel() - 1)];
    if (message.isAllNotesOff() || message.isAllSoundOff())
    {
        for (auto& held : channelNotes)
        {
            if (held)
                --heldNoteCount;
            held = false;
        }
    }
    else
    {
        auto& held = channelNotes[static_cast<size_t> (message.getNoteNumber())];
        const auto noteOn = message.isNoteOn();
        if (held != noteOn)
        {
            heldNoteCount += noteOn ? 1 : -1;
            held = noteOn;
        }
    }
    noteEnvelope.setTargetValue (heldNoteCount > 0 ? 1.0f : 0.0f);
}

void MKNoiseRhythmMachineProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                   juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const auto channelCount = buffer.getNumChannels();
    if (channelCount == 0 || modulationBuffer[0].empty() || delayBuffer[0].empty())
    {
        buffer.clear();
        return;
    }

    Controls controls;
    updateControls (controls);

    double bpm = 120.0;
    double ppq = 0.0;
    bool hasPpq = false;
    if (auto* hostPlayHead = getPlayHead())
        if (const auto position = hostPlayHead->getPosition())
        {
            if (const auto hostBpm = position->getBpm())
                if (std::isfinite (*hostBpm) && *hostBpm > 0.0)
                    bpm = *hostBpm;
            if (const auto hostPpq = position->getPpqPosition())
            {
                ppq = *hostPpq;
                hasPpq = std::isfinite (ppq);
            }
        }

    auto* left = buffer.getWritePointer (0);
    auto* right = channelCount > 1 ? buffer.getWritePointer (1) : nullptr;
    const auto modulationLength = static_cast<int> (modulationBuffer[0].size());
    const auto delayLength = static_cast<int> (delayBuffer[0].size());
    const auto gateAttackCoefficient = timeCoefficient (controls.gateAttack, currentSampleRate);
    const auto gateReleaseCoefficient = timeCoefficient (controls.gateRelease, currentSampleRate);
    const auto lfoIncrement = juce::MathConstants<double>::twoPi * controls.effectRate / currentSampleRate;
    auto midiEvent = midi.begin();
    const auto midiEnd = midi.end();

    for (int sampleIndex = 0; sampleIndex < buffer.getNumSamples(); ++sampleIndex)
    {
        while (midiEvent != midiEnd && (*midiEvent).samplePosition <= sampleIndex)
        {
            const auto message = (*midiEvent).getMessage();
            if (controls.gateRetrigger && message.isNoteOn())
                gatePhase = 0.0;
            handleMidiMessage (message);
            ++midiEvent;
        }
        const auto noteGain = noteEnvelope.getNextValue();
        for (size_t effect = 0; effect < controls.effectAmounts.size(); ++effect)
            controls.effectAmounts[effect] = effectAmountSmoothing[effect].getNextValue();
        const auto& amounts = controls.effectAmounts;
        float l = 0.0f, r = 0.0f;
        for (int voice = 0; voice < voiceCount; ++voice)
        {
            const auto index = static_cast<size_t> (voice);
            const auto raw = controls.voiceOn[index] && noteGain > 0.0f
                ? generateVoiceSample (voices[index], controls.voiceType[index], 0.0f) : 0.0f;
            const auto mono = pitchShifters[index].process (raw,
                pitchSmoothing[index].getNextValue() + (controls.effectOn[0] ? amounts[0] : 0.0f));
            if (! controls.voiceOn[index])
                continue;
            const auto pan = controls.voicePan[index];
            const auto leftGain = std::cos ((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
            const auto rightGain = std::sin ((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
            l += mono * controls.voiceGain[index] * leftGain;
            r += mono * controls.voiceGain[index] * rightGain;
        }

        const auto lfo = static_cast<float> (std::sin (lfoPhase));

        if (controls.effectOn[1])
        {
            for (int channel = 0; channel < 2; ++channel)
            {
                auto& value = channel == 0 ? l : r;
                const auto dry = value;
                for (int stage = 0; stage < 4; ++stage)
                {
                    const auto coefficient = 0.25f + 0.5f
                        * (0.5f + 0.5f * lfo * (stage % 2 == 0 ? 1.0f : -1.0f));
                    const auto input = value;
                    value = -coefficient * input + phaserState[static_cast<size_t> (channel)][static_cast<size_t> (stage)];
                    phaserState[static_cast<size_t> (channel)][static_cast<size_t> (stage)] =
                        input + coefficient * value;
                }
                value = dry + amounts[1] * (value - dry);
            }
        }

        if (controls.effectOn[2])
        {
            const auto cutoff = 250.0f + 9000.0f * (0.5f + 0.5f * lfo);
            const auto coefficient = onePoleCoefficient (cutoff, currentSampleRate);
            autoFilterState[0] += coefficient * (l - autoFilterState[0]);
            autoFilterState[1] += coefficient * (r - autoFilterState[1]);
            l += amounts[2] * (autoFilterState[0] - l);
            r += amounts[2] * (autoFilterState[1] - r);
        }

        modulationBuffer[0][static_cast<size_t> (modulationWritePosition)] = l;
        modulationBuffer[1][static_cast<size_t> (modulationWritePosition)] = r;
        const auto chorusMs = 18.0f + 8.0f * lfo;
        const auto modDelaySamples = juce::jlimit (1.0f, static_cast<float> (modulationLength - 2),
                                                    chorusMs * static_cast<float> (currentSampleRate * 0.001));
        if (controls.effectOn[3])
        {
            const auto delayedL = readDelay (modulationBuffer[0], modDelaySamples, modulationWritePosition);
            const auto delayedR = readDelay (modulationBuffer[1], modDelaySamples, modulationWritePosition);
            const auto wet = amounts[3] * 0.55f;
            l += wet * delayedL;
            r += wet * delayedR;
        }
        const auto flangerSamples = static_cast<float> (currentSampleRate * 0.001) * (3.0f + 2.0f * lfo);
        for (int channel = 0; channel < 2; ++channel)
        {
            auto& value = channel == 0 ? l : r;
            auto& history = flangerBuffer[static_cast<size_t> (channel)];
            const auto delayed = readDelay (history, flangerSamples, modulationWritePosition);
            history[static_cast<size_t> (modulationWritePosition)] = value + delayed * 0.3f;
            if (controls.effectOn[4])
                value += amounts[4] * 0.35f * delayed;
        }
        if (++modulationWritePosition >= modulationLength)
            modulationWritePosition = 0;

        if (controls.effectOn[5])
        {
            const auto ring = 1.0f - amounts[5] + amounts[5] * lfo;
            l *= ring;
            r *= ring;
        }
        if (controls.effectOn[6])
        {
            const auto pan = lfo * amounts[6];
            l *= std::sqrt (1.0f - pan);
            r *= std::sqrt (1.0f + pan);
        }

        const auto delaySamples = juce::jlimit (1.0f, static_cast<float> (delayLength - 2),
            controls.delayTime * static_cast<float> (currentSampleRate * 0.001));
        const auto delayedL = readDelay (delayBuffer[0], delaySamples, delayWritePosition);
        const auto delayedR = readDelay (delayBuffer[1], delaySamples, delayWritePosition);
        delayBuffer[0][static_cast<size_t> (delayWritePosition)] = l + delayedL * 0.32f;
        delayBuffer[1][static_cast<size_t> (delayWritePosition)] = r + delayedR * 0.32f;
        if (++delayWritePosition >= delayLength)
            delayWritePosition = 0;
        if (controls.effectOn[7])
        {
            l += delayedL * amounts[7];
            r += delayedR * amounts[7];
        }

        if (controls.effectOn[8])
        {
            const auto mid = (l + r) * 0.5f;
            const auto decorrelated = readDelay (modulationBuffer[1],
                static_cast<float> (currentSampleRate * 0.007), modulationWritePosition);
            const auto side = (l - r) * 0.5f * (1.0f + amounts[8])
                            + (r - decorrelated) * 0.25f * amounts[8];
            l = mid + side;
            r = mid - side;
        }

        if (controls.effectOn[9])
        {
            const auto peak = juce::jmax (std::abs (l), std::abs (r));
            const auto coefficient = timeCoefficient (peak > compressorEnvelope[0] ? 10.0f : 100.0f,
                                                       currentSampleRate);
            compressorEnvelope[0] += coefficient * (peak - compressorEnvelope[0]);
            const auto threshold = 0.2f - amounts[9] * 0.17f;
            const auto ratio = 1.0f + amounts[9] * 2.0f;
            const auto gain = compressorEnvelope[0] > threshold
                ? std::pow (threshold / compressorEnvelope[0], 1.0f - 1.0f / ratio) : 1.0f;
            l *= gain;
            r *= gain;
        }

        const auto currentPpq = hasPpq && ! controls.gateRetrigger
            ? ppq + static_cast<double> (sampleIndex) * bpm / (60.0 * currentSampleRate)
            : gatePhase;
        const auto swungPpq = swingGatePosition (currentPpq, gateSwingSmoothing.getNextValue());
        auto phase = std::fmod (swungPpq / controls.gateCycleBeats, 1.0);
        if (phase < 0.0) phase += 1.0;
        gatePlaybackPhase.store (static_cast<float> (phase), std::memory_order_relaxed);
        float gateTarget = 1.0f;
        float gateTargetRight = 1.0f;
        if (controls.gateOn)
        {
            gateTarget = evaluateGateCurve (gatePattern, controls.gate, controls.gateOffsets,
                controls.gateSteps, controls.gateShape, static_cast<float> (phase), controls.gateTension);
            const auto rightPhase = controls.gateStereoMode != 0 ? std::fmod (phase + 0.5, 1.0) : phase;
            gateTargetRight = evaluateGateCurve (gatePattern, controls.gate, controls.gateOffsets,
                controls.gateSteps, controls.gateShape, static_cast<float> (rightPhase), controls.gateTension);
        }
        const auto gateCoefficient = gateTarget > smoothedGate ? gateAttackCoefficient : gateReleaseCoefficient;
        smoothedGate += gateCoefficient * (gateTarget - smoothedGate);
        if (gateTarget == 0.0f && smoothedGate < 0.00001f)
            smoothedGate = 0.0f;
        smoothedGateRight += (gateTargetRight > smoothedGateRight ? gateAttackCoefficient : gateReleaseCoefficient)
                           * (gateTargetRight - smoothedGateRight);
        if (gateTargetRight == 0.0f && smoothedGateRight < 0.00001f)
            smoothedGateRight = 0.0f;
        if (controls.gateOn && controls.gateFilterOn)
            for (int channel = 0; channel < 2; ++channel)
            {
                auto& value = channel == 0 ? l : r;
                const auto envelope = channel == 0 ? smoothedGate : smoothedGateRight;
                const auto coefficient = onePoleCoefficient (30.0f + 18000.0f * envelope * envelope, currentSampleRate);
                auto& states = gateFilterState[static_cast<size_t> (channel)];
                states[0] += coefficient * (value - states[0]);
                states[1] += coefficient * (states[0] - states[1]);
                value = states[1];
            }
        l *= smoothedGate * controls.outputGain * noteGain;
        r *= smoothedGateRight * controls.outputGain * noteGain;

        left[sampleIndex] = right != nullptr ? l : (l + r) * 0.5f;
        if (right != nullptr)
            right[sampleIndex] = r;

        gatePhase += bpm / (60.0 * currentSampleRate);
        if (gatePhase >= controls.gateCycleBeats)
            gatePhase = std::fmod (gatePhase, controls.gateCycleBeats);
        lfoPhase += lfoIncrement;
        if (lfoPhase >= juce::MathConstants<double>::twoPi)
            lfoPhase -= juce::MathConstants<double>::twoPi;
    }
    midi.clear();
}

juce::AudioProcessorEditor* MKNoiseRhythmMachineProcessor::createEditor()
{
    return new MKNoiseRhythmMachineEditor (*this);
}

void MKNoiseRhythmMachineProcessor::getStateInformation (juce::MemoryBlock& destination)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destination);
}

void MKNoiseRhythmMachineProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (parameters.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            for (const auto* id : { "gateUsePositions", "gateSwing" })
                if (! state.getChildWithProperty ("id", id).isValid())
                {
                    juce::ValueTree legacyDefault ("PARAM");
                    legacyDefault.setProperty ("id", id, nullptr);
                    legacyDefault.setProperty ("value", 0.0f, nullptr);
                    state.appendChild (legacyDefault, nullptr);
                }
            parameters.replaceState (state);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MKNoiseRhythmMachineProcessor();
}
