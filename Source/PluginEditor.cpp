#include "PluginEditor.h"

namespace
{
const std::array<juce::Colour, 3> bandColours {
    juce::Colour(0xff39a9ff), juce::Colour(0xff43df88), juce::Colour(0xffff5b65)
};
}

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

    scopePoints.resize(static_cast<size_t>(ScopeConstants::bufferSize * 3));
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

    auto dot = juce::Point<float>(currentX, currentY);
    if (dot.getDistanceFromOrigin() > 1.0f)
        dot /= dot.getDistanceFromOrigin();
    const float dotX = centre.x + dot.x * radius;
    const float dotY = centre.y - dot.y * radius;

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
    clearScope();
}

void XYPad::clearScope()
{
    scopeWriteIndex = 0;
    scopePointCount = 0;
    repaint(getScopeRepaintArea());
}

void XYPad::pushScopeSamples(const ScopeFrame* frames, int numFrames)
{
    if (!scopeEnabled || frames == nullptr || numFrames <= 0)
        return;

    const int capacity = static_cast<int>(scopePoints.size());
    const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
    for (int i = 0; i < numFrames; ++i)
    {
        for (size_t band = 0; band < 3; ++band)
        {
            const float l = frames[i].left[band];
            const float r = frames[i].right[band];
            if (l * l + r * r < 0.000001f)
                continue;

            // Conventional vectorscope: mono is vertical, stereo differences
            // spread horizontally. Colour describes frequency, not position.
            const float x = (r - l) * 0.70710678f;
            const float y = (l + r) * 0.70710678f;
            scopePoints[static_cast<size_t>(scopeWriteIndex)] = { { x, y }, now, band };
            scopeWriteIndex = (scopeWriteIndex + 1) % capacity;
            scopePointCount = juce::jmin(scopePointCount + 1, capacity);
        }
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

    const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;

    for (int i = 0; i < count; ++i)
    {
        const int index = (startIndex + i >= capacity) ? (startIndex + i - capacity) : (startIndex + i);
        const auto& point = scopePoints[static_cast<size_t>(index)];

        const auto age = now - point.timestamp;
        if (age >= ScopeConstants::persistenceSeconds)
            continue;
        const float alpha = 0.28f * static_cast<float>(1.0 - age / ScopeConstants::persistenceSeconds);
        g.setColour(bandColours[point.band].withAlpha(alpha));

        const float x = centre.x + point.position.x * scopeRadius;
        const float y = centre.y - point.position.y * scopeRadius;
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

    widthSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    widthSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    widthSlider.setTextValueSuffix(" %");
    widthSlider.setNumDecimalPlacesToDisplay(1);
    widthSlider.setDoubleClickReturnValue(true, 0.0);
    widthSlider.setTooltip("Added stereo width: 0% is unchanged, 200% is maximum. Preserves the mono sum; protects deep bass.");
    widthSlider.setName("Stereo width");
    addAndMakeVisible(widthSlider);

    widthLabel.setText("Width", juce::dontSendNotification);
    widthLabel.setJustificationType(juce::Justification::centred);
    widthLabel.attachToComponent(&widthSlider, false);
    addAndMakeVisible(widthLabel);

    const std::array<juce::String, 3> bandNames { "LOW < 250 Hz", "MID 250 Hz - 4 kHz", "HIGH > 4 kHz" };
    for (size_t i = 0; i < bandLabels.size(); ++i)
    {
        bandLabels[i].setText(bandNames[i], juce::dontSendNotification);
        bandLabels[i].setColour(juce::Label::textColourId, bandColours[i]);
        bandLabels[i].setJustificationType(juce::Justification::centred);
        bandLabels[i].setTooltip("Frequency bands overlap smoothly at the crossover frequencies. The cloud shows stereo shape, not physical front/back location.");
        addAndMakeVisible(bandLabels[i]);
    }

    depthSlider.setDoubleClickReturnValue(true, 0.5);
    mixSlider.setDoubleClickReturnValue(true, 1.0);
    depthSlider.setTooltip("Strength of the front/back tone, level and reverb effect.");
    mixSlider.setTooltip("Blend between the original signal and all effects, including Width.");
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
        audioProcessor.getScopeInputBuffer().discard();
        audioProcessor.getScopeOutputBuffer().discard();
        xyPad.setScopeEnabled(scopeToggle.getToggleState());
        audioProcessor.setScopeCaptureEnabled(scopeToggle.getToggleState());
    };

    scopeSourceToggle.onClick = [this]()
    {
        scopeSourceToggle.setButtonText(scopeSourceToggle.getToggleState() ? "IN" : "OUT");
        audioProcessor.getScopeInputBuffer().discard();
        audioProcessor.getScopeOutputBuffer().discard();
        xyPad.clearScope();
    };

    depthAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "Depth", depthSlider);
    mixAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "Mix", mixSlider);

    widthAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, "Width", widthSlider);
    audioProcessor.getScopeInputBuffer().discard();
    audioProcessor.getScopeOutputBuffer().discard();
    audioProcessor.setScopeCaptureEnabled(true);

    startTimerHz(30);

    setSize(460, 560);
}

OrbitalPanAudioProcessorEditor::~OrbitalPanAudioProcessorEditor()
{
    stopTimer();
    audioProcessor.setScopeCaptureEnabled(false);
}

void OrbitalPanAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void OrbitalPanAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(12);

    auto controlsArea = bounds.removeFromBottom(136);
    bounds.removeFromBottom(22); // Space for the knob labels.
    auto legend = bounds.removeFromBottom(26);
    const int legendWidth = legend.getWidth() / 3;
    for (auto& label : bandLabels)
        label.setBounds(legend.removeFromLeft(legendWidth));
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

    const int controlWidth = controlsArea.getWidth() / 3;
    depthSlider.setBounds(controlsArea.removeFromLeft(controlWidth).reduced(10));
    widthSlider.setBounds(controlsArea.removeFromLeft(controlWidth).reduced(10));
    mixSlider.setBounds(controlsArea.reduced(10));
}

void OrbitalPanAudioProcessorEditor::timerCallback()
{
    auto& input = audioProcessor.getScopeInputBuffer();
    auto& output = audioProcessor.getScopeOutputBuffer();
    if (!scopeToggle.getToggleState())
    {
        input.discard();
        output.discard();
        return;
    }

    const bool showInput = scopeSourceToggle.getToggleState();
    auto& selected = showInput ? input : output;
    auto& other = showInput ? output : input;
    other.discard();
    const int pulled = selected.pull(scopeReadBuffer.data(), static_cast<int>(scopeReadBuffer.size()));
    xyPad.pushScopeSamples(scopeReadBuffer.data(), pulled);
    // Continue repainting after transport stops so the trail fades to silence.
    xyPad.repaint(xyPad.getScopeRepaintArea());
}
