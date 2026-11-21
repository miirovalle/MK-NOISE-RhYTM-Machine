#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include "../Source/GatePresets.h"
#include <cmath>
#include <iostream>

namespace
{
double energyIn (const juce::AudioBuffer<float>& buffer, int start, int end)
{
    double energy = 0.0;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        for (int sample = start; sample < end; ++sample)
        {
            const auto value = buffer.getSample (channel, sample);
            if (! std::isfinite (value))
                return -1.0;
            energy += static_cast<double> (value) * value;
        }
    return energy;
}

bool check (bool passed, const char* description)
{
    if (! passed)
        std::cerr << description << '\n';
    return passed;
}

void setParameter (MKNoiseRhythmMachineProcessor& processor, const char* id, float value)
{
    auto* parameter = processor.parameters.getParameter (id);
    parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
}

class TestPlayHead final : public juce::AudioPlayHead
{
public:
    double ppq = 0.0, bpm = 120.0;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo position;
        position.setPpqPosition (ppq);
        position.setBpm (bpm);
        position.setIsPlaying (true);
        return position;
    }
};

bool testGateTiming()
{
    MKNoiseRhythmMachineProcessor processor;
    TestPlayHead host;
    processor.setPlayHead (&host);
    setParameter (processor, "gateAttack", 0.1f);
    setParameter (processor, "gateRelease", 0.1f);
    processor.applyGatePreset (36); // 1/4 Classic Gate
    processor.prepareToPlay (8000.0, 4000);
    juce::AudioBuffer<float> buffer (2, 4000);
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
    processor.processBlock (buffer, midi);
    if (! check (energyIn (buffer, 500, 1800) > 0.001
                 && energyIn (buffer, 2500, 3800) == 0.0,
                 "Quarter-note gate must open and close within one beat at 120 BPM"))
        return false;
    host.ppq = 0.7;
    processor.processBlock (buffer, midi);
    if (! check (energyIn (buffer, 100, 900) == 0.0, "Host seek must position the gate in its closed section"))
        return false;

    processor.applyGatePreset (45); // 1/8 Classic Gate
    host.ppq = 0.0;
    processor.processBlock (buffer, midi);
    if (! check (energyIn (buffer, 200, 900) > 0.001
                 && energyIn (buffer, 1400, 1800) == 0.0
                 && energyIn (buffer, 2300, 2900) > 0.001,
                 "Eighth-note preset must run twice as fast as quarter-note preset"))
        return false;

    processor.applyGatePreset (36);
    setParameter (processor, "gateRetrigger", 1.0f);
    host.ppq = 0.7;
    midi.addEvent (juce::MidiMessage::noteOn (1, 62, 1.0f), 1000);
    processor.processBlock (buffer, midi);
    if (! check (energyIn (buffer, 1200, 2000) > 0.001,
                 "MIDI retrigger must restart the envelope at the event offset"))
        return false;

    setParameter (processor, "gateRetrigger", 0.0f);
    processor.applyGatePreset (36);
    host.ppq = 0.0;
    host.bpm = 240.0;
    processor.processBlock (buffer, midi);
    if (! check (energyIn (buffer, 200, 900) > 0.001
                 && energyIn (buffer, 1400, 1800) == 0.0,
                 "Host tempo doubling must double gate speed"))
        return false;

    for (int point = 0; point < 32; ++point)
        processor.setGateStep (point, 0.0f);
    processor.processBlock (buffer, midi);
    return check (energyIn (buffer, 100, 4000) == 0.0, "Zero curve must completely close the gate");
}

bool testGate12Curves()
{
    Pattern upstream (2);
    Pattern evaluator (3);
    std::array<float, 32> levels {}, offsets {};
    for (int point = 0; point < 8; ++point)
    {
        levels[static_cast<size_t> (point)] = point % 2 == 0 ? 0.2f : 0.9f;
        offsets[static_cast<size_t> (point)] = point % 2 == 0 ? -0.25f : 0.25f;
    }
    for (int shape = 0; shape <= 8; ++shape)
    {
        upstream.points.clear();
        for (int point = 0; point < 8; ++point)
        {
            const auto offset = point == 0 || point == 7 ? 0.0f : offsets[static_cast<size_t> (point)];
            upstream.insertPoint ((point + offset) / 7.0, levels[static_cast<size_t> (point)], 0.37, shape);
        }
        upstream.buildSegments();
        for (int sample = 1; sample < 400; ++sample)
        {
            const auto phase = static_cast<float> (sample) / 400.0f;
            const auto actual = MKNoiseRhythmMachineProcessor::evaluateGateCurve (
                evaluator, levels, offsets, 8, shape, phase, 0.37f);
            const auto expected = juce::jlimit (0.0, 1.0, upstream.get_y_at (phase));
            if (! check (std::isfinite (actual) && std::abs (actual - expected) < 0.0001,
                         "Integrated curve must match Gate12 for all nine segment types"))
            {
                std::cerr << "Shape " << shape << ", phase " << phase
                          << ", expected " << expected << ", actual " << actual << '\n';
                return false;
            }
        }
    }
    return true;
}

