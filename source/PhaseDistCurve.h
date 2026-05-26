#pragma once

#include <iostream>
#include <cmath>
#include "shared_includes.h"

using CurvePoint = juce::Point<float>;

struct MotionRange {
    juce::Point<float> start;
    juce::Point<float> end;
};

inline CurvePoint interpRange(const MotionRange& mr, float amount) {
    const float deltX = mr.end.getX() - mr.start.getX(); 
    const float deltY = mr.end.getY() - mr.start.getY();
    return CurvePoint(mr.start.getX() + deltX * amount, 
                      mr.start.getY() + deltY * amount); 
}

inline float interpCurve(const std::vector<CurvePoint>& curve, float phase) {
    jassert(curve.size() >= 2);
    for (int i = 1; i < curve.size(); i++) {
        const auto A = curve[i - 1];
        const auto B = curve[i];
        if (A.getX() <= phase && phase <= B.getX()) {
            const float AY = A.getY();
            const float AX = A.getX();
            const float BY = B.getY();
            const float BX = B.getX();
            if (AX == BX) {
                return AY;
            } else {
                const float result = AY + ((phase - AX) / (BX - AX))  * (BY - AY);
                jassert( ! std::isnan(result));
                return result;
            }
        }
    }
    jassert(false); // We should always find a point from the curve
    return 0.0;
}

class PhaseDistCurve {
    public:
    PhaseDistCurve(float minOut, float maxOut, bool setupGain = false)
      : minOut(minOut),
        maxOut(maxOut),
        isGain(setupGain)
    {
        reset();
    }

    void reset() {
        points.clear();
        if (isGain) {
            points.push_back({CurvePoint(0.0f, 1.0f), CurvePoint(0.0f, 1.0f)});
            points.push_back({CurvePoint(1.0f, 1.0f), CurvePoint(1.0f, 1.0f)});
        } else {
            points.push_back({CurvePoint(0.0f, 0.0f), CurvePoint(0.0f, 0.0f)});
            points.push_back({CurvePoint(1.0f, 1.0f), CurvePoint(1.0f, 1.0f)});
        }

        for (auto& buf: compiled) {
            buf.setSize(1, 256);
        }
        dirty = true;
        compile();
    }
    
    int pointCount() {
        return points.size();
    }

    int firstPointId() {
        return 0;
    }
    int lastPointId() {
        return points.size() - 1;
    };

    float getMinOut() {
        return minOut;
    }
    float getMaxOut() {
        return maxOut;
    }
    float getOutRange() {
        return maxOut - minOut;
    }

    CurvePoint constraint(int id, bool start, CurvePoint target) {
        float X = target.getX();
        float Y = target.getY();
        
        if (id == firstPointId()) {
            if (X != 0.0) {
                //  Keep the X value to the left
               X = 0.0f;
            }
        } else if (id == lastPointId()) {
            if (X != 1.0) {
                //  Keep the X value right
                X = 1.0f;
            }
        } else {
            const MotionRange prev = points[id - 1];
            const MotionRange next = points[id + 1];
            CurvePoint left, right;
            if (start) {
                left = prev.start;
                right = next.start;
            } else {
                left = prev.end;
                right = next.end;
            }       
            X = std::max(X, left.getX());
            X = std::min(X, right.getX());
        }
          
        Y = std::max(Y, minOut);
        Y = std::min(Y, maxOut);
        return CurvePoint(X, Y);
    }

    CurvePoint movePoint(int id, bool start, CurvePoint target) {
        jassert(id >= firstPointId() && id <= lastPointId());

        const auto constrained = constraint(id, start, target);
        if (start) {
            points[id].start = constrained;
        } else {
            points[id].end = constrained;
        }

        if (isGain) {
            juce::AccessibilityHandler::postAnnouncement
              (std::format("gain = {:.2f} at position {:.2f}", constrained.getX(), constrained.getY()),
               juce::AccessibilityHandler::AnnouncementPriority::medium);

        } else {
            juce::AccessibilityHandler::postAnnouncement
              (std::format("phase output {:.2f} at position {:.2f}", constrained.getY(), constrained.getX()),
               juce::AccessibilityHandler::AnnouncementPriority::medium);
        }

        dirty = true;
        return constrained;
    }

    CurvePoint moveRelative(int id, bool start, float x, float y) {
        jassert(id >= firstPointId() && id <= lastPointId());
    
        CurvePoint target;
        if (start) {
            target = points[id].start;
        } else {
            target = points[id].end;
        }
        
        target.setX (target.getX() + x);
        target.setY (target.getY() + y);
        return movePoint(id, start, target);
    }

    inline static const float moveStep_ = 0.05f;

    CurvePoint moveUp(int id, bool start) {
        if (isGain) {
            return moveRelative(id, start, 0.f, moveStep_);
        } else {
            return moveRelative(id, start, moveStep_, 0.f);
        }
    }
    CurvePoint moveDown(int id, bool start) {
        if (isGain) {
            return moveRelative(id, start, 0.f, -moveStep_);
        } else {
            return moveRelative(id, start, -moveStep_, 0.f);
        }
    }
    CurvePoint moveLeft(int id, bool start) {
        if (isGain) {
            return moveRelative(id, start, -moveStep_, 0.f);
        } else {
            return moveRelative(id, start, 0.f, -moveStep_);
        }
    }
    CurvePoint moveRight(int id, bool start) {
        if (isGain) {
            return moveRelative(id, start, moveStep_, 0.f);
        } else {
            return moveRelative(id, start, 0.f, moveStep_);
        }
    }
    
