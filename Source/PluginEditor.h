#pragma once

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

private:
    void setFromMouse(const juce::Point<float>& position);
    juce::Point<float> clampToCircle(const juce::Point<float>& position) const;

    juce::AudioProcessorValueTreeState& apvts;
    juce::RangedAudioParameter* posXParam = nullptr;
    juce::RangedAudioParameter* posYParam = nullptr;

    std::unique_ptr<juce::ParameterAttachment> posXAttachment;
    std::unique_ptr<juce::ParameterAttachment> posYAttachment;

    float currentX = 0.0f; // -1 to +1
    float currentY = 0.0f; // -1 to +1

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XYPad)
};

class OrbitalPanAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OrbitalPanAudioProcessorEditor(OrbitalPanAudioProcessor&);
    ~OrbitalPanAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    OrbitalPanAudioProcessor& audioProcessor;

    XYPad xyPad;

    juce::Slider depthSlider;
    juce::Slider mixSlider;

    juce::Label depthLabel;
    juce::Label mixLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> depthAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrbitalPanAudioProcessorEditor)
};