bool testEffectKnobs()
{
    constexpr const char* switches[] = {
        "pitchOn", "phaserOn", "autoFilterOn", "chorusOn", "flangerOn",
        "modulationOn", "autoPanOn", "delayOn", "stereoShaperOn", "compressorOn"
    };
    for (int effect = 0; effect < 10; ++effect)
    {
        MKNoiseRhythmMachineProcessor dry, zero, wet;
        const auto amountId = MKNoiseRhythmMachineProcessor::effectAmountParameterID (effect);
        for (auto* processor : { &dry, &zero, &wet })
        {
            setParameter (*processor, "gateOn", 0.0f);
            setParameter (*processor, "voiceType1", 5.0f);
        }
        setParameter (zero, switches[effect], 1.0f);
        setParameter (wet, switches[effect], 1.0f);
        setParameter (zero, amountId.toRawUTF8(), 0.0f);
        setParameter (wet, amountId.toRawUTF8(), effect == 0 ? 12.0f : 1.0f);
        juce::AudioBuffer<float> baseline (2, 16000), neutral (2, 16000), adjusted (2, 16000);
        for (auto* processor : { &dry, &zero, &wet })
            processor->prepareToPlay (8000.0, 16000);
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
        dry.processBlock (baseline, midi);
        midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
        zero.processBlock (neutral, midi);
        midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
        wet.processBlock (adjusted, midi);
        double difference = 0.0;
        for (int channel = 0; channel < 2; ++channel)
            for (int sample = 0; sample < 16000; ++sample)
            {
                if (! check (std::abs (baseline.getSample (channel, sample) - neutral.getSample (channel, sample)) < 0.00001f,
                             "Each zero/neutral knob must preserve dry sound"))
                    return false;
                const auto delta = adjusted.getSample (channel, sample) - baseline.getSample (channel, sample);
                if (! check (std::isfinite (delta), "Effect output must be finite"))
                    return false;
                difference += delta * delta;
            }
        if (! check (difference > 0.001, "Each effect's own knob must audibly change the output"))
            return false;
    }
    return true;
}

bool testCurveState()
{
    MKNoiseRhythmMachineProcessor source, restored;
    source.applyGatePreset (45);
    source.setGateStep (4, 0.63f);
    setParameter (source, "gateOffset05", 0.21f);
    setParameter (source, "gateShape", 8.0f);
    setParameter (source, "gateTension", -0.3f);
    setParameter (source, "chorusAmount", 0.72f);
    juce::MemoryBlock state;
    setParameter (source, "gateX05", 0.3f);
    setParameter (source, "gateFilterOn", 1);
    setParameter (source, "gateStereoMode", 2);
    setParameter (source, "voiceTune1", 12);
    setParameter (source, "gateSwing", 37.5f);
    source.getStateInformation (state);
    restored.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
    for (const auto* id : { "gateStep05", "gateOffset05", "gateShape", "gateTension",
                            "gateCycle", "chorusAmount", "gateX05", "gateUsePositions",
                            "gateFilterOn", "gateStereoMode", "voiceTune1", "gateSwing" })
        if (! check (std::abs (source.parameters.getRawParameterValue (id)->load()
                              - restored.parameters.getRawParameterValue (id)->load()) < 0.0001f,
                     "Edited gate curves and independent knobs must survive session restore"))
            return false;
    auto legacy = source.parameters.copyState();
    legacy.removeChild (legacy.getChildWithProperty ("id", "gateUsePositions"), nullptr);
    legacy.removeChild (legacy.getChildWithProperty ("id", "gateSwing"), nullptr);
    juce::MemoryBlock oldState;
    juce::AudioProcessor::copyXmlToBinary (*legacy.createXml(), oldState);
    restored.setStateInformation (oldState.getData(), static_cast<int> (oldState.getSize()));
    if (! check (restored.parameters.getRawParameterValue ("gateUsePositions")->load() == 0
                 && restored.parameters.getRawParameterValue ("gateSwing")->load() == 0
                 && std::abs (restored.getGatePointOffset (4) - 0.21f) < 0.0001f,
                 "Old saved projects must still use their original grid-cell offsets"))
        return false;
    return true;
}

