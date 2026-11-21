#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include <atomic>
#include <vector>
#include <Pattern.h>
#include "PitchShifter.h"

class MKNoiseRhythmMachineProcessor final : public juce::AudioProcessor
{
public:
    MKNoiseRhythmMachineProcessor();

    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "MK NOISE RhYTM Machine"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.005; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void setGateStep (int step, float value);
    float getGateStep (int step) const;
    int getGateStepCount() const;
    void setGatePointCount (int count);
    void applyGatePreset (int presetIndex);
    void randomizeGate();
    static juce::StringArray getGatePresetNames();
    juce::String getGatePresetDescription() const;
    float getGatePointOffset (int point) const;
    juce::String gatePositionParameterID (int point) const;
    static juce::String effectAmountParameterID (int effect);
    static juce::String gateOffsetParameterID (int point);
    static float evaluateGateCurve (Pattern& evaluator, const std::array<float, 32>& levels,
                                    const std::array<float, 32>& offsets,
                                    int count, int shape, float phase, float tension = 0.0f);
    float getGatePlaybackPhase() const { return gatePlaybackPhase.load(); }
    static double swingGatePosition (double beats, float amount) noexcept;

    juce::AudioProcessorValueTreeState parameters;

private:
    struct VoiceState
    {
        uint32_t randomState = 1;
        float pinkA = 0.0f, pinkB = 0.0f, pinkC = 0.0f;
        float lowState = 0.0f, highState = 0.0f;
        double phase = 0.0;
        int crackleCountdown = 0;
        float crackle = 0.0f;
    };

    struct Controls
    {
        std::array<bool, 3> voiceOn {};
        std::array<int, 3> voiceType {};
        std::array<float, 3> voiceGain {}, voicePan {}, voiceTune {};
        std::array<float, 32> gate {};
        std::array<float, 32> gateOffsets {};
        std::array<bool, 10> effectOn {};
        std::array<float, 10> effectAmounts {};
        int gateSteps = 16;
        int gateShape = 0;
        bool gateRetrigger = false;
        double gateCycleBeats = 4.0;
        bool gateOn = true;
        bool gateFilterOn = false;
        int gateStereoMode = 0;
        float gateAttack = 2.0f, gateRelease = 15.0f;
        float gateTension = 0.0f;
        float effectRate = 0.3f;
        float delayTime = 250.0f, stereoWidth = 1.0f, outputGain = 0.8f;
    };

    static juce::String gateStepParameterID (int step);
    float nextNoise (VoiceState&) noexcept;
    float generateVoiceSample (VoiceState&, int type, float tune) noexcept;
    float readDelay (const std::vector<float>&, float delaySamples, int writePosition) const noexcept;
    void updateControls (Controls&);
    void handleMidiMessage (const juce::MidiMessage&);

    std::array<VoiceState, 3> voices;
    std::array<PitchShifter, 3> pitchShifters;
    std::array<juce::SmoothedValue<float>, 3> pitchSmoothing;
    std::array<std::array<float, 4>, 2> phaserState {};
    std::array<float, 2> pitchFilterState {}, autoFilterState {}, compressorEnvelope {};
    std::array<std::vector<float>, 2> modulationBuffer, flangerBuffer, delayBuffer;
    double currentSampleRate = 44100.0;
    double lfoPhase = 0.0, gatePhase = 0.0;
    int modulationWritePosition = 0, delayWritePosition = 0;
    float smoothedGate = 1.0f;
    float smoothedGateRight = 0.0f;
    std::array<std::array<float, 2>, 2> gateFilterState {};
    std::array<std::array<bool, 128>, 16> heldNotes {};
    int heldNoteCount = 0;
    juce::SmoothedValue<float> noteEnvelope;
    juce::SmoothedValue<float> gateSwingSmoothing;
    std::array<juce::SmoothedValue<float>, 10> effectAmountSmoothing;
    std::atomic<float> gatePlaybackPhase { 0.0f };
    Pattern gatePattern { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MKNoiseRhythmMachineProcessor)
};
