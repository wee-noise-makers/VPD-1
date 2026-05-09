#pragma once

#include <vector>
#include <array>
#include <string>

#include "../shared_includes.h"

namespace GUI
{
    class MovablePointConstrainer : public juce::ComponentBoundsConstrainer 
    {
        public:

        MovablePointConstrainer(int id, 
                                bool start,
                                PhaseDistCurve& curve,
                                std::function<CurvePoint(juce::Point<int>)> guiToCurve,
                                std::function<juce::Point<int>(CurvePoint)> curveToGui) 
            : curve(curve),
              id(id),
              start(start),
              guiToCurve(guiToCurve),
              curveToGui(curveToGui)
        {
        }

        virtual void checkBounds (juce::Rectangle<int>& bounds,
                                  const juce::Rectangle<int>& previousBounds,
                                  const juce::Rectangle<int>& limits,
                                  bool isStretchingTop,
                                  bool isStretchingLeft,
                                  bool isStretchingBottom,
                                  bool isStretchingRight)
        {
            juce::ignoreUnused(isStretchingTop);
            juce::ignoreUnused(isStretchingLeft);
            juce::ignoreUnused(isStretchingBottom);
            juce::ignoreUnused(isStretchingRight);
            juce::ignoreUnused(limits);
            juce::ignoreUnused(previousBounds);

            const CurvePoint pt = guiToCurve(bounds.getCentre());
            const CurvePoint constrained = curve.constraint(id, start, pt);
            bounds.setCentre(curveToGui(constrained));
        }
        
        private:
        int id = 0;
        bool start = false;
        PhaseDistCurve& curve;
        std::function<CurvePoint(juce::Point<int>)> guiToCurve;
        std::function<juce::Point<int>(CurvePoint)> curveToGui;
    };

    class MovablePoint : public juce::Component
    {
        public:
        MovablePoint(int id,
                     bool start,
                     PhaseDistCurve& curve,
                     std::function<CurvePoint(juce::Point<int>)> guiToCurve,
                     std::function<juce::Point<int>(CurvePoint)> curveToGui,
                     std::function<void(int, bool)> onDragStart,
                     std::function<void(int, bool)> onDragEnd,
                     std::function<void(int, bool, int, int)> onDragMove) 
          : id(id),
            start(start),
            curve(curve),
            constrainer(id, start, curve, guiToCurve, curveToGui),
            guiToCurve(guiToCurve),
            onDragStart(onDragStart),
            onDragEnd(onDragEnd),
            onDragMove(onDragMove)
        {
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        }

        void paint(juce::Graphics& g) override{
            g.setColour(juce::Colours::whitesmoke);
            const auto bounds = getLocalBounds().reduced(1, 1);
            const auto TL = bounds.getTopLeft();
            const auto BR = bounds.getBottomRight();
            const auto TR = bounds.getTopRight();
            const auto BL = bounds.getBottomLeft();
            // g.drawLine(TL.getX(), TL.getY(), BR.getX(), BR.getY());
            // g.drawLine(TR.getX(), TR.getY(), BL.getX(), BL.getY());
            g.drawEllipse(bounds.toFloat(), 1.0f);
        }
        void mouseDown(const juce::MouseEvent& event) override {
            beginDragAutoRepeat(30);
            dragger.startDraggingComponent(this, event);
            onDragStart(id, start);
        }
        void mouseDrag(const juce::MouseEvent& event) override {
            dragger.dragComponent(this, event, &constrainer);
            dragged = true;
            curve.movePoint(id, start, guiToCurve(getBounds().getCentre()));

            onDragMove(id, start, getBounds().getCentreX(), getBounds().getCentreY());
            repaint();
        }
        void mouseUp(const juce::MouseEvent& event) override {
            dragged = false;
            onDragEnd(id, start);
            repaint();
        }

        private:
        int id = -1;
        bool start = false;
        bool dragged = false;
        PhaseDistCurve& curve;
        juce::ComponentDragger dragger;
        MovablePointConstrainer constrainer;
        std::function<CurvePoint(juce::Point<int>)> guiToCurve;

        std::function<void(int, bool)> onDragStart;
        std::function<void(int, bool)> onDragEnd;
        std::function<void(int, bool, int, int)> onDragMove;
    };