bool testGateSwing()
{
    using Processor = MKNoiseRhythmMachineProcessor;
    for (int pair = -8; pair <= 8; ++pair)
        for (float amount : { 0.0f, 50.0f, 100.0f })
        {
            const auto start = pair * 0.5;
            const auto delayedOffbeat = start + 0.25 + amount / 1200.0;
            if (! check (Processor::swingGatePosition (start, amount) == start
                         && std::abs (Processor::swingGatePosition (delayedOffbeat, amount) - (start + 0.25)) < 0.000001,
                         "Swing must delay offbeat sixteenths without moving eighth-note boundaries, including negative PPQ"))
                return false;
            auto previous = start;
            for (int step = 1; step <= 100; ++step)
            {
                const auto beat = start + step * 0.005;
                const auto position = Processor::swingGatePosition (beat, amount);
                if (! check (position > previous && (amount != 0 || position == beat),
                             "Swing timing must move forward continuously; zero swing must preserve exact timing"))
                    return false;
                previous = position;
            }
        }
    Processor straight, swung, neutral;
    TestPlayHead host;
    const auto preset = Processor::getGatePresetNames().indexOf ("1/16 Classic Gate");
    for (auto* processor : { &straight, &swung, &neutral })
    {
        processor->applyGatePreset (preset);
        processor->setPlayHead (&host);
    }
    setParameter (swung, "gateSwing", 100);
    setParameter (neutral, "gateSwing", 0);
    for (auto* processor : { &straight, &swung, &neutral })
        processor->prepareToPlay (8000, 8000);
    juce::AudioBuffer<float> a (2, 8000), b (2, 8000), c (2, 8000);
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
    straight.processBlock (a, midi);
    midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
    swung.processBlock (b, midi);
    midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
    neutral.processBlock (c, midi);
    for (int channel = 0; channel < 2; ++channel)
        for (int sample = 0; sample < 8000; ++sample)
            if (! check (a.getSample (channel, sample) == c.getSample (channel, sample),
                         "Default and explicit zero swing must produce identical audio"))
                return false;
    if (! check (energyIn (a, 1100, 1200) > 0 && energyIn (b, 1100, 1200) == 0
                 && energyIn (a, 1560, 1640) == 0 && energyIn (b, 1560, 1640) > 0
                 && energyIn (a, 2200, 2300) > 0 && energyIn (b, 2200, 2300) > 0,
                 "100% swing must move the second sixteenth-note onset from 1000 to 1333 samples at 120 BPM/8 kHz"))
        return false;
    swung.applyGatePreset (36);
    return check (swung.parameters.getRawParameterValue ("gateSwing")->load() == 100,
                  "Selecting a gate preset must retain the user's global swing amount");
}

