#include "my_wavetablecomponent.h"

MyWavetableComponent::MyWavetableComponent()
{
}

MyWavetableComponent::~MyWavetableComponent()
{
}
//==============================================================================

void MyWavetableComponent::showPhase (float start, float len)
{
    phaseStart = start;
    phaseLen = len;
    repaint();
}

void MyWavetableComponent::hidePhase()
{
    phaseStart = -1;
    phaseLen = -1;
    repaint();
}

void MyWavetableComponent::setParams (PhaseDistOscillator::Params params_)
{
    if (! juce::approximatelyEqual (params.bend, params_.bend)       ||
        ! juce::approximatelyEqual (params.asym, params_.asym)       ||
        ! juce::approximatelyEqual (params.fold, params_.fold) )
    {
        params = params_;
        needsUpdate = true;
        repaint();
    }
    else if (! juce::approximatelyEqual (params.position, params_.position))
    {
        params = params_;
        repaint();
    }
}

void MyWavetableComponent::resized()
{
    needsUpdate = true;
    repaint();
}

void MyWavetableComponent::enablementChanged() {
    needsUpdate = true;
    repaint();
}

void MyWavetableComponent::setWavetables (gin::Wavetable* bllt_)
{
    bllt = bllt_;
    needsUpdate = true;
    repaint();
}

void MyWavetableComponent::setPhaseDistortion (InterpPhaseDistortion* pd_)
{
    pd = pd_;
    needsUpdate = true;
    repaint();
}

void MyWavetableComponent::paint (juce::Graphics& g)
{
    if (needsUpdate && bllt)
    {
        needsUpdate = false;

        paths.clear();
        auto numTables = std::min (32, bllt->size());
        for (auto i = 0; i < numTables; i++)
            paths.add (createWavetablePath (float (i) / numTables));
    }

    if (dragOver)
    {
        g.setColour (findColour (activeWaveColourId, true).withAlpha (0.3f));
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 5.0f);
    }

    if (paths.size() > 0)
    {
        g.setColour (findColour (waveColourId, true).withMultipliedAlpha (isEnabled() ? 1.0f : 0.5f));
        for (auto& p : paths)
            g.strokePath (p, juce::PathStrokeType (0.75f));

        if (isEnabled())
        {
            g.setColour (findColour (activeWaveColourId, true).withMultipliedAlpha (isEnabled() ? 1.0f : 0.5f));
            g.strokePath (createWavetablePath (params.position), juce::PathStrokeType (0.75f));

            if (phaseStart >= 0)
            {
                auto s1 = phaseStart;
                auto e1 = phaseStart + phaseLen;

                juce::Point<float> pdot;

                g.setColour (findColour (phaseWaveColourId, true));
                if (e1 > 1.0)
                {
                    auto s2 = 0.0f;
                    auto e2 = phaseLen - (1.0f - s1);

                    e1 = 1.0f;

                    auto p1 = createWavetablePath (params.position, s1, e1);
                    auto p2 = createWavetablePath (params.position, s2, e2);

                    g.strokePath (p1, juce::PathStrokeType (0.75f));
                    g.strokePath (p2, juce::PathStrokeType (0.75f));

                    pdot = p1.getPointAlongPath (0.0f);
                }
                else
                {
                    auto p1 = createWavetablePath (params.position, s1, e1);

                    g.strokePath (p1, juce::PathStrokeType (0.75f));

                    pdot = p1.getPointAlongPath (0.0f);
                }

                g.fillEllipse (pdot.x - 2.0f, pdot.y - 2.0f, 4.0f, 4.0f);
            }
        }
    }
}

juce::Path MyWavetableComponent::createWavetablePathB (float wtPos, float start, float end)
{
    constexpr auto samples = 64;

    juce::AudioSampleBuffer buf (2, samples);

    {
        PhaseDistOscillator osc;

        auto hz = 44100.0f / samples;
        auto note = gin::getMidiNoteFromHertz (hz);
        auto p = params;

        p.position = wtPos;
        p.bend = isEnabled() ? params.bend : 0.f;

        osc.setBlockDC (false);
        osc.setSampleRate (44100.0);
        osc.setWavetable (bllt);
        osc.setPhaseDistortion(pd);
        osc.noteOn (0.0f);
        osc.process (note, p, buf);
    }

    juce::Path p;

    auto w = float (getWidth())  * 0.8f;
    auto h = float (getHeight()) * 0.35f;

    auto x = 10 + (getWidth() - 20 - w) * wtPos;
    auto y = getHeight() / 2.0f + (-(wtPos - 0.5f)) * h;

    auto data = buf.getReadPointer (0);
    auto offset = juce::roundToInt (start * samples);

    p.startNewSubPath (x + w * float (offset) / samples, y + -data[offset] * h);

    for (auto s = 1 + offset; s < std::min (samples, juce::roundToInt (samples * end) + 1); s++)
    {
        p.lineTo (x + w * float (s) / samples, y + -data[s] * h);
    }

    return p;
}

juce::Path MyWavetableComponent::createWavetablePathA (float wtPos, float start, float end)
{
    constexpr auto samples = 64;

    juce::AudioSampleBuffer buf (2, samples);

    {
        PhaseDistOscillator osc;

        auto hz = 44100.0f / samples;
        auto note = gin::getMidiNoteFromHertz (hz);
        auto p = params;

        p.bend = isEnabled() ? params.bend : 0.f;
        p.position = wtPos;

        osc.setSampleRate (44100.0);
        osc.setWavetable (bllt);
        osc.setPhaseDistortion(pd);
        osc.noteOn (0.0f);
        osc.process (note, p, buf);
    }

    juce::Path p;

    auto w = float (getWidth());
    auto h = float (getHeight());

    auto data = buf.getReadPointer (0);

    auto xSpread = 0.4f;
    auto yScale = -(1.0f / 4.5f);
    auto dx = std::min (w, h);

    auto xSlope = (dx * 1.5f * (1.0f - xSpread)) / float (samples);
    auto ySlope = xSlope / 4.0f;

    auto xOffset = (dx * xSpread) * wtPos;
    auto yOffset = (dx * xSpread) - xOffset;

    xOffset += (w - dx * xSpread - samples * xSlope) / 2.0f;
    yOffset += (h - dx * xSpread - samples * ySlope) / 2.0f;

    auto offset = juce::roundToInt (start * samples);
    xOffset += xSlope * offset;
    yOffset += ySlope * offset;

    p.startNewSubPath (xOffset, data[offset] * yScale * dx + yOffset);

    for (auto s = 1 + offset; s < std::min (samples, juce::roundToInt (samples * end) + 1); s++)
    {
        p.lineTo (xOffset, data[s] * yScale * dx + yOffset);

        xOffset += xSlope;
        yOffset += ySlope;
    }

    return p;
}

bool MyWavetableComponent::isInterestedInFileDrag (const juce::StringArray& files)
{
    if (onFileDrop && files.size() == 1 && juce::File (files[0]).hasFileExtension (".wav"))
        return true;

    return false;
}

void MyWavetableComponent::fileDragEnter (const juce::StringArray&, int, int)
{
    dragOver = true;
    repaint();
}

void MyWavetableComponent::fileDragExit (const juce::StringArray&)
{
    dragOver = false;
    repaint();
}

void MyWavetableComponent::filesDropped (const juce::StringArray& files, int, int)
{
    dragOver = false;
    repaint();

    onFileDrop (juce::File (files[0]));
}

