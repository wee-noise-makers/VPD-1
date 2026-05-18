#pragma once

#include "../shared_includes.h"
#include "../WavetableVoice.h"

/** Draws a wavetable (copy of gin_wavetablecomponent.h but with a different oscillator...)
*/
class MyWavetableComponent : public juce::Component,
                             public juce::FileDragAndDropTarget
{
public:
    MyWavetableComponent();
    ~MyWavetableComponent() override;
    
    enum ColourIds
    {
        lineColourId             = 0x3331e10,
        backgroundColourId       = 0x3331e11,
        waveColourId             = 0x3331e12,
        activeWaveColourId       = 0x3331f13,
        phaseWaveColourId        = 0x3331f14,
    };

    void showPhase (float start, float len);
    void hidePhase();

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

    void enablementChanged() override;
    
    void setParams (PhaseDistOscillator::Params params);
    void setWavetables (gin::Wavetable*);
    void setPhaseDistortion (InterpPhaseDistortion*);

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray& files, int x, int y) override;
    void fileDragExit (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

    std::function<void (const juce::File&)> onFileDrop;
    
    enum Style
    {
        a,
        b,
    };
    
    void setStyle (Style s) { style = s; }

private:
    juce::Path createWavetablePath (float pos, float s = 0.0f, float e = 1.0f)
    {
        if (style == a) return createWavetablePathA (pos, s, e);
        if (style == b) return createWavetablePathB (pos, s, e);
        return {};
    }
    
    juce::Path createWavetablePathA (float pos, float s = 0.0f, float e = 1.0f);
    juce::Path createWavetablePathB (float pos, float s = 0.0f, float e = 1.0f);

    gin::Wavetable* bllt = nullptr;
    InterpPhaseDistortion* pd = nullptr;
    PhaseDistOscillator::Params params;
    juce::Array<juce::Path> paths;
    bool needsUpdate = false;
    bool dragOver = false;

    float phaseStart = -1;
    float phaseLen = -1;
    
    Style style = a;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MyWavetableComponent)
};