bool testPitchShifter()
{
    for (const auto rate : { 8000.0, 44100.0, 48000.0 })
        for (const auto semitones : { -24.0f, -12.0f, 0.0f, 12.0f, 24.0f })
        {
            PitchShifter shifter;
            shifter.prepare (rate);
            int crossings = 0;
            float previous = 0;
            for (int sample = 0; sample < static_cast<int> (rate); ++sample)
            {
                const auto input = static_cast<float> (std::sin (sample * 400.0 * juce::MathConstants<double>::twoPi / rate));
                const auto output = shifter.process (input, semitones);
                if (! check (std::isfinite (output) && std::abs (output) < 1.001f, "Pitch shifting must stay finite and bounded"))
                    return false;
                if (semitones == 0 && ! check (output == input, "Zero semitones must be exact dry bypass"))
                    return false;
                if (sample >= static_cast<int> (rate / 2) && previous <= 0 && output > 0)
                    ++crossings;
                previous = output;
            }
            const auto measured = crossings * 2.0;
            const auto expected = 400.0 * std::pow (2.0, semitones / 12.0);
            if (! check (std::abs (measured - expected) <= 2.0,
                         "Actual sine frequency must shift by the requested octave ratio"))
            {
                std::cerr << "Rate " << rate << ", semitones " << semitones << ", Hz " << measured << " / " << expected << '\n';
                return false;
            }
        }
    for (int type = 0; type < 7; ++type)
        for (float pitch : { -24.0f, 24.0f })
        {
            MKNoiseRhythmMachineProcessor processor;
            setParameter (processor, "voiceType1", static_cast<float> (type));
            setParameter (processor, "voiceTune1", pitch);
            setParameter (processor, "gateOn", 0);
            processor.prepareToPlay (8000, 8000);
            juce::AudioBuffer<float> buffer (2, 8000);
            juce::MidiBuffer midi;
            midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 100);
            midi.addEvent (juce::MidiMessage::noteOff (1, 60), 6000);
            processor.processBlock (buffer, midi);
            if (! check (energyIn (buffer, 0, 100) == 0 && energyIn (buffer, 1000, 6000) > 0
                         && energyIn (buffer, 6041, 8000) == 0,
                         "All pitched noises must respect MIDI start/stop and remain finite"))
                return false;
        }
    return true;
}

float presetLevel (const GatePreset& preset, float phase)
{
    std::array<float, 32> levels {}, offsets {};
    for (size_t i = 0; i < preset.points.size(); ++i)
    {
        levels[i] = preset.points[i].y;
        offsets[i] = preset.points[i].x * static_cast<float> (preset.points.size() - 1) - static_cast<float> (i);
    }
    Pattern evaluator (7);
    return MKNoiseRhythmMachineProcessor::evaluateGateCurve (
        evaluator, levels, offsets, static_cast<int> (preset.points.size()), preset.shape, phase);
}

