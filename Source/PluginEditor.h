#pragma once

#include <vector>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class XYPad : public juce::Component
{
public:
    explicit XYPad(juce::AudioProcessorValueTreeState& state);
    ~XYPad() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    void setScopeEnabled(bool enabled);
    void clearScope();
    void pushScopeSamples(const ScopeFrame* frames, int numFrames);
    juce::Rectangle<int> getScopeRepaintArea() const;

private:
    void setFromMouse(const juce::Point<float>& position);
    juce::Point<float> clampToCircle(const juce::Point<float>& position) const;
    void drawScope(juce::Graphics& g, const juce::Rectangle<float>& circleBounds, float radius);

    juce::AudioProcessorValueTreeState& apvts;
    juce::RangedAudioParameter* posXParam = nullptr;
    juce::RangedAudioParameter* posYParam = nullptr;

    std::unique_ptr<juce::ParameterAttachment> posXAttachment;
    std::unique_ptr<juce::ParameterAttachment> posYAttachment;

    float currentX = 0.0f; // -1 to +1
    float currentY = 0.0f; // -1 to +1

    bool scopeEnabled = true;
    struct ScopePoint
    {
        juce::Point<float> position;
        double timestamp = 0;
        size_t band = 0;
    };
    std::vector<ScopePoint> scopePoints;
    int scopeWriteIndex = 0;
    int scopePointCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XYPad)
};

class OrbitalPanAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit OrbitalPanAudioProcessorEditor(OrbitalPanAudioProcessor&);
    ~OrbitalPanAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    OrbitalPanAudioProcessor& audioProcessor;

    XYPad xyPad;
    juce::Label versionLabel;
    juce::Label buildLabel;

    juce::Slider depthSlider;
    juce::Slider mixSlider;
    juce::Slider widthSlider;

    juce::Label depthLabel;
    juce::Label mixLabel;
    juce::Label widthLabel;
    std::array<juce::Label, 3> bandLabels;

    juce::TextButton scopeToggle;
    juce::TextButton scopeSourceToggle;

    std::array<ScopeFrame, ScopeConstants::bufferSize> scopeReadBuffer;
    juce::TooltipWindow tooltips { this, 600 };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> depthAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;
    std::unique_ptr<SliderAttachment> widthAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrbitalPanAudioProcessorEditor)
};
