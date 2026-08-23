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

    scopePoints.resize(static_cast<size_t>(ScopeConstants::bufferSize));
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

    drawScope(g, circleBounds, radius);

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

void XYPad::setScopeEnabled(bool enabled)
{
    if (scopeEnabled == enabled)
        return;

    scopeEnabled = enabled;
    if (!scopeEnabled)
        scopePointCount = 0;
}

void XYPad::pushScopeSamples(const float* left, const float* right, int numSamples)
{
    if (!scopeEnabled || scopePoints.empty() || left == nullptr || right == nullptr || numSamples <= 0)
        return;

    const int capacity = static_cast<int>(scopePoints.size());

    for (int i = 0; i < numSamples; ++i)
    {
        const float l = juce::jlimit(-1.0f, 1.0f, left[i]);
        const float r = juce::jlimit(-1.0f, 1.0f, right[i]);
        scopePoints[static_cast<size_t>(scopeWriteIndex)] = { l, r };
        scopeWriteIndex = (scopeWriteIndex + 1 == capacity) ? 0 : scopeWriteIndex + 1;
        scopePointCount = juce::jmin(scopePointCount + 1, capacity);
    }
}

juce::Rectangle<int> XYPad::getScopeRepaintArea() const
{
    auto bounds = getLocalBounds().reduced(10);
    const int diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
    const auto centre = bounds.getCentre();
    juce::Rectangle<int> circleBounds(
        centre.x - diameter / 2, centre.y - diameter / 2, diameter, diameter);
    return circleBounds.expanded(2);
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

void XYPad::drawScope(juce::Graphics& g, const juce::Rectangle<float>& circleBounds, float radius)
{
    if (!scopeEnabled || scopePointCount < 2 || scopePoints.empty())
        return;

    juce::Graphics::ScopedSaveState state(g);
    juce::Path clip;
    clip.addEllipse(circleBounds);
    g.reduceClipRegion(clip);

    const float scopeRadius = radius * 0.86f;
    const auto centre = circleBounds.getCentre();
    const int capacity = static_cast<int>(scopePoints.size());
    const int startIndex = (scopePointCount < capacity) ? 0 : scopeWriteIndex;
    const int count = scopePointCount;

    const auto baseColour = juce::Colours::deepskyblue;
    const float minAlpha = 0.04f;
    const float maxAlpha = 0.22f;

    for (int i = 0; i < count; ++i)
    {
        const int index = (startIndex + i >= capacity) ? (startIndex + i - capacity) : (startIndex + i);
        const auto& point = scopePoints[static_cast<size_t>(index)];

        const float t = (count > 1) ? static_cast<float>(i) / static_cast<float>(count - 1) : 1.0f;
        const float alpha = juce::jmap(t, minAlpha, maxAlpha);
        g.setColour(baseColour.withAlpha(alpha));

        const float x = centre.x + point.x * scopeRadius;
        const float y = centre.y - point.y * scopeRadius;
        g.fillEllipse(x - 1.2f, y - 1.2f, 2.4f, 2.4f);
    }
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

    scopeToggle.setButtonText("Scope");
    scopeToggle.setClickingTogglesState(true);
    scopeToggle.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(scopeToggle);

    scopeSourceToggle.setClickingTogglesState(true);
    scopeSourceToggle.setToggleState(false, juce::dontSendNotification);
    scopeSourceToggle.setButtonText("OUT");
    addAndMakeVisible(scopeSourceToggle);

    scopeToggle.onClick = [this]()
    {
        xyPad.setScopeEnabled(scopeToggle.getToggleState());
        xyPad.repaint(xyPad.getScopeRepaintArea());
    };

    scopeSourceToggle.onClick = [this]()
    {
        scopeSourceToggle.setButtonText(scopeSourceToggle.getToggleState() ? "IN" : "OUT");
    };

    depthAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "Depth", depthSlider);
    mixAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "Mix", mixSlider);

    scopeReadBuffer.setSize(2, ScopeConstants::bufferSize, false, false, true);

    startTimerHz(30);

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

    const auto padBounds = xyPad.getBounds();
    const int toggleWidth = 60;
    const int toggleHeight = 22;
    const int toggleMargin = 8;

    juce::Rectangle<int> toggleArea(
        padBounds.getRight() - toggleWidth - toggleMargin,
        padBounds.getY() + toggleMargin,
        toggleWidth,
        toggleHeight);

    scopeToggle.setBounds(toggleArea);
    scopeSourceToggle.setBounds(toggleArea.translated(0, toggleHeight + 6));

    const int controlWidth = controlsArea.getWidth() / 2;
    auto left = controlsArea.removeFromLeft(controlWidth);
    auto right = controlsArea;

    depthSlider.setBounds(left.reduced(10));
    mixSlider.setBounds(right.reduced(10));
}

void OrbitalPanAudioProcessorEditor::timerCallback()
{
    const bool scopeEnabled = scopeToggle.getToggleState();

    if (!scopeEnabled)
        return;

    auto& fifo = scopeSourceToggle.getToggleState()
        ? audioProcessor.getScopeInputBuffer()
        : audioProcessor.getScopeOutputBuffer();

    const int maxSamples = scopeReadBuffer.getNumSamples();
    const int pulled = fifo.pull(scopeReadBuffer.getWritePointer(0), scopeReadBuffer.getWritePointer(1), maxSamples);

    if (pulled > 0)
    {
        xyPad.pushScopeSamples(scopeReadBuffer.getReadPointer(0), scopeReadBuffer.getReadPointer(1), pulled);
        xyPad.repaint(xyPad.getScopeRepaintArea());
    }
}