    class AddButton : public juce::TextButton {
        void paintButton (juce::Graphics& g, bool, bool) override {
            const auto bounds = getLocalBounds().reduced(1, 1);

            if (bounds.getWidth() <= 0) {
                return;
            }
            
            const auto TL = bounds.getTopLeft();
            const auto BR = bounds.getBottomRight();
            const auto TR = bounds.getTopRight();
            const auto BL = bounds.getBottomLeft();
            const auto CX = bounds.getCentreX();
            const auto CY = bounds.getCentreY();

            g.setColour(findColour(gin::GinLookAndFeel::backgroundColourId));
            g.fillRect(bounds);

            g.setColour(juce::Colours::whitesmoke);
            g.drawLine(TL.getX(), CY, BR.getX(), CY);
            g.drawLine(CX, TR.getY(), CX, BL.getY());
            g.drawRect(bounds, 1);
        }
    };

    class RemoveButton : public juce::TextButton {
        void paintButton (juce::Graphics& g, bool, bool) override {
            const auto bounds = getLocalBounds().reduced(1, 1);

            if (bounds.getWidth() <= 0) {
                return;
            }
            
            const auto TL = bounds.getTopLeft();
            const auto BR = bounds.getBottomRight();
            const auto TR = bounds.getTopRight();
            const auto BL = bounds.getBottomLeft();
            const auto CY = bounds.getCentreY();

            g.setColour(findColour(gin::GinLookAndFeel::backgroundColourId));
            g.fillRect(bounds);

            g.setColour(juce::Colours::whitesmoke);
            g.drawLine(TL.getX(), CY, BR.getX(), CY);
            g.drawRect(bounds, 1);
        }
    };
    
    class CurveEditor : public gin::MultiParamComponent, private juce::Timer
    {
        public:
        CurveEditor(PhaseDistCurve& curve,
                    juce::Viewport& viewport,
                    bool vertical = false)
            : curve(curve),
              viewport(viewport),
              vertical(vertical)
        {
            
            std::function<void(int, bool)> onDragStart = [this](int id, bool start) {onPointDragStart(id, start);};
            std::function<void(int, bool)> onDragEnd = [this](int id, bool start) {onPointDragEnd(id, start);};
            std::function<void(int, bool, int, int)> onDragMove = [this](int id, bool start, int x, int y) {onPointDragMove(id, start, x, y);};

            std::function<CurvePoint(juce::Point<int>)> guiToCurveCB = [this](juce::Point<int> pt) {
                return guiToCurve(pt);
            };
            std::function<juce::Point<int>(CurvePoint)> curveToGuiCB = [this](CurvePoint pt) {
                return curveToGui(pt);
            };
            
            for (int i = 0; i < 20; i++) {
                startPoints.push_back(std::make_unique<MovablePoint>(i,
                                                                     true,
                                                                     curve,
                                                                     guiToCurveCB,
                                                                     curveToGuiCB,
                                                                     onDragStart,
                                                                     onDragEnd,
                                                                     onDragMove));
                startPoints[i].get()->setEnabled(false);
                startPoints[i].get()->setVisible(false);
                addChildComponent(startPoints[i].get());

                endPoints.push_back(std::make_unique<MovablePoint>(i,
                                                                   false,
                                                                   curve,
                                                                   guiToCurveCB,
                                                                   curveToGuiCB,
                                                                   onDragStart,
                                                                   onDragEnd,
                                                                   onDragMove));
                endPoints[i].get()->setEnabled(false);
                endPoints[i].get()->setVisible(false);
                addChildComponent(endPoints[i].get());

                addPointButtons.add(std::make_unique<AddButton>());
                addPointButtons[i]->setEnabled(false);
                addPointButtons[i]->setVisible(false);
                addPointButtons[i]->onClick = [this, i]{
                    this->curve.addPoint(i + 1, this->params.bend);
                };
                addChildComponent(addPointButtons[i]);

                removePointButtons.add(std::make_unique<RemoveButton>());
                removePointButtons[i]->setEnabled(false);
                removePointButtons[i]->setVisible(false);
                removePointButtons[i]->onClick = [this, i]{
                    this->curve.removePoint(i);
                };
                addChildComponent(removePointButtons[i]);

            }

            repaint();
            startTimerHz (30);
        }
        
        void setWavetables (gin::Wavetable* bllt_)
        {
            bllt = bllt_;
        }

        void timerCallback() override {
            update();
            repaint();
        }

