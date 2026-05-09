#pragma once

#include "shared_includes.h"
#include "Cfg.h"
// #include "MTS-ESP/Client/libMTSClient.h"

class WavetableAudioProcessor;

class PhaseDistOscillator
{
public:
    PhaseDistOscillator() = default;

    struct Params
    {
        float leftGain = 1.0f;
        float rightGain = 1.0f;
        float position = 0.0f;
        float bend = 0.0f;
        float asym = 0.0f;
        float fold = 0.0f;
        int oversampling = 1;
    };

    void setSampleRate (double sr) 
    { 
        sampleRate = sr;
        blockerL.setSampleRate (float (sr));
        blockerR.setSampleRate (float (sr));
        blockerL.setCutoff (10.0f);
        blockerR.setCutoff (10.0f);
    }

    void noteOn (float p = -1) {
        p >= 0 ? phase = p : juce::Random::getSystemRandom().nextFloat();

        tableIndex = -1;
        lastTableIndex = -1;
    }

    template<typename T>
    T phaseDistortion (T phaseIn, float bend)
    {
        auto res = phaseDistorter->process(phaseIn, bend);
        jassert (gin::math::minVal (res) >= 0.0f && gin::math::maxVal (res) < 1.0f);
        return res;
    }

    void processGen (float note, const Params& params, juce::AudioSampleBuffer& buffer) {
        if (bllt == nullptr || bllt->size() == 0) return;

        float freq = float (std::min (sampleRate / 2.0, 440.0 * std::pow (2.0, (note - 69.0) / 12.0)));
        float delta = 1.0f / (float ((1.0f / freq) * sampleRate));

        int samps = buffer.getNumSamples();
        auto l = buffer.getWritePointer (0);
        auto r = buffer.getWritePointer (1);

        const float pos = float (bllt->size()) * params.position;
        const auto index1 = std::min (bllt->size() - 1, static_cast<int> (pos));
        const auto index2 = std::min (bllt->size() - 1, index1 + 1);
        
        const auto frac = pos - static_cast<float>(index1);
        
        auto table1 = bllt->getUnchecked (index1);
        auto table2 = bllt->getUnchecked (index2);

        const float oversampling = static_cast<float>(params.oversampling);

        while (samps > 0)
        {
            const float osDelta = delta / oversampling;
            float s = 0.0f;
            for (int os = 0; os < static_cast<int>(oversampling); os++) {

                auto s1 = table1->processLinear (note, phaseDistortion (std::min (almostOne, phase), params.bend));
                auto s2 = table2->processLinear (note, phaseDistortion (std::min (almostOne, phase), params.bend));

                s += s1 * (1.0f - frac) + s2 * frac;
                phase += osDelta;
            }
            while (phase >= 1.0f) {
                phase -= 1.0f;
            }

            s /= oversampling;
            s *= phaseDistorter->gain(phase, params.bend);

            *l++ += s * params.leftGain;
            *r++ += s * params.rightGain;
            samps--;
        }

    }

    void process (float note, const Params& params, juce::AudioSampleBuffer& buffer)
    {
        buffer.clear();
        processAdding (note, params, buffer);
    }

    void processAdding (float note, const Params& params, juce::AudioSampleBuffer& buffer)
    {
        processGen(note, params, buffer);

        if (blockDC)
        {
            int samps = buffer.getNumSamples();
            auto l = buffer.getWritePointer (0);
            auto r = buffer.getWritePointer (1);

            while (samps > 0)
            {
                *l -= blockerL.process (*l);
                *r -= blockerR.process (*r);

                l++;
                r++;
                samps--;
            }
        }
    }

    void setWavetable (gin::Wavetable* table) {
        bllt = table;
    }
    void setPhaseDistortion (InterpPhaseDistortion* phaseDist) {
        phaseDistorter = phaseDist;
    }

    void setBlockDC (bool b) { blockDC = b; }

    gin::Wavetable* bllt = nullptr;
    InterpPhaseDistortion* phaseDistorter = nullptr;

    double sampleRate = 44100.0;
    float phase = 0.0f;
    int tableIndex = 0;
    int lastTableIndex = -1;
    bool blockDC = true;

    gin::DCBlocker blockerL;
    gin::DCBlocker blockerR;

    static constexpr float almostOne = { 1.0f - std::numeric_limits<float>::epsilon() };

};

struct WTPhaseDistStereoOscillatorParams : public gin::VoicedOscillatorParams
{
    float position  = 0.5;
    float bend      = 0.0f;
    float asym      = 0.0f;
    float fold      = 0.0f;
    int oversampling = 1;

    inline void init (PhaseDistOscillator::Params& p) const
    {
        p.position = position;
        p.bend     = bend;
        p.asym     = asym;
        p.fold     = fold;
        p.oversampling = oversampling;
    }
};

class WTPhaseDistStereoOscillator : public gin::VoicedStereoOscillator<PhaseDistOscillator, WTPhaseDistStereoOscillatorParams>
{
public:
    WTPhaseDistStereoOscillator (int maxVoices = 8)
    {
        for (int i = 0; i < maxVoices; i++)
            oscillators.add (new PhaseDistOscillator());
    }

    void setWavetable (gin::Wavetable* table)
    {
        for (auto o : oscillators)
            o->setWavetable (table);
    }

    void setPhaseDistortion (InterpPhaseDistortion* PD)
    {
        for (auto o : oscillators)
            o->setPhaseDistortion (PD);
    }
};

//==============================================================================
class WavetableVoice : public gin::SynthesiserVoice,
                       public gin::ModVoice
{
public:
    WavetableVoice (WavetableAudioProcessor& p);
    
    ~WavetableVoice() override;
    
    void noteStarted() override;
    void noteRetriggered() override;
    void noteStopped (bool allowTailOff) override;

    void notePressureChanged() override;
    void noteTimbreChanged() override;
    void notePitchbendChanged() override;
    void noteKeyStateChanged() override     {}

	float getCurrentNote() override;

    void setCurrentSampleRate (double newRate) override;

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    bool isVoiceActive() override;

    float getFilterCutoffNormalized();
    PhaseDistOscillator::Params getLiveWTParams (int osc);

    void updateParams (int blockSize);

    WavetableAudioProcessor& proc;

    WTPhaseDistStereoOscillator oscillators[Cfg::numOSCs];

    gin::Filter filter;
    gin::ADSR filterADSR;
    
    gin::ADSR modADSRs[Cfg::numENVs];
    gin::LFO modLFOs[Cfg::numLFOs];
    gin::StepLFO modStepLFO;

    gin::AnalogADSR adsr;

    float currentMidiNotes[Cfg::numOSCs];
    WTPhaseDistStereoOscillatorParams oscParams[Cfg::numOSCs];
    
    gin::EasedValueSmoother<float> noteSmoother;
    
    float ampKeyTrack = 1.0f;    
};