bool testPresetSemantics()
{
    MKNoiseRhythmMachineProcessor processor;
    const auto names = processor.getGatePresetNames();
    for (int index = 0; index < names.size(); ++index)
    {
        const auto preset = makeGatePreset (names[index]);
        processor.applyGatePreset (index);
        if (! check (preset.points.size() >= 8 && preset.points.size() <= 32 && preset.description.isNotEmpty(),
                     "Every named preset must have an authored, described, editable curve"))
            return false;
        for (size_t i = 0; i < preset.points.size(); ++i)
        {
            const auto& point = preset.points[i];
            if (! check (point.y >= 0 && point.y <= 1 && point.x >= 0 && point.x <= 1
                         && (i == 0 || point.x > preset.points[i - 1].x),
                         "Preset points must be ordered and normalized"))
            {
                std::cerr << names[index] << ", point " << i << '\n';
                return false;
            }
            const auto actualX = (static_cast<float> (i) + processor.getGatePointOffset (static_cast<int> (i)))
                               / static_cast<float> (preset.points.size() - 1);
            if (! check (std::abs (actualX - point.x) < 0.00002f
                         && std::abs (processor.getGateStep (static_cast<int> (i)) - point.y) < 0.0011f,
                         "Applying presets must preserve their actual curve times and levels"))
                return false;
        }
        for (int sample = 1; sample < 200; ++sample)
            if (! check (std::isfinite (presetLevel (preset, static_cast<float> (sample) / 200)),
                         "Every preset curve must evaluate to finite audio levels"))
                return false;
    }
    for (const auto* prefix : { "1/4 ", "1/8 ", "1/16 ", "1/32 ",
                               "1/8 Triplet ", "1/16 Triplet ", "1/8 Dotted ", "1/16 Dotted " })
    {
        const auto musical = juce::String (prefix);
        const auto family = musical.contains ("Triplet") || musical.contains ("Dotted");
        const auto classic = makeGatePreset (musical + (family ? "Gate" : "Classic Gate"));
        const auto trance = makeGatePreset (musical + (family ? "Trance" : "Trance Gate"));
        const auto pulse = makeGatePreset (musical + (family ? "Pulse" : "Pulse Gate"));
        const auto pump = makeGatePreset (musical + (family ? "Pump" : "Sidechain Pump"));
        if (! check (presetLevel (classic, 0.3f) == 1 && presetLevel (classic, 0.7f) == 0
                     && presetLevel (trance, 0.3f) == 0 && presetLevel (pulse, 0.2f) == 0
                     && presetLevel (pump, 0.1f) < presetLevel (pump, 0.8f),
                     "All timing families must distinguish classic, trance, pulse and recovering pump contours"))
            return false;
    }
    const auto soft = makeGatePreset ("1/4 Soft Pump"), deep = makeGatePreset ("1/4 Deep Pump"),
               hard = makeGatePreset ("1/4 Hard Pump");
    if (! check (presetLevel (soft, 0) >= 0.45f && presetLevel (hard, 0.5f) < presetLevel (deep, 0.5f)
                 && presetLevel (deep, 0.5f) < presetLevel (soft, 0.5f),
                 "Soft, deep and hard pumps must have distinct depths and recoveries"))
        return false;
    const auto swing = makeGatePreset ("1/8 Swing Gate");
    if (! check (presetLevel (swing, 0.52f) == 0 && presetLevel (swing, 0.69f) > 0,
                 "Swing must displace the second pulse to two-thirds of the pair"))
        return false;
    for (const auto* name : { "Fade In", "Fade Out", "Fade Out In", "Reverse Sidechain", "1st Stp Fade" })
    {
        const auto p = makeGatePreset (name);
        if (! check (juce::String (name) == "Fade In" ? presetLevel (p, 0.2f) < presetLevel (p, 0.8f)
                      : juce::String (name) == "Fade Out In" ? presetLevel (p, 0.5f) == 0
                      : presetLevel (p, 0.01f) > presetLevel (p, 0.9f), "Fades must follow their named directions"))
            return false;
    }
    if (! check (presetLevel (makeGatePreset ("1st Step"), 0.04f) == 1
                 && presetLevel (makeGatePreset ("1st Step"), 0.1f) == 0
                 && presetLevel (makeGatePreset ("2 Step Gate"), 0.1f) == 1
                 && presetLevel (makeGatePreset ("End Gate"), 0.9f) == 0
                 && presetLevel (makeGatePreset ("End Gate"), 0.97f) == 1,
                 "First-step, two-step and end gates must open only their named phrase sections"))
        return false;
    const auto onsetIntervals = [] (const GatePreset& p) {
        std::vector<float> intervals;
        float previous = -1;
        for (size_t i = 0; i < p.points.size(); ++i)
            if (p.points[i].x < 0.99f && p.points[i].y > 0 && (i == 0 || p.points[i - 1].y == 0))
            {
                if (previous >= 0) intervals.push_back (p.points[i].x - previous);
                previous = p.points[i].x;
            }
        return intervals;
    };
    for (const auto* name : { "Spd Up Gate", "Slw Dn Gate" })
    {
        const auto intervals = onsetIntervals (makeGatePreset (name));
        for (size_t i = 1; i < intervals.size(); ++i)
            if (! check (juce::String (name) == "Spd Up Gate" ? intervals[i] < intervals[i - 1]
                                                            : intervals[i] > intervals[i - 1],
                         "Speed-up/slow-down must change successive pulse intervals monotonically"))
                return false;
    }
    for (const auto* name : { "Random Rhythm Gate", "Experimental Glitch Gate" })
    {
        const auto first = makeGatePreset (name), second = makeGatePreset (name);
        for (int i = 0; i < 100; ++i)
            if (! check (presetLevel (first, i / 100.0f) == presetLevel (second, i / 100.0f),
                         "Random/glitch presets must be reproducible on recall"))
                return false;
    }
    processor.applyGatePreset (36);
    for (int count : { 32, 8 })
    {
        processor.setGatePointCount (count);
        std::array<float, 32> levels {}, offsets {};
        for (int i = 0; i < count; ++i)
        {
            levels[static_cast<size_t> (i)] = processor.getGateStep (i);
            offsets[static_cast<size_t> (i)] = processor.getGatePointOffset (i);
        }
        Pattern evaluator (9);
        if (! check (processor.getGateStepCount() == count
                     && processor.evaluateGateCurve (evaluator, levels, offsets, count, 0, 0.3f) == 1
                     && processor.evaluateGateCurve (evaluator, levels, offsets, count, 0, 0.7f) == 0,
                     "Changing point count must preserve an authored square-gate contour"))
            return false;
    }
    return check (makeGatePreset ("1/8 Triplet Gate").cycle == 7
                  && makeGatePreset ("1/8 Dotted Gate").cycle == 9
                  && makeGatePreset ("1/4 Bt Gate").cycle == 5,
                  "Triplet, dotted and literal beat-fraction presets must use their correct cycle lengths");
}