        void setParams (PhaseDistOscillator::Params params_) {
            params = params_;
        }

        int phaseDistOutRange() {
            return static_cast<int>(curve.getOutRange());
        }

        float minOut() {
            return curve.getMinOut();
        }

        float maxOut() {
            return curve.getMaxOut();
        }

        juce::Rectangle<int> curveBounds() {
            if (vertical) {
                //const auto width = proportionOfWidth(0.5f);
                
                // // auto bounds = getLocalBounds().removeFromRight(width);
                // // bounds.setHeight(width * phaseDistOutRange());
                // return getLocalBounds().withTrimmedLeft(width);
                
                //const auto width = getParentComponent()->getLocalBounds().proportionOfHeight(0.8f);

                const auto height = getLocalBounds().proportionOfHeight(0.8f);
                return getLocalBounds().withSizeKeepingCentre(height / phaseDistOutRange(),
                                                              height);
            } else {
                const auto height = getLocalBounds().proportionOfHeight(0.8F);
                return getLocalBounds().withSizeKeepingCentre(height * phaseDistOutRange(), height);
            }
        }

        juce::Point<int> curveToGui(CurvePoint pt) 
        {
            juce::Point<int> out;
            const auto bounds = curveBounds().toFloat();

            if (vertical) {
                float phase = juce::jmap<float>(pt.getX(), bounds.getBottomLeft().getX(), bounds.getTopRight().getX());

                const float range = maxOut() - minOut();
                const float Y = (pt.getY() - minOut()) / range; // value from 0.0 to 1.0
                const float dist = juce::jmap<float>(Y, bounds.getBottomLeft().getY(), bounds.getTopRight().getY());

                out.setX(static_cast<int>(phase));
                out.setY(static_cast<int>(dist));

            } else {

                float phase = juce::jmap<float>(pt.getX(), bounds.getBottomLeft().getY(), bounds.getTopRight().getY());
                const float range = maxOut() - minOut();
                const float Y = (pt.getY() - minOut()) / range; // value from 0.0 to 1.0
                const float dist = juce::jmap<float>(Y, bounds.getBottomLeft().getX(), bounds.getTopRight().getX());
                out.setX(static_cast<int>(dist));
                out.setY(static_cast<int>(phase));
            }

            return out;
        }

        CurvePoint guiToCurve(juce::Point<int> pt) {
            const auto bounds = curveBounds().toFloat();
            float dist, phase;
            if (vertical) {
                phase = ((static_cast<float>(pt.getX()) - bounds.getTopLeft().getX()) / bounds.getWidth());
                const int ptx = pt.getY();
                const float tly = bounds.getTopLeft().getY();
                const float height = bounds.getHeight();
                const float diff = static_cast<float>(ptx) - tly;
                dist = 1.0f - diff / height;
                dist = juce::jmap<float>(dist, minOut(), maxOut());

            } else {
                phase = 1.0f - ((static_cast<float>(pt.getY()) - bounds.getTopLeft().getY()) / bounds.getHeight());
                const int ptx = pt.getX();
                const float tlx = bounds.getTopLeft().getX();
                const float width = bounds.getWidth();
                const float diff = static_cast<float>(ptx) - tlx;
                dist = diff / width;
                dist = juce::jmap<float>(dist, minOut(), maxOut());
            }

            return CurvePoint(phase, dist);
        };

        void onPointDragStart(int id, bool start){
            //std::cout << "Drag start:" << id << std::endl;
            dragInProgress = true;
        }
        void onPointDragEnd(int id, bool start){
            //std::cout << "Drag end:" << id << std::endl;
            dragInProgress = false;
        }
        void onPointDragMove(int id, bool start, int x, int y){
            //std::cout << "Drag move:" << id << " " << x << " " << y <<std::endl;
            const auto vpx = viewport.getViewPositionX();
            const auto vpy = viewport.getViewPositionY();
            const auto dx = x - vpx;
            const auto dy = y - vpy;
            viewport.autoScroll(dx, dy, 10, 20);
        }

        void update(void) // override
        {
            updateHandlePos();
            updateHandleSizes(lastMousePos);
            curve.compile();
        }

