#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
constexpr float kSmoothingTimeSeconds = 0.05f;
// Tweak these to change the overall "distance" character.
constexpr float kMaxBrightnessDb = 4.0f; // front/back tone range (high-shelf boost/cut)
constexpr float kMaxLevelDb = 3.0f;      // front/back loudness range
constexpr float kShelfFrequencyHz = 2500.0f;
constexpr float kShelfQ = 0.7f;
constexpr int kControlInterval = 32;
constexpr float kMaxReverbWet = 0.35f;   // back distance reverb amount
}

OrbitalPanAudioProcessor::OrbitalPanAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    posXParameter = apvts.getRawParameterValue("PosX");
    posYParameter = apvts.getRawParameterValue("PosY");
    depthParameter = apvts.getRawParameterValue("Depth");
    mixParameter = apvts.getRawParameterValue("Mix");
    widthParameter = apvts.getRawParameterValue("Width");
}

OrbitalPanAudioProcessor::~OrbitalPanAudioProcessor() = default;

const juce::String OrbitalPanAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool OrbitalPanAudioProcessor::acceptsMidi() const
{
    return false;
}

bool OrbitalPanAudioProcessor::producesMidi() const
{
    return false;
}

bool OrbitalPanAudioProcessor::isMidiEffect() const
{
    return false;
}

double OrbitalPanAudioProcessor::getTailLengthSeconds() const
{
    return 4.0;
}

int OrbitalPanAudioProcessor::getNumPrograms()
{
    return 1;
}

int OrbitalPanAudioProcessor::getCurrentProgram()
{
    return 0;
}

void OrbitalPanAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String OrbitalPanAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void OrbitalPanAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void OrbitalPanAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    posXSmoothed.reset(sampleRate, kSmoothingTimeSeconds);
    posYSmoothed.reset(sampleRate, kSmoothingTimeSeconds);
    depthSmoothed.reset(sampleRate, kSmoothingTimeSeconds);
    mixSmoothed.reset(sampleRate, kSmoothingTimeSeconds);

    posXSmoothed.setCurrentAndTargetValue(*apvts.getRawParameterValue("PosX"));
    posYSmoothed.setCurrentAndTargetValue(*apvts.getRawParameterValue("PosY"));
    depthSmoothed.setCurrentAndTargetValue(*apvts.getRawParameterValue("Depth"));
    mixSmoothed.setCurrentAndTargetValue(*apvts.getRawParameterValue("Mix"));

    widthSmoothed.reset(sampleRate, kSmoothingTimeSeconds);
    widthSmoothed.setCurrentAndTargetValue(widthParameter->load());

    scopeInputBuffer.prepare(sampleRate, samplesPerBlock);
    scopeOutputBuffer.prepare(sampleRate, samplesPerBlock);
    highShelfL.reset();
    highShelfR.reset();
    widener.prepare(sampleRate);
    controlSamplesRemaining = 0;

    // JUCE Reverb scales its dry setting by two. Configure before preparing so
    // its gain smoothers start at the requested values instead of fading in.
    updateFiltersAndReverb(posYSmoothed.getCurrentValue(), depthSmoothed.getCurrentValue());
    reverb.setSampleRate(sampleRate);
    reverb.reset();
}

void OrbitalPanAudioProcessor::releaseResources()
{
}

bool OrbitalPanAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainIn = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    return mainIn == juce::AudioChannelSet::stereo() && mainOut == juce::AudioChannelSet::stereo();
}

void OrbitalPanAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    if (buffer.getNumChannels() < 2)
        return;

    const int numSamples = buffer.getNumSamples();
    const bool captureScope = scopeCaptureEnabled.load(std::memory_order_relaxed);
    if (captureScope)
        scopeInputBuffer.push(buffer.getReadPointer(0), buffer.getReadPointer(1), numSamples);

    posXSmoothed.setTargetValue(posXParameter->load());
    posYSmoothed.setTargetValue(posYParameter->load());
    depthSmoothed.setTargetValue(depthParameter->load());
    mixSmoothed.setTargetValue(mixParameter->load());
    widthSmoothed.setTargetValue(widthParameter->load());

    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);
    std::array<float, kControlInterval> dryL {}, dryR {};

    // Fixed-size chunks handle arbitrary host block sizes without allocating.
    // Tone updates follow the smoothed controls every 32 samples, independent
    // of the host's block size, instead of jumping once per audio block.
    for (int offset = 0; offset < numSamples;)
    {
        if (controlSamplesRemaining == 0)
        {
            updateFiltersAndReverb(posYSmoothed.getCurrentValue(), depthSmoothed.getCurrentValue());
            controlSamplesRemaining = kControlInterval;
        }
        const int count = juce::jmin(controlSamplesRemaining, numSamples - offset);
        for (int i = 0; i < count; ++i)
        {
            dryL[static_cast<size_t>(i)] = left[offset + i];
            dryR[static_cast<size_t>(i)] = right[offset + i];
            left[offset + i] = highShelfL.process(left[offset + i]);
            right[offset + i] = highShelfR.process(right[offset + i]);
        }
        reverb.processStereo(left + offset, right + offset, count);

        for (int i = 0; i < count; ++i)
        {
            const float posX = posXSmoothed.getNextValue();
            const float posY = posYSmoothed.getNextValue();
            const float depth = depthSmoothed.getNextValue();
            const float mix = mixSmoothed.getNextValue();
            const float width = widthSmoothed.getNextValue() * 0.01f;

            // Equal-power stereo balance, normalised to unity at centre.
            const float theta = (posX + 1.0f) * (juce::MathConstants<float>::pi / 4.0f);
            const float leftGain = std::cos(theta) * juce::MathConstants<float>::sqrt2;
            const float rightGain = std::sin(theta) * juce::MathConstants<float>::sqrt2;
            const float level = juce::Decibels::decibelsToGain(posY * depth * kMaxLevelDb);
            float processedL = left[offset + i] * leftGain * level;
            float processedR = right[offset + i] * rightGain * level;

            // Widen after balance so the width control preserves this stage's
            // mono sum even away from the centre of the XY pad.
            widener.process(processedL, processedR, width);
            const auto index = static_cast<size_t>(i);
            left[offset + i] = mix == 0.0f ? dryL[index] : dryL[index] + (processedL - dryL[index]) * mix;
            right[offset + i] = mix == 0.0f ? dryR[index] : dryR[index] + (processedR - dryR[index]) * mix;
        }
        offset += count;
        controlSamplesRemaining -= count;
    }

    if (captureScope)
        scopeOutputBuffer.push(left, right, numSamples);
}

bool OrbitalPanAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* OrbitalPanAudioProcessor::createEditor()
{
    return new OrbitalPanAudioProcessorEditor(*this);
}

void OrbitalPanAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void OrbitalPanAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr)
    {
        if (xmlState->hasTagName(apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml(*xmlState);
            // Old sessions predate Width. Reset it explicitly even if another
            // preset previously left this instance at a nonzero setting.
            if (!state.getChildWithProperty("id", "Width").isValid())
            {
                juce::ValueTree widthState("PARAM");
                widthState.setProperty("id", "Width", nullptr);
                widthState.setProperty("value", 0.0f, nullptr);
                state.appendChild(widthState, nullptr);
            }
            apvts.replaceState(state);
        }
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout OrbitalPanAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "PosX", "PosX", juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "PosY", "PosY", juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "Depth", "Depth", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "Mix", "Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "Width", "Width", juce::NormalisableRange<float>(0.0f, 200.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    return { params.begin(), params.end() };
}

void OrbitalPanAudioProcessor::updateFiltersAndReverb(float posY, float depth)
{
    // Tone mapping: front is brighter, back is darker.
    const float shelfDb = posY * depth * kMaxBrightnessDb;
    const float shelfGain = juce::Decibels::decibelsToGain(shelfDb);

    const auto coeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf(
        currentSampleRate, juce::jmin(kShelfFrequencyHz, static_cast<float>(currentSampleRate * 0.45)),
        kShelfQ, shelfGain);

    highShelfL.setCoefficients(coeffs);
    highShelfR.setCoefficients(coeffs);

    // Reverb mapping: more reverb the further "back" the sound is.
    const float backAmount = juce::jlimit(0.0f, 1.0f, (-posY) * depth);

    reverbParams.roomSize = 0.2f + backAmount * 0.5f;
    reverbParams.damping = 0.5f;
    reverbParams.wetLevel = backAmount * kMaxReverbWet;
    reverbParams.dryLevel = 0.5f; // JUCE internal dry scale = 2, giving unity gain.
    reverbParams.width = 1.0f;
    reverbParams.freezeMode = 0.0f;

    reverb.setParameters(reverbParams);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OrbitalPanAudioProcessor();
}