bool testStereoAndFilterGate()
{
    MKNoiseRhythmMachineProcessor processor;
    const auto names = processor.getGatePresetNames();
    processor.applyGatePreset (names.indexOf ("Stereo Alternating Gate"));
    processor.prepareToPlay (8000, 8000);
    juce::AudioBuffer<float> stereo (2, 8000);
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
    processor.processBlock (stereo, midi);
    double leftFirst = 0, rightFirst = 0, leftSecond = 0, rightSecond = 0;
    for (int i = 200; i < 3800; ++i)
    {
        leftFirst += std::pow (stereo.getSample (0, i), 2);
        rightFirst += std::pow (stereo.getSample (1, i), 2);
        leftSecond += std::pow (stereo.getSample (0, i + 4000), 2);
        rightSecond += std::pow (stereo.getSample (1, i + 4000), 2);
    }
    if (! check (leftFirst > 0.01 && rightFirst == 0 && leftSecond == 0 && rightSecond > 0.01,
                 "Stereo alternating preset must actually switch audible channels"))
        return false;
    processor.applyGatePreset (names.indexOf ("Ping Pong Gate"));
    processor.prepareToPlay (8000, 8000);
    midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
    processor.processBlock (stereo, midi);
    if (! check (std::abs (stereo.getSample (1, 400)) == 0
                 && std::abs (stereo.getSample (0, 4400)) == 0
                 && energyIn (stereo, 200, 800) > 0 && energyIn (stereo, 4200, 4800) > 0
                 && energyIn (stereo, 1500, 3500) == 0 && energyIn (stereo, 5500, 7500) == 0,
                 "Ping-pong gate must bounce short hits between channels with rests"))
        return false;
    MKNoiseRhythmMachineProcessor filtered, amplitude;
    for (auto* p : { &filtered, &amplitude })
    {
        p->applyGatePreset (names.indexOf ("1/8 Triplet Filter Gate"));
        p->prepareToPlay (8000, 8000);
    }
    setParameter (amplitude, "gateFilterOn", 0);
    juce::AudioBuffer<float> a (2, 8000), b (2, 8000);
    midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
    filtered.processBlock (a, midi);
    midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
    amplitude.processBlock (b, midi);
    if (! check (energyIn (a, 100, 8000) > 0 && energyIn (a, 100, 8000) < energyIn (b, 100, 8000) * 0.9,
                 "Filter Gate presets must apply real lowpass filtering, not just rename amplitude gating"))
        return false;
    processor.applyGatePreset (36);
    return check (processor.parameters.getRawParameterValue ("gateFilterOn")->load() == 0
                  && processor.parameters.getRawParameterValue ("gateStereoMode")->load() == 0,
                  "Ordinary presets must reset stereo and filter gating");
}