        void updateHandlePos() {
            const int pointCount = curve.pointCount();
            const int last = curve.lastPointId();
            const int first = curve.firstPointId();
            for (int i = first; i < startPoints.size() - 1; i++) {
                const bool enable = i < pointCount;

                startPoints[i].get()->setVisible(enable);
                startPoints[i].get()->setEnabled(enable);
                endPoints[i].get()->setVisible(enable);
                endPoints[i].get()->setEnabled(enable);

                bool removeEnable = enable && i != first && i != last;

                if (enable) {
                    const auto range = curve[i];
                    const auto midPoint = range.start + (range.end - range.start) / 2.0f;

                    const auto SP = curveToGui(range.start);
                    const auto EP = curveToGui(range.end);
                    const auto MP = curveToGui(midPoint);
                    startPoints[i].get()->setCentrePosition(SP.toInt());
                    endPoints[i].get()->setCentrePosition(EP.toInt());
                    removePointButtons[i]->setCentrePosition(MP.toInt());

                    if (SP.getDistanceFrom(EP) < handleSize * 2.0f) {
                        removeEnable = false;
                    }

                }

                removePointButtons[i]->setEnabled(removeEnable);
                removePointButtons[i]->setVisible(removeEnable);
            }

            const float distAmount = params.bend;
            for (int i = 0; i < addPointButtons.size(); i++) {
                
                const bool enable = i < (pointCount - 1);

                if (enable) {
                    const auto A = curve[i];
                    const auto B = curve[i + 1];
                    const CurvePoint CA = interpRange(A, distAmount);
                    const CurvePoint CB = interpRange(B, distAmount);
                    const CurvePoint Mid = interpRange(MotionRange{CA, CB}, 0.5);

                    const auto Pt = curveToGui(Mid);
                    auto bounds = addPointButtons[i]->getBounds();
                    bounds.setCentre(Pt.getX(), Pt.getY());
                    addPointButtons[i]->setBounds(bounds);
                }

                addPointButtons[i]->setEnabled(enable);
                addPointButtons[i]->setVisible(enable);
            }
        }

        void updateHandleSizes(juce::Point<int> mousePos) {

            if (dragInProgress) {
                return;
            }

            if (vertical) {
                handleSize = curveBounds().proportionOfWidth(0.1f);
            } else {
                handleSize = curveBounds().proportionOfHeight(0.1f);
            }

            const auto fMousePos = mousePos.toFloat();
            juce::Component* closest = nullptr;
            float closestDist = 100000.0f;

            const auto height = static_cast<float>(getLocalBounds().getHeight());

            for (int i = 0; i < getNumChildComponents(); i++) {
                juce::Component* comp = getChildComponent(i);
                if (comp->isEnabled()) {
                    const auto center = comp->getBoundsInParent().getCentre().toFloat();
                    const auto dist = fMousePos.getDistanceFrom(center);
                    comp->setSize(handleSize / 1.5f, handleSize / 1.5f);
                    if (dynamic_cast<AddButton*>(comp) || dynamic_cast<RemoveButton*>(comp)) {
                        comp->setVisible(false);
                    }

                    comp->setCentrePosition(center.toInt());
                    if (dist < height && dist < closestDist) {
                        closest = comp;
                        closestDist = dist;
                    }
                } else {
                    comp->setSize(0, 0);
                }
            }

            if (closest != nullptr) {
                const auto center = closest->getBoundsInParent().getCentre();
                closest->setSize(handleSize, handleSize);
                closest->setCentrePosition(center);
                closest->setVisible(true);
            }
        }

        void mouseMove (const juce::MouseEvent& event) {
            // std::cout << "== mouseMove ==" << std::endl;
            // std::cout << "X: " << event.getPosition().getX() << " Y: " << event.getPosition().getY() << std::endl;
            lastMousePos = event.getPosition();
        }

