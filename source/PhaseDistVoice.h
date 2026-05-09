#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PhaseDistOscillator.h"
#include "PhaseDistLayer.h"
#include "SharedState.h"

#include "shared_includes.h"

class PhaseDistStereoOscillator
{
public:
    PhaseDistStereoOscillator (PhaseDistCurve& distCurve,
                               PhaseDistCurve& gainCurve,
                               juce::AudioSampleBuffer& waveTable)
      : distCurve(distCurve),
        gainCurve(gainCurve),
        waveTable(waveTable)
    {
    }

    struct Params
    {
        float leftGain = 1.0;
        float rightGain = 1.0;
        float morph = 0.0;
        float dist = 0.0;
        float fold = 0.0f;
        float asym = 0.0f;
    };

    void setSampleRate (double sr)  { sampleRate = sr; }
    void noteOn (float p = -1)
    {
        p >= 0 ? phase = p : juce::Random::getSystemRandom().nextFloat();
    }

    void process (float note, const Params& params, juce::AudioSampleBuffer& buffer)
    {
        buffer.clear();
        processAdding (note, params, buffer);
    }

    void processAdding (float note, const Params& params, juce::AudioSampleBuffer& buffer)
    {
        float freq = float (std::min (sampleRate / 2.0, 440.0 * std::pow (2.0, (note - 69.0) / 12.0)));
        float delta = 1.0f / (float ((1.0f / freq) * sampleRate));

        int samps = buffer.getNumSamples();
        auto l = buffer.getWritePointer (0);
        auto r = buffer.getWritePointer (1);

        for (int i = 0; i < samps; i++)
        {
            const auto phaseDist = distCurve.distort(phase, params.dist);
            jassert(phaseDist >= 0.0);
            jassert(phaseDist <= 1.0);
            auto s = interpMorph(phaseDist, params.morph, waveTable);
            s *=  gainCurve.distort(phase, params.dist);

            postProcess (params, s);

            *l++ += s * params.leftGain;
            *r++ += s * params.rightGain;

            phase += delta;
            while (phase >= 1.0f)
                phase -= 1.0f;
        }
    }

private:
    template<typename T>
    void postProcess (const Params& params, T& v)
    {
        if (params.asym > 0)
            v = gin::math::lerp (v, gin::math::pow4 (v - 1.0f) * -1.0f + 1.0f, gin::math::pow2 (params.asym));

        if (params.fold > 0)
        {
            const auto fold = gin::math::pow2 (gin::math::pow2 (1.0f - params.fold)) * 1.5f;
            v = (v - ((gin::math::max (v, fold) - fold) * T(2.0f)) - ((gin::math::min (v, -fold) + fold) * T(2.0f)));
        }
    }

    PhaseDistCurve& distCurve;
    PhaseDistCurve& gainCurve;
    juce::AudioSampleBuffer& waveTable;

    double sampleRate = 44100.0;
    float phase = 0.0f;
};

struct PhaseDistVoicedOscillatorParams : public gin::VoicedStereoOscillatorParams
{
    float morph = 0.0;
    float dist  = 0.0;
    float fold  = 0.0f;
    float asym  = 0.0f;

    inline void init (PhaseDistStereoOscillator::Params& p) const
    {
        p.morph = morph;
        p.dist  = dist;
        p.asym  = asym;
        p.fold  = fold;
    }
};

class PhaseDistVoicedStereoOscillator : public gin::VoicedStereoOscillator<PhaseDistStereoOscillator, PhaseDistVoicedOscillatorParams>
{
public:
    PhaseDistVoicedStereoOscillator (PhaseDistCurve& distCurve,
                                     PhaseDistCurve& gainCurve,
                                     juce::AudioSampleBuffer& waveTable,
                                     int maxVoices = 8)
    {
        for (int i = 0; i < maxVoices; i++)
            oscillators.add (new PhaseDistStereoOscillator (distCurve, gainCurve, waveTable));
    }
};




















class modLFO
{
    public:

    void reset() {
        phase = 0.0f;
    }

    void setSampleRate(double SR) {
        sampleRate = static_cast<float>(SR);
    }

    float nextValue(float frequency, float amplitude) {
        const float phaseIncrement = frequency / sampleRate;
        phase += phaseIncrement;
        while (phase > 1.0f) {
            phase -= 1.0f;
        }

        return phase * amplitude; // ramp...
    }

    private:

    float phase = 0.0f;
    float sampleRate = 44000.0f;
};


class PhaseDistVoice : public juce::MPESynthesiserVoice,  public juce::AudioProcessorParameter::Listener
{
public:
    PhaseDistVoice(SharedState& state);


    // MPE stuff
    void noteStarted() override;
    void noteStopped (bool allowTailOff) override;
    void notePressureChanged() override;
    void notePitchbendChanged() override;
    void noteTimbreChanged() override;
    void noteKeyStateChanged() override;

    void prepareToPlay (double sampleRate, int maxSamplesPerBlock, int outputChannels);
    void renderNextBlock (juce::AudioBuffer< float > &outputBuffer,
                         int startSample,
                         int numSamples) override;

    void updateSettings ();

    void parameterValueChanged(int parameterIndex, float newValue);
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting);

    void setUserInputs(const VoiceInputData& userInputs) {
        inputs = userInputs;
    }

    VoiceStatus getStatus();

    private:

    SharedState& gStateRef;

    float tmpPhase = 0.0f;

    // MPE stuff
    juce::SmoothedValue<float> level, timbre, frequency;

    float lastVelocity = 1.0;
    float lastPitchWheel = 0.0;

    PhaseDistLayer layerA, layerB;
    
    modLFO LFOs[3];
    juce::ADSR envs[kEnvCount];

    juce::dsp::LadderFilter<float> filter;

    VoiceStatus status;
    bool isPrepared { false };

    void stopVoice();
    float pitchWheelDetuneCents();

    VoiceInputData inputs;
    VoiceOutputData outputs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhaseDistVoice)
};