bool testLargeKnobLayout()
{
    MKNoiseRhythmMachineProcessor processor;
    MKNoiseRhythmMachineEditor editor (processor);

    if (! check (editor.isResizable() && editor.getConstrainer() != nullptr
                 && std::abs (editor.getConstrainer()->getFixedAspectRatio() - 960.0f / 1076.0f) < 0.001f,
                 "The editor must scale freely while keeping the panel aspect ratio"))
        return false;

    for (const auto width : { 768, 900, 1200 })
    {
        editor.setSize (width, 600);
        auto* view = dynamic_cast<juce::Viewport*> (editor.getChildComponent (0));
        if (! check (view != nullptr && view->getViewedComponent()->getHeight() > editor.getHeight(),
                     "Short iPad host windows must scroll the complete enlarged interface"))
            return false;
        int knobs = 0;
        juce::Slider* swing = nullptr;
        auto* panel = view->getViewedComponent();
        const auto designScale = static_cast<float> (panel->getWidth()) / 960.0f;
        for (int i = 0; i < panel->getNumChildComponents(); ++i)
            if (auto* slider = dynamic_cast<juce::Slider*> (panel->getChildComponent (i)); slider != nullptr)
            {
                if (slider->getName() == "Gate Swing")
                    swing = slider;
                if (! slider->isRotary())
                    continue;
                ++knobs;
                const auto bounds = slider->getLookAndFeel().getSliderLayout (*slider).sliderBounds.toFloat().reduced (10);
                if (! check (juce::jmin (bounds.getWidth(), bounds.getHeight()) >= 3.0f * 28.0f * designScale - 1.5f,
                             "Actual drawable effect dial diameter must stay at least three times the old diameter"))
                    return false;
            }
        if (! check (knobs == 10, "All ten independent effects must retain enlarged knobs"))
            return false;
        if (! check (swing != nullptr && swing->isVertical() && swing->getHeight() >= 110
                     && swing->getX() * 4 < panel->getWidth(),
                     "Swing must be a tall vertical slider at the bottom-left at desktop and iPad widths"))
            return false;
        for (auto* child : panel->getChildren())
            if (child != swing && ! check (! child->getBounds().intersects (swing->getBounds()),
                                          "The swing slider must not overlap any other UI control"))
                return false;
        swing->setValue (50, juce::sendNotificationSync);
        if (! check (processor.parameters.getRawParameterValue ("gateSwing")->load() == 50,
                     "The swing slider must control its automatable DSP parameter"))
            return false;
        view->setViewPosition (0, panel->getHeight());
        if (! check (view->getViewPositionY() > 0 && view->getViewPositionY() + view->getViewHeight() >= panel->getHeight(),
                     "Bottom gate controls must be reachable by scrolling"))
            return false;
    }
    return true;
}