        void paintWaveform(juce::Graphics& g) {
            if ( ! vertical && bllt != nullptr &&  bllt->size() > 0) {

                juce::Path wavePath; 
                bool start = true;

                const float pos = float (bllt->size()) * params.position;
                const auto index1 = std::min (bllt->size() - 1, static_cast<int> (pos));
                const auto index2 = std::min (bllt->size() - 1, index1 + 1);
                const auto frac = pos - static_cast<float>(index1);

                auto table1 = bllt->getUnchecked (index1);
                auto table2 = bllt->getUnchecked (index2);

                const float yOffset = 0;
                for (float Y = minOut(); Y <= maxOut(); Y += 0.01f) {

                    float phase = Y;
                    while (phase > 1.0f)
                        phase -= 1.0f;
                    while (phase < 0.0f)
                        phase += 1.0f;
                    
                    
                    const float s1 = table1->processLinear(40.0, phase);
                    const float s2 = table2->processLinear(40.0, phase);

                    float value = s1 * (1.0f - frac) + s2 * frac;

                    value = value + 0.5f;

                    const auto V = curveToGui(CurvePoint(value, Y));

                    if (start) {
                        start = false;
                        wavePath.startNewSubPath(V.getX(), V.getY() - yOffset);
                    } else {
                        wavePath.lineTo(V.getX(), V.getY() - yOffset);
                    }
                }
                g.setColour (findColour (gin::WavetableComponent::waveColourId, true).withMultipliedAlpha (isEnabled() ? 1.0f : 0.5f));
                g.strokePath(wavePath, juce::PathStrokeType(2.0));
            }
        }

        void paint(juce::Graphics& g) override {
            float dashPattern[2];
            dashPattern[0] = 8.0;
            dashPattern[1] = 8.0;

            float dotPattern[2];
            dotPattern[0] = 2.0;
            dotPattern[1] = 2.0;

            const auto bounds = getLocalBounds();

            g.setColour(juce::Colours::whitesmoke.withAlpha(0.4f));

            g.setFont (9.0f);

            const auto textHeight = vertical ?  curveBounds().getWidth() / 10.0f : curveBounds().getHeight() / 10.0f;
            const auto textWidth = textHeight * 2;
            //  Grid "vertical"
            for (float Y = minOut(); Y <= maxOut(); Y += 0.25f) {
                const auto S = curveToGui(CurvePoint(0.0, Y));
                const auto E = curveToGui(CurvePoint(1.0, Y));
                if (Y == 0.0f || Y == 1.0f) {
                    g.drawLine(juce::Line<float>(S.toFloat(), E.toFloat()));
                } else {
                    g.drawDashedLine(juce::Line<float>(S.toFloat(), E.toFloat()), dashPattern, 2, 1.0);
                }

                std::stringstream ss;
                ss << std::fixed << std::setprecision(2) << Y;
                if (vertical) {
                    juce::Rectangle<int> area (S.getX() - textWidth,
                                               S.getY() - textHeight * 0.5f,
                                               textWidth,
                                               textHeight);
                    g.drawFittedText(ss.str(), area, juce::Justification::centred, 1);
                } else {
                    juce::Rectangle<int> area (S.getX() - textWidth * 0.5f,
                                               S.getY() + textHeight * 0.2f,
                                               textWidth,
                                               textHeight);
                    g.drawFittedText(ss.str(), area, juce::Justification::centred, 1);
                }

            }
            //  Grid "horizontal"
            for (float X = 0.0; X <= 1.0; X += 0.25f) {
                const auto S = curveToGui(CurvePoint(X, maxOut()));
                const auto E = curveToGui(CurvePoint(X, minOut()));
                if (X == 0.0f || X == 1.0f) {
                    g.drawLine(juce::Line<float>(S.toFloat(), E.toFloat()));
                } else {
                    g.drawDashedLine(juce::Line<float>(S.toFloat(), E.toFloat()),
                                     dashPattern, 2, 1.0);
                }
            }

            paintWaveform(g);

            //  Curve
            const float distAmount = params.bend;
            juce::Path mainPath;                                 
            for (int i = curve.firstPointId(); i <= curve.lastPointId(); i++) {
                const auto range = curve[i];
                const CurvePoint current = interpRange(range, distAmount);

                const auto S = curveToGui(range.start);
                const auto E = curveToGui(range.end);
                const auto C = curveToGui(current);

                juce::ColourGradient grad (juce::Colours::green.darker(0.2), S.toFloat(),
                                           juce::Colours::green.brighter(0.5), E.toFloat(), 
                                           false);
                g.setGradientFill(grad);
                g.drawDashedLine(juce::Line<float>(S.toFloat(), E.toFloat()), dotPattern, 2, 2.0);

                if (i == 0) {
                    mainPath.startNewSubPath(C.getX(), C.getY());
                } else {
                    mainPath.lineTo(C.getX(), C.getY());
                }

            }
            g.setColour(juce::Colours::red);
            g.setColour (findColour (gin::WavetableComponent::activeWaveColourId, true).withMultipliedAlpha (isEnabled() ? 1.0f : 0.5f));

            g.strokePath(mainPath, juce::PathStrokeType(2.0));            
        }

