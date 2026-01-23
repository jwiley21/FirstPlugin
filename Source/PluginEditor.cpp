#include "PluginEditor.h"

XYPad::XYPad(juce::AudioProcessorValueTreeState& state)
    : apvts(state)
{
    posXParam = apvts.getParameter("PosX");
    posYParam = apvts.getParameter("PosY");

    posXAttachment = std::make_unique<juce::ParameterAttachment>(
        *posXParam,
        [this](float newValue)
        {
            currentX = newValue;
            repaint();
        },
        nullptr);

    posYAttachment = std::make_unique<juce::ParameterAttachment>(
        *posYParam,
        [this](float newValue)
        {
            currentY = newValue;
            repaint();
        },
        nullptr);

    posXAttachment->sendInitialUpdate();
    posYAttachment->sendInitialUpdate();
}

XYPad::~XYPad() = default;

void XYPad::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(10.0f);
    const float diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
    const auto centre = bounds.getCentre();
    const float radius = diameter * 0.5f;

    juce::Rectangle<float> circleBounds(
        centre.x - radius, centre.y - radius, diameter, diameter);

    g.fillAll(juce::Colours::black);

    g.setColour(juce::Colours::darkgrey);
    g.fillEllipse(circleBounds);

    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.drawEllipse(circleBounds, 2.0f);

    g.setColour(juce::Colours::white.withAlpha(0.15f));
    g.drawLine(centre.x - radius, centre.y, centre.x + radius, centre.y, 1.0f);
    g.drawLine(centre.x, centre.y - radius, centre.x, centre.y + radius, 1.0f);

    const float dotX = centre.x + currentX * radius;
    const float dotY = centre.y - currentY * radius;

    g.setColour(juce::Colours::orange);
    g.fillEllipse(dotX - 6.0f, dotY - 6.0f, 12.0f, 12.0f);
}

void XYPad::resized()
{
}

void XYPad::mouseDown(const juce::MouseEvent& event)
{
    if (posXParam != nullptr && posYParam != nullptr)
    {
        posXParam->beginChangeGesture();
        posYParam->beginChangeGesture();
    }
    setFromMouse(event.position);
}

void XYPad::mouseDrag(const juce::MouseEvent& event)
{
    setFromMouse(event.position);
}

void XYPad::mouseUp(const juce::MouseEvent& event)
{
    juce::ignoreUnused(event);
    if (posXParam != nullptr && posYParam != nullptr)
    {
        posXParam->endChangeGesture();
        posYParam->endChangeGesture();
    }
}

void XYPad::setFromMouse(const juce::Point<float>& position)
{
    if (posXParam == nullptr || posYParam == nullptr)
        return;

    const auto clamped = clampToCircle(position);
    const auto bounds = getLocalBounds().toFloat().reduced(10.0f);
    const float diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
    const float radius = diameter * 0.5f;
    const auto centre = bounds.getCentre();

    const float normX = (clamped.x - centre.x) / radius;
    const float normY = (centre.y - clamped.y) / radius;

    currentX = juce::jlimit(-1.0f, 1.0f, normX);
    currentY = juce::jlimit(-1.0f, 1.0f, normY);

    posXParam->setValueNotifyingHost(posXParam->convertTo0to1(currentX));
    posYParam->setValueNotifyingHost(posYParam->convertTo0to1(currentY));

    repaint();
}

juce::Point<float> XYPad::clampToCircle(const juce::Point<float>& position) const
{
    const auto bounds = getLocalBounds().toFloat().reduced(10.0f);
    const float diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
    const float radius = diameter * 0.5f;
    const auto centre = bounds.getCentre();

    auto offset = position - centre;
    const float distance = offset.getDistanceFromOrigin();

    if (distance > radius)
    {
        offset = offset / distance * radius;
        return centre + offset;
    }

    return position;
}

OrbitalPanAudioProcessorEditor::OrbitalPanAudioProcessorEditor(OrbitalPanAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), xyPad(p.apvts)
{
    addAndMakeVisible(xyPad);

    depthSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    depthSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(depthSlider);

    mixSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(mixSlider);

    depthLabel.setText("Depth", juce::dontSendNotification);
    depthLabel.setJustificationType(juce::Justification::centred);
    depthLabel.attachToComponent(&depthSlider, false);
    addAndMakeVisible(depthLabel);

    mixLabel.setText("Mix", juce::dontSendNotification);
    mixLabel.setJustificationType(juce::Justification::centred);
    mixLabel.attachToComponent(&mixSlider, false);
    addAndMakeVisible(mixLabel);

    depthAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "Depth", depthSlider);
    mixAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "Mix", mixSlider);

    setSize(420, 500);
}

OrbitalPanAudioProcessorEditor::~OrbitalPanAudioProcessorEditor() = default;

void OrbitalPanAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void OrbitalPanAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(12);

    auto controlsArea = bounds.removeFromBottom(140);
    xyPad.setBounds(bounds);

    const int controlWidth = controlsArea.getWidth() / 2;
    auto left = controlsArea.removeFromLeft(controlWidth);
    auto right = controlsArea;

    depthSlider.setBounds(left.reduced(10));
    mixSlider.setBounds(right.reduced(10));
}
