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
    void pushScopeSamples(const float* left, const float* right, int numSamples);
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
    std::vector<juce::Point<float>> scopePoints;
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

    juce::Slider depthSlider;
    juce::Slider mixSlider;

    juce::Label depthLabel;
    juce::Label mixLabel;

    juce::TextButton scopeToggle;
    juce::TextButton scopeSourceToggle;

    juce::AudioBuffer<float> scopeReadBuffer;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> depthAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrbitalPanAudioProcessorEditor)
};