    void addPoint(int beforeId, float distAmount = 0.5f) {

        if (beforeId <= firstPointId() || beforeId > lastPointId()) {
            return;
        }

        const MotionRange beforeRange = points[beforeId - 1];
        const MotionRange afterRange = points[beforeId];
        const CurvePoint before = interpRange(beforeRange, distAmount);
        const CurvePoint after  = interpRange(afterRange, distAmount);

        const auto midPoint = before + (after - before) / 2.0f;

        points.insert(points.begin() + beforeId, MotionRange{midPoint, midPoint});
        dirty = true;
    }

    void removePoint(int id) {
        if (id < firstPointId() || id > lastPointId()) {
            return;
        }
        points.erase(points.begin() + id);
        dirty = true;
    }

    MotionRange operator [](int id) {
        jassert(id >= firstPointId() && id <= lastPointId());
        return points[id];
    }

    void compile(){
        if ( ! dirty ) {
            return;
        }

        const float distAmountDepth = static_cast<float>(compiled.size() - 1);

        for (int i = 0; i < compiled.size(); i++) {
            const float distAmount = static_cast<float>(i) / distAmountDepth;
            const auto curve = at(distAmount);

            const int size = compiled[i].getNumSamples();
            const float phaseDepth = static_cast<float>(size - 1);
            for (int j = 0; j < size; j++) {
                const float phase = static_cast<float>(j) / phaseDepth;
                const float val =  interpCurve(curve, phase);
                jassert( ! std::isnan(val));
                compiled[i].setSample(0, j, val);
            }
        }
        dirty = false;
    }

    float interpSingle(float phase, const juce::AudioSampleBuffer& waveForm) const
    {
        jassert(phase < 1.0f);
        jassert(phase >= 0.0f);
        const int size = waveForm.getNumSamples();
        const float findex = phase * static_cast<float>(size - 1);
        const int a_index = static_cast<int>(findex);
        const int b_index = a_index >= (size - 1) ? 0 : a_index + 1;

        const float a = waveForm.getSample(0, a_index);
        const float b = waveForm.getSample(0, b_index);

        const float delt = findex - static_cast<float>(a_index);
        return a + delt * (b - a); 
    }

    float distort(float phase, float distAmount) const {
        const std::size_t size = compiled.size();
        const float findex = distAmount * static_cast<float>(size - 1);
        const std::size_t a_index = static_cast<std::size_t>(findex);
        const std::size_t b_index = a_index + 1;

        float output;
        if (a_index == size - 1) {
            const auto a = interpSingle(phase, compiled[a_index]);
            output = a;
        } else {
            const auto a = interpSingle(phase, compiled[a_index]);
            const auto b = interpSingle(phase, compiled[b_index]);
            const float delt = findex - static_cast<float>(a_index);

            output = a + delt * (b - a); 
        }

        while (output > 1.0)
            output -= 1.0;
        while (output < 0.0)
            output += 1.0;

        return output;
    }

    juce::String toXml() const {
        auto xml = juce::XmlElement("pdcurve");

        for (auto& range : points) {
            auto *rangeXml = new juce::XmlElement("range");
            rangeXml->setAttribute(juce::Identifier("sx"), range.start.getX());
            rangeXml->setAttribute(juce::Identifier("sy"), range.start.getY());
            rangeXml->setAttribute(juce::Identifier("ex"), range.end.getX());
            rangeXml->setAttribute(juce::Identifier("ey"), range.end.getY());
            xml.addChildElement(rangeXml);
        }
        // std::cout << "toXml:" << xml.toString() << std::endl;
        return xml.toString();
    }

    juce::String toBase64Encoding() const {
        return juce::Base64::toBase64(toXml());
    }

    void fromXml(juce::String xmlStr) {

        // std::cout << "fromXml: " << xmlStr << std::endl;
        auto xml = juce::XmlDocument::parse(xmlStr);

        if (xml != nullptr && xml->getTagName() == "pdcurve") {
            points.clear();
            for (auto* element : xml->getChildIterator()) {
               if(element->getTagName() == "range") {
                   MotionRange range;
                   range.start.setX(element->getDoubleAttribute("sx"));
                   range.start.setY(element->getDoubleAttribute("sy"));
                   range.end.setX(element->getDoubleAttribute("ex"));
                   range.end.setY(element->getDoubleAttribute("ey"));
                   points.push_back(range);
               }
            }
            dirty = true;
            compile();
        } else {
            reset();
        }
    }

    void fromBase64Encoding(juce::String encoding) {
        juce::MemoryOutputStream mo;
        if (juce::Base64::convertFromBase64(mo, encoding)) {
            fromXml(mo.toUTF8());
        }
    }
    private:

    std::vector<CurvePoint> at(float distAmout) {
        std::vector<CurvePoint> res;

        for (int i = firstPointId(); i <= lastPointId(); i++) {
            res.push_back(interpRange(points[i], distAmout));
        }
        return res;
    }

    bool isGain = false;
    bool dirty = true;
    float minOut = 0.0f;
    float maxOut = 1.0f;
    std::array<juce::AudioSampleBuffer, 256> compiled;
    std::vector<MotionRange> points;
};

class InterpPhaseDistortion
{
    public:

    InterpPhaseDistortion(PhaseDistCurve& distCurve_,
                          PhaseDistCurve& gainCurve_)
      : distCurve(distCurve_),
        gainCurve(gainCurve_)
    {

    }

    inline float process (float phase, float distortion)
    {
        phase = distCurve.distort(phase, distortion);

        while (phase >= 1.0f)
            phase -= 1.0f;
        while (phase < 0.0f)
            phase += 1.0f;

        return phase;
    }

    inline float gain(float phase, float distortion) {
        return gainCurve.distort(phase, distortion);
    }

    private:
    
    // TODO: Should use gin::Wavetables here?
    PhaseDistCurve& distCurve;
    PhaseDistCurve& gainCurve;
};