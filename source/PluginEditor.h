#pragma once

#include "shared_includes.h"
#include "PluginProcessor.h"
#include "Panels.h"
#include "Editor.h"

class MyLookAndFeel : public gin::GinLookAndFeel
{
    public:
    MyLookAndFeel()
      : gin::GinLookAndFeel()
    {
        setColour (GinLookAndFeel::colourId1, juce::Colours::green);
        setColour (GinLookAndFeel::colourId2, juce::Colours::red);
        setColour (GinLookAndFeel::colourId3, juce::Colours::blue);
        setColour (GinLookAndFeel::colourId4, juce::Colours::violet);
        setColour (GinLookAndFeel::colourId5, juce::Colours::orange);
    
        setColour (juce::Label::textColourId, defaultColour (4).withAlpha (0.9f));
    
        setColour (juce::Slider::trackColourId, defaultColour (4));
        setColour (juce::Slider::rotarySliderFillColourId, defaultColour (4));
    
        setColour (juce::TextButton::buttonColourId, defaultColour (0));
        setColour (juce::TextButton::buttonOnColourId, defaultColour (4));
        setColour (juce::TextButton::textColourOffId, defaultColour (4));
        setColour (juce::TextButton::textColourOnId, defaultColour (0));
    
        setColour (juce::ComboBox::backgroundColourId, juce::Colours::transparentWhite);
        setColour (juce::ComboBox::outlineColourId, defaultColour (4));
    
        setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentWhite);
    
        setColour (juce::TextEditor::backgroundColourId, juce::Colours::transparentWhite);
        setColour (juce::TextEditor::textColourId, defaultColour (4));
        setColour (juce::TextEditor::highlightColourId, defaultColour (4));
        setColour (juce::TextEditor::highlightedTextColourId, defaultColour (0));
        setColour (juce::TextEditor::outlineColourId, defaultColour (4));
        setColour (juce::TextEditor::focusedOutlineColourId, defaultColour (4));
        setColour (juce::TextEditor::shadowColourId, juce::Colours::transparentWhite);

    }

};

//==============================================================================
class WavetableAudioProcessorEditor : public gin::ProcessorEditor,
                                      public juce::DragAndDropContainer
{
public:
    WavetableAudioProcessorEditor (WavetableAudioProcessor&);
    ~WavetableAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void addMenuItems (juce::PopupMenu& m) override;

    void showAboutInfo() override;

private:
    WavetableAudioProcessor& wtProc;

    MyLookAndFeel lookAndFeel;

    gin::TriggeredScope scope { wtProc.scopeFifo };
    gin::SynthesiserUsage usage { wtProc };
    gin::ModulationOverview modOverview { wtProc.modMatrix };
    gin::ModOverlay modOverlay;

    Editor editor { wtProc };

    juce::MPEKeyboardComponent mpeKeyboard;

   #if JUCE_DEBUG
    std::unique_ptr<melatonin::Inspector> inspector;
   #endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavetableAudioProcessorEditor)
};
