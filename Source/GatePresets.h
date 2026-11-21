#pragma once

#include <juce_core/juce_core.h>
#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <vector>

struct GatePreset
{
    struct Point { float x, y; };
    std::vector<Point> points;
    int cycle = 1, shape = 0, stereo = 0;
    float attackMs = 0.5f, releaseMs = 0.5f;
    bool filter = false;
    juce::String description;
};

inline GatePreset makeGatePreset (const juce::String& name)
{
    GatePreset p;
    const auto curve = [&p] (int cycle, const std::function<float(float)>& function,
                             const juce::String& description)
    {
        p.cycle = cycle;
        p.shape = 1;
        p.description = description;
        for (int i = 0; i <= 16; ++i)
        {
            const auto phase = static_cast<float> (i) / 16.0f;
            p.points.push_back ({ phase, function (phase) });
        }
    };
    const auto pulses = [&p] (int cycle, std::initializer_list<GatePreset::Point> events,
                              float width, const juce::String& description)
    {
        p.cycle = cycle;
        p.description = description;
        p.points = { { 0.0f, 0.0f } };
        for (const auto& event : events)
        {
            if (event.x == 0.0f) p.points.front().y = event.y;
            else p.points.push_back (event);
            if (event.x + width < 1.0f)
                p.points.push_back ({ event.x + width, 0.0f });
        }
        p.points.push_back ({ 1.0f, p.points.front().y });
    };
    struct Prefix { const char* text; int cycle, pairCycle; };
    constexpr Prefix prefixes[] {
        { "1/4 ", 3, 2 }, { "1/8 Triplet ", 7, 13 }, { "1/16 Triplet ", 8, 7 },
        { "1/8 Dotted ", 9, 14 }, { "1/16 Dotted ", 10, 9 },
        { "1/8 ", 4, 3 }, { "1/16 ", 5, 4 }, { "1/32 ", 6, 5 }
    };
    bool handled = false;
    for (const auto& prefix : prefixes)
    {
        if (! name.startsWith (prefix.text)) continue;
        const auto kind = name.substring (juce::String (prefix.text).length());
        const auto timing = juce::String (prefix.text).trim();
        if (kind == "Gate" || kind == "Classic Gate")
            pulses (prefix.cycle, { { 0, 1 } }, 0.5f, timing + ": on-beat, 50% open.");
        else if (kind == "Trance" || kind == "Trance Gate")
            pulses (prefix.cycle, { { 0, 1 } }, 0.25f, timing + ": short on-beat trance chops, 25% open.");
        else if (kind == "Offbeat Gate")
            pulses (prefix.cycle, { { 0.5f, 1 } }, 0.5f, timing + ": opens halfway between beats.");
        else if (kind == "Pump" || kind == "Sidechain Pump" || kind == "Deep Pump"
                 || kind == "Soft Pump" || kind == "Hard Pump")
        {
            const auto exponent = kind == "Deep Pump" ? 2.0f : kind == "Hard Pump" ? 4.0f : 0.65f;
            const auto floor = kind == "Soft Pump" ? 0.45f : 0.0f;
            curve (prefix.cycle, [exponent, floor] (float x) { return floor + (1 - floor) * std::pow (x, exponent); },
                   timing + ": " + kind + ", duck on the beat and recover before the next.");
        }
        else if (kind == "Swing" || kind == "Swing Gate")
            pulses (prefix.pairCycle, { { 0, 1 }, { 2.0f / 3.0f, 0.8f } }, 0.12f,
                    timing + ": a two-note pair swung 2:1; second onset delayed, not just quieter.");
        else if (kind == "Alternating" || kind == "Alternating Gate")
            pulses (prefix.pairCycle, { { 0, 1 }, { 0.5f, 0.45f } }, 0.25f,
                    timing + ": alternating strong and weak pulses.");
        else if (kind == "Pulse" || kind == "Pulse Gate")
            pulses (prefix.cycle, { { 0, 1 } }, 0.125f, timing + ": narrow 12.5% pulse.");
        else if (kind == "Chopper" || kind == "Rapid Chopper")
            pulses (prefix.cycle, { { 0, 1 }, { 0.25f, 1 }, { 0.5f, 1 }, { 0.75f, 1 } }, 0.125f,
                    timing + ": four equally spaced, 50%-duty sub-chops per cycle.");
        else if (kind == "Stutter" || kind == "Stutter Gate")
            pulses (prefix.cycle, { { 0, 1 }, { 0.125f, 0.8f }, { 0.25f, 0.6f } }, 0.065f,
                    timing + ": three decaying repeats followed by a rest.");
        else if (kind == "Machine Gun Gate")
            pulses (prefix.cycle, { { 0, 1 }, { 0.125f, 1 }, { 0.25f, 1 }, { 0.375f, 1 },
                                    { 0.5f, 1 }, { 0.625f, 1 }, { 0.75f, 1 }, { 0.875f, 1 } }, 0.045f,
                    timing + ": eight very short equal shots per cycle.");
        else if (kind == "Micro Gate")
        {
            pulses (prefix.cycle, { { 0, 1 } }, 0.04f, timing + ": one 4%-duty micro slice.");
            p.attackMs = p.releaseMs = 0.1f;
        }
        else if (kind == "Glitch" || kind == "Glitch Gate")
            pulses (prefix.cycle, { { 0, 1 }, { 0.19f, 0.4f }, { 0.55f, 0.8f }, { 0.71f, 0.6f } }, 0.045f,
                    timing + ": irregular, accented glitch slices.");
        else if (kind == "Filter Gate")
        {
            curve (prefix.cycle, [] (float x) { return x < 0.5f ? 1.0f - 2.0f * x : 0.0f; },
                   timing + ": descending lowpass pluck; real 12 dB/oct filter plus amplitude gate.");
            p.filter = true;
        }
        else if (kind == "Dynamic")
            pulses (prefix.pairCycle, { { 0, 1 }, { 0.5f, 0.65f } }, 0.3f, "Quarter-note accented dynamic pair.");
        else if (kind == "Gate Out")
            curve (prefix.cycle, [] (float x) { return x < 0.5f ? 1.0f : 2.0f * (1.0f - x); },
                   "Quarter-note gate held open, then fading out.");
        else if (kind == "Gate In")
            curve (prefix.cycle, [] (float x) { return juce::jmin (1.0f, x * 2.0f); },
                   "Quarter-note gate fading in, then held open.");
        else
            continue;
        handled = true;
        break;
    }
    const juce::StringArray basicNames { "2 Beat Gate", "1 Beat Gate", "1/2 Bt Gate", "1/3 Bt Gate",
        "1/4 Bt Gate", "1/6 Bt Gate", "1/8 Bt Gate", "1/12 Bt Gate", "1/16 Bt Gate" };
    const auto basic = basicNames.indexOf (name);
    if (! handled && basic >= 0)
    {
        constexpr int cycles[] { 2, 3, 4, 7, 5, 8, 6, 11, 12 };
        pulses (cycles[basic], { { 0, 1 } }, 0.5f, name + ": literal beat fraction, 50% open.");
        handled = true;
    }
    if (! handled)
    {
        if (name == "Trance Gt 1" || name == "Classic Supersaw Gate")
            pulses (1, { { 0, 1 }, { 0.25f, 0.8f }, { 0.5f, 1 }, { 0.75f, 0.8f } }, 0.075f,
                    "One bar: short quarter-note chops with alternating accents.");
        else if (name == "Trance Gt 2")
            pulses (1, { { 0, 1 }, { 0.125f, 0.5f }, { 0.375f, 0.9f }, { 0.5f, 1 },
                        { 0.625f, 0.5f }, { 0.875f, 0.9f } }, 0.045f, "One bar: syncopated eighth-note trance chops.");
        else if (name == "Trance Gt 3")
            pulses (1, { { 0, 1 }, { 0.1875f, 0.8f }, { 0.375f, 0.8f }, { 0.5f, 1 },
                        { 0.6875f, 0.8f }, { 0.875f, 0.8f } }, 0.045f, "One bar: a 3-3-2 sixteenth-note rhythm, repeated twice.");
        else if (name == "Trance Gt 4")
            pulses (1, { { 0, 1 }, { 0.0625f, 0.65f }, { 0.25f, 1 }, { 0.3125f, 0.65f },
                        { 0.5f, 1 }, { 0.5625f, 0.65f }, { 0.75f, 1 }, { 0.8125f, 0.65f } }, 0.03f,
                    "One bar: accented sixteenth-note double chops on each beat.");
        else if (name == "Saw Gate 1" || name == "Sawtooth Gate")
            curve (3, [] (float x) { return 1 - x; }, "Quarter-note falling sawtooth.");
        else if (name == "Saw Gate 2")
        {
            p.cycle = 3; p.shape = 1; p.description = "Two falling sawtooth teeth per quarter note.";
            p.points = { { 0, 1 }, { 0.49999f, 0 }, { 0.5f, 1 }, { 1, 0 } };
        }
        else if (name == "1st Step")
            pulses (1, { { 0, 1 } }, 1.0f / 16, "One bar: only the first sixteenth-note step opens.");
        else if (name == "1st Stp Fade")
        {
            p.shape = 1; p.description = "One bar: the first sixteenth-note step fades to silence.";
            p.points = { { 0, 1 }, { 0.0625f, 0 }, { 1, 0 } };
        }
        else if (name == "2 Step Gate")
            pulses (1, { { 0, 1 } }, 0.125f, "One bar: the first two sixteenth-note steps open, then silence.");
        else if (name == "Off Beat")
            pulses (3, { { 0.5f, 1 } }, 0.5f, "Quarter-note offbeat: second half of every beat opens.");
        else if (name == "Spd Up Gate" || name == "Slw Dn Gate")
        {
            p.description = name == "Spd Up Gate" ? "One bar: pulse intervals steadily shorten."
                                                   : "One bar: pulse intervals steadily lengthen.";
            const std::vector<float> starts = name == "Spd Up Gate"
                ? std::vector<float> { 0, 0.3f, 0.54f, 0.72f, 0.84f, 0.92f }
                : std::vector<float> { 0, 0.08f, 0.2f, 0.38f, 0.62f, 0.92f };
            for (const auto x : starts) { p.points.push_back ({ x, 1 }); p.points.push_back ({ x + 0.035f, 0 }); }
            p.points.push_back ({ 1, 1 });
        }
        else if (name == "End Fade")
        {
            p.shape = 1; p.description = "One bar: held open for three beats, fades during the final beat.";
            p.points = { { 0, 1 }, { 0.75f, 1 }, { 1, 0 } };
        }
        else if (name == "End Gate")
            pulses (1, { { 0.9375f, 1 } }, 0.0625f, "One bar: only the final sixteenth-note step opens.");
        else if (name == "Fade In" || name == "Fade In Gate" || name == "Ramp Gate")
            curve (1, [] (float x) { return x; }, "One-bar linear fade in.");
        else if (name == "Fade Out" || name == "Fade Out Gate")
            curve (1, [] (float x) { return 1 - x; }, "One-bar linear fade out.");
        else if (name == "Fade Out In")
            curve (1, [] (float x) { return std::abs (2 * x - 1); }, "One bar: fades out to midpoint, then back in.");
        else if (name == "Tremelo Slw" || name == "Tremelo Fst" || name == "Tremolo Gate" || name == "AM")
        {
            const auto cycle = name == "Tremelo Slw" ? 2 : name == "Tremelo Fst" ? 5 : name == "AM" ? 12 : 3;
            const auto floor = name == "AM" ? 0.0f : 0.15f;
            curve (cycle, [floor] (float x) {
                return floor + (1 - floor) * (0.5f + 0.5f * std::cos (x * juce::MathConstants<float>::twoPi));
            }, name == "AM" ? "Unipolar amplitude modulation: 32 Hz at 120 BPM."
                           : name == "Tremelo Slw" ? "Slow sinusoidal tremolo: once every two beats."
                           : name == "Tremelo Fst" ? "Fast sinusoidal tremolo: once every sixteenth note."
                                                  : "Quarter-note sinusoidal tremolo.");
        }
        else if (name == "Sidechain" || name == "Festival Pump" || name == "Reverse Sidechain"
                 || name == "Ghost Sidechain" || name == "Rhythmic Ducking")
            curve (3, [name] (float x) {
                if (name == "Reverse Sidechain") return std::pow (1 - x, 0.65f);
                if (name == "Ghost Sidechain") return 0.7f + 0.3f * std::pow (x, 0.65f);
                if (name == "Rhythmic Ducking") return 0.35f + 0.65f * x;
                return std::pow (x, name == "Festival Pump" ? 2.5f : 0.65f);
            }, name == "Reverse Sidechain" ? "Reverse pump: fades down towards the next beat."
             : name == "Ghost Sidechain" ? "Subtle quarter-note duck: minimum level 70%."
             : name == "Rhythmic Ducking" ? "Quarter-note linear duck and recovery: minimum level 35%."
             : name == "Festival Pump" ? "Quarter-note deep festival pump with a late recovery."
                                      : "Quarter-note sidechain pump: duck on beat, recover afterwards.");
        else if (name == "Square Wave Gate")
            pulses (3, { { 0, 1 } }, 0.5f, "Quarter-note square wave: 50% open.");
        else if (name == "Stereo Alternating Gate")
        {
            pulses (2, { { 0, 1 } }, 0.5f, "Left/right alternate each quarter note; stereo amplitude gating.");
            p.stereo = 1;
        }
        else if (name == "Ping Pong Gate")
        {
            pulses (2, { { 0, 1 } }, 0.125f, "Short quarter-note hits bouncing left/right with rests.");
            p.stereo = 2;
        }
        else if (name == "Drum Loop")
            pulses (1, { { 0, 1 }, { 0.125f, 0.3f }, { 0.25f, 0.8f }, { 0.375f, 0.3f },
                        { 0.5f, 1 }, { 0.625f, 0.3f }, { 0.75f, 0.8f }, { 0.875f, 0.45f } }, 0.04f,
                    "One bar: accented kick/snare-like beats with quieter offbeat hats.");
        else if (name == "Copter")
            pulses (5, { { 0, 1 }, { 0.5f, 0.65f } }, 0.17f,
                    "Rotor-like double throb per sixteenth note: 16 hits/second at 120 BPM.");
        else if (name == "Euphoric Trance Gate")
            pulses (1, { { 0, 1 }, { 0.125f, 0.7f }, { 0.25f, 0.85f }, { 0.375f, 0.7f },
                        { 0.5f, 1 }, { 0.625f, 0.7f }, { 0.75f, 0.85f }, { 0.875f, 0.7f } }, 0.07f,
                    "One bar: broad eighth-note trance chops with beat accents.");
        else if (name == "Uplifting Trance Gate")
            pulses (1, { { 0, 0.5f }, { 0.125f, 0.6f }, { 0.25f, 0.7f }, { 0.375f, 0.8f },
                        { 0.5f, 0.85f }, { 0.625f, 0.9f }, { 0.75f, 0.95f }, { 0.875f, 1 } }, 0.045f,
                    "One bar: eighth-note chops increasing in level towards the last beat.");
        else if (name == "Progressive Gate")
            pulses (1, { { 0, 0.7f }, { 0.1875f, 0.85f }, { 0.375f, 0.9f }, { 0.5625f, 1 },
                        { 0.75f, 0.85f }, { 0.875f, 0.6f } }, 0.085f, "One bar: spacious syncopated progressive rhythm.");
        else if (name == "Psytrance Gate")
            pulses (3, { { 0, 0.4f }, { 0.25f, 1 }, { 0.5f, 0.8f }, { 0.75f, 0.9f } }, 0.1f,
                    "One beat: quiet downbeat and three tight, rolling sixteenth-note chops.");
        else if (name == "Goa Trance Gate")
            pulses (1, { { 0, 1 }, { 0.125f, 0.65f }, { 0.3125f, 0.9f }, { 0.4375f, 0.55f },
                        { 0.625f, 1 }, { 0.75f, 0.7f }, { 0.9375f, 0.85f } }, 0.04f,
                    "One bar: interlocking, unevenly accented Goa-style chops.");
        else if (name == "Random Rhythm Gate" || name == "Experimental Glitch Gate")
        {
            auto random = name == "Random Rhythm Gate" ? 0x54ad3271u : 0x41cf287bu;
            const auto next = [&random] { random ^= random << 13; random ^= random >> 17; random ^= random << 5; return random; };
            p.description = name == "Random Rhythm Gate" ? "One bar: reproducible random sixteenth-note rhythm."
                                                         : "One bar: reproducible irregular micro-slices and accents.";
            for (int i = 0; i < 16; ++i)
            {
                if (i != 0 && next() % 3 == 0) continue;
                const auto x = static_cast<float> (i) / 16;
                const auto offset = name == "Experimental Glitch Gate" ? static_cast<float> (next() % 200) / 10000 : 0.0f;
                const auto start = x + offset;
                p.points.push_back ({ start, 0.4f + static_cast<float> (next() % 600) / 1000 });
                p.points.push_back ({ start + (name == "Experimental Glitch Gate" ? 0.012f : 0.04f), 0 });
            }
            if (p.points.front().x > 0) p.points.insert (p.points.begin(), { 0, 0 });
            p.points.push_back ({ 1, p.points.front().y });
        }
        else
            throw std::invalid_argument ("Undefined gate preset: " + name.toStdString());
    }
    // Extra editable points subdivide existing segments without changing their contour.
    while (p.points.size() < 8)
    {
        size_t longest = 0;
        for (size_t i = 1; i + 1 < p.points.size(); ++i)
            if (p.points[i + 1].x - p.points[i].x > p.points[longest + 1].x - p.points[longest].x)
                longest = i;
        const auto first = p.points[longest], second = p.points[longest + 1];
        p.points.insert (p.points.begin() + static_cast<std::ptrdiff_t> (longest + 1),
                         { (first.x + second.x) * 0.5f, p.shape == 0 ? first.y : (first.y + second.y) * 0.5f });
    }
    if (p.points.size() > 32)
        throw std::logic_error ("Gate preset exceeds 32 editable points: " + name.toStdString());
    return p;
}