bool testPresetMenu()
{
    MKNoiseRhythmMachineProcessor processor;
    MKNoiseRhythmMachineEditor editor (processor);
    const auto names = processor.getGatePresetNames();
    auto* view = dynamic_cast<juce::Viewport*> (editor.getChildComponent (0));
    juce::ComboBox* menu = nullptr;
    if (view != nullptr)
        for (auto* child : view->getViewedComponent()->getChildren())
            if (auto* box = dynamic_cast<juce::ComboBox*> (child); box != nullptr && box->getNumItems() == names.size())
                menu = box;
    if (! check (menu != nullptr, "The actual preset menu must expose every supplied preset"))
        return false;

    processor.prepareToPlay (8000, 32000);
    juce::AudioBuffer<float> buffer (2, 32000);
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
    auto* messages = juce::MessageManager::getInstance();
    bool passed = true;
    int selections = 0;
    std::function<void()> selectNext;
    const auto applySelection = menu->onChange;
    menu->onChange = [&, applySelection]
    {
        if (! passed || selections == names.size())
            return;
        applySelection();
        const auto index = (selections + 1) % names.size();
        const auto expected = makeGatePreset (names[index]);
        if (! check (menu->getSelectedItemIndex() == index
                     && juce::roundToInt (processor.parameters.getRawParameterValue ("gatePreset")->load()) == index
                     && processor.getGateStepCount() == static_cast<int> (expected.points.size())
                     && processor.parameters.getRawParameterValue ("gateCycle")->load() == expected.cycle,
                     "Every real menu selection must apply its preset, not just change the label"))
        {
            std::cerr << "Requested " << index << ", menu " << menu->getSelectedItemIndex()
                      << ", applied " << processor.parameters.getRawParameterValue ("gatePreset")->load() << '\n';
            passed = false;
            messages->stopDispatchLoop();
            return;
        }
        processor.processBlock (buffer, midi);
        if (! check (energyIn (buffer, 0, buffer.getNumSamples()) > 0,
                     "Every preset selected through the menu must produce finite gated audio with a held MIDI note"))
        {
            std::cerr << names[index] << '\n';
            passed = false;
            messages->stopDispatchLoop();
            return;
        }
        if (++selections == names.size())
        {
            processor.applyGatePreset (36);
            processor.setGateStep (0, 0.123f);
            juce::Timer::callAfterDelay (60, [&]
            {
                passed = check (menu->getSelectedItemIndex() == 36
                                && std::abs (processor.getGateStep (0) - 0.123f) < 0.001f,
                                "Refreshing an externally changed preset must preserve subsequent custom curve edits");
                messages->stopDispatchLoop();
            });
        }
        else
            messages->callAsync ([&] { selectNext(); });
    };
    selectNext = [&]
    {
        const auto index = (selections + 1) % names.size();
        menu->setSelectedItemIndex (index, juce::sendNotificationAsync);
        if (selections == 0)
        {
            juce::Thread::sleep (60);
            juce::Timer::callPendingTimersSynchronously();
            if (! check (menu->getSelectedItemIndex() == index,
                         "A timer refresh must not revert a menu selection before its asynchronous callback"))
            {
                passed = false;
                messages->stopDispatchLoop();
                return;
            }
        }
    };
    juce::Timer::callAfterDelay (100, [&] { selectNext(); });
    messages->runDispatchLoop();
    return passed && selections == names.size();
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    MKNoiseRhythmMachineProcessor processor;
    processor.prepareToPlay (44100.0, 512);

    juce::AudioBuffer<float> buffer (2, 8192);
    juce::MidiBuffer midi;
    processor.processBlock (buffer, midi);

    if (! check (energyIn (buffer, 0, 8192) == 0.0, "Must be silent without MIDI notes"))
        return 1;

    processor.parameters.getParameter ("gateOn")->setValueNotifyingHost (0.0f);
    processor.parameters.getParameter ("delayOn")->setValueNotifyingHost (1.0f);
    midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 512);
    midi.addEvent (juce::MidiMessage::noteOff (1, 60), 4096);
    processor.processBlock (buffer, midi);
    if (! check (energyIn (buffer, 0, 512) == 0.0, "Note-on must respect its sample offset")
        || ! check (energyIn (buffer, 800, 4096) > 0.001, "Held note must produce sound")
        || ! check (energyIn (buffer, 4096, 4300) > 0.0, "Note-off should have a short release")
        || ! check (energyIn (buffer, 4317, 8192) == 0.0, "Note-off must silence effect tails"))
        return 2;

    midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
    midi.addEvent (juce::MidiMessage::noteOn (2, 60, 1.0f), 512);
    midi.addEvent (juce::MidiMessage::noteOff (1, 60), 1024);
    processor.processBlock (buffer, midi);
    if (! check (energyIn (buffer, 1400, 8192) > 0.001, "Another channel's held note must keep playing"))
        return 5;
    processor.processBlock (buffer, midi);
    if (! check (energyIn (buffer, 0, 8192) > 0.001, "Held notes must persist across blocks"))
        return 6;
    midi.addEvent (juce::MidiMessage::noteOn (2, 60, 0.0f), 512);
    processor.processBlock (buffer, midi);
    if (! check (energyIn (buffer, 800, 8192) == 0.0, "Zero-velocity note-on must release the note"))
        return 7;

    midi.addEvent (juce::MidiMessage::noteOn (1, 62, 1.0f), 0);
    midi.addEvent (juce::MidiMessage::noteOn (1, 65, 1.0f), 512);
    midi.addEvent (juce::MidiMessage::noteOff (1, 62), 1024);
    midi.addEvent (juce::MidiMessage::allNotesOff (1), 4096);
    processor.processBlock (buffer, midi);
    if (! check (energyIn (buffer, 1400, 4096) > 0.001, "Overlapping pitches must keep playing")
        || ! check (energyIn (buffer, 4317, 8192) == 0.0, "All-notes-off must release held notes"))
        return 8;

    midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
    midi.addEvent (juce::MidiMessage::allSoundOff (1), 4096);
    processor.processBlock (buffer, midi);
    if (! check (energyIn (buffer, 4317, 8192) == 0.0, "All-sound-off must stop the noise"))
        return 9;
    midi.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
    processor.processBlock (buffer, midi);
    processor.prepareToPlay (44100.0, 512);
    processor.processBlock (buffer, midi);
    if (! check (energyIn (buffer, 0, 8192) == 0.0, "Preparing playback must reset held notes"))
        return 10;

    processor.setGateStep (7, 0.73f);
    if (std::abs (processor.getGateStep (7) - 0.73f) > 0.002f)
        return 3;

    processor.randomizeGate();
    for (int step = 0; step < 32; ++step)
        if (processor.getGateStep (step) < 0.0f || processor.getGateStep (step) > 1.0f)
            return 4;

    processor.releaseResources();
    if (! testGateTiming() || ! testGate12Curves() || ! testEffectKnobs() || ! testCurveState()
        || ! testPitchShifter() || ! testGateSwing() || ! testPresetSemantics() || ! testStereoAndFilterGate() || ! testLargeKnobLayout()
        || ! testPresetMenu())
        return 11;
    return 0;
}