        void resized() override {
            auto bounds = getLocalBounds();

            float pointSize;
            if (vertical) {
                pointSize = curveBounds().proportionOfWidth(0.7f);
            } else {
                pointSize = curveBounds().proportionOfHeight(0.7f);
            }

            for (int i = 0; i < getNumChildComponents(); i++) {
                getChildComponent(i)->setSize(pointSize, pointSize);
            }
        }
 
        private:

        PhaseDistOscillator::Params params;

        PhaseDistCurve& curve;
        juce::Viewport& viewport;
        gin::Wavetable* bllt = nullptr;


        bool vertical = false;

        bool dragInProgress = false;
        float handleSize = 0.0f;
        juce::Point<int> lastMousePos;

        std::vector<std::unique_ptr<MovablePoint>> startPoints;
        std::vector<std::unique_ptr<MovablePoint>> endPoints;
        juce::OwnedArray<juce::TextButton> addPointButtons;
        juce::OwnedArray<juce::TextButton> removePointButtons;
    };

    class PhaseDistShaper : public gin::MultiParamComponent
    {
        public:
        PhaseDistShaper(PhaseDistCurve& distCurve,
                        PhaseDistCurve& gainCurve)
            : tabs(juce::TabbedButtonBar::Orientation::TabsAtRight)
            , phaseDistCurveEdit(distCurve, viewport, false)
            , gainCurveEdit(gainCurve, gainViewport, true)
              //, windowPhaseDistCurveEdit(distCurve, windowViewport, false)

        {
            setName ("shaper");

            addAndMakeVisible(tabs);

            viewport.setViewedComponent(&phaseDistCurveEdit, false);
            gainViewport.setViewedComponent(&gainCurveEdit, false);

            // addAndMakeVisible(viewport);
            //setTabBarDepth(200);
            tabs.addTab("Distortion", juce::Colours::transparentBlack, &viewport, false);
            tabs.addTab("Gain", juce::Colours::transparentBlack, &gainViewport, false);

            // openWindow();

            tabs.setOutline(0);
        }

        void setWavetables (gin::Wavetable* bllt_) {
            phaseDistCurveEdit.setWavetables(bllt_);
        }

        void setParams (PhaseDistOscillator::Params params_) {
            phaseDistCurveEdit.setParams(params_);
            gainCurveEdit.setParams(params_);
        }

        void paint(juce::Graphics& g) override {
           //g.fillAll (getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
        }

        void resized() override {
            auto bounds = getLocalBounds();
            auto viewportBounds = bounds;

            tabs.setBounds(bounds);

            viewportBounds.removeFromRight(tabs.getTabBarDepth());
            viewport.setBounds(viewportBounds);
            gainViewport.setBounds(viewportBounds);

            const auto height = viewportBounds.proportionOfHeight(0.90f);
            const auto width = viewportBounds.proportionOfWidth(0.95f);
            const auto distCurveBounds = viewportBounds.withSizeKeepingCentre(width * phaseDistCurveEdit.phaseDistOutRange() / 2.0f,
                                                                              height);
            phaseDistCurveEdit.setBounds(distCurveBounds);

            const auto gainCurveBounds = viewportBounds.withSizeKeepingCentre(viewportBounds.proportionOfWidth(0.98f), height);
            gainCurveEdit.setBounds(gainCurveBounds);
        }
 
        // void openWindow() {
        //     if (window)
        //         window->toFront(true);
        //     else
        //     {
        //         window = new juce::DocumentWindow("test", 
        //             juce::Colours::red, juce::DocumentWindow::allButtons, true);
        //         window->centreWithSize (600, 400);
        //         window->addAndMakeVisible(windowViewport);
        //         windowViewport.setBounds(window->getBounds());

        //         window->setVisible (true);
        //     }
        // }

        private:

        juce::Viewport viewport;
        CurveEditor phaseDistCurveEdit;

        CurveEditor gainCurveEdit;
        juce::Viewport gainViewport;

        juce::TabbedComponent tabs;

        // juce::Viewport windowViewport;
        // CurveEditor windowPhaseDistCurveEdit;
        // juce::Component::SafePointer<juce::TopLevelWindow> window;

    };
}