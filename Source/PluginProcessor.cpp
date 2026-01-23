#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr float kSmoothingTimeSeconds = 0.05f;
// Tweak these to change the overall "distance" character.
constexpr float kMaxBrightnessDb = 4.0f; // front/back tone range (high-shelf boost/cut)
constexpr float kMaxLevelDb = 3.0f;      // front/back loudness range
constexpr float kShelfFrequencyHz = 2500.0f;
constexpr float kShelfQ = 0.7f;
constexpr float kMaxReverbWet = 0.35f;   // back distance reverb amount
}

OrbitalPanAudioProcessor::OrbitalPanAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
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
    return 1.0;
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

    dryBuffer.setSize(2, samplesPerBlock, false, false, true);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 1;

    highShelfL.prepare(spec);
    highShelfR.prepare(spec);

    highShelfL.reset();
    highShelfR.reset();

    reverb.reset();

    updateFiltersAndReverb(posYSmoothed.getCurrentValue(), depthSmoothed.getCurrentValue());
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

    if (dryBuffer.getNumSamples() != buffer.getNumSamples())
        dryBuffer.setSize(2, buffer.getNumSamples(), false, false, true);

    dryBuffer.makeCopyOf(buffer, true);

    posXSmoothed.setTargetValue(*apvts.getRawParameterValue("PosX"));
    posYSmoothed.setTargetValue(*apvts.getRawParameterValue("PosY"));
    depthSmoothed.setTargetValue(*apvts.getRawParameterValue("Depth"));
    mixSmoothed.setTargetValue(*apvts.getRawParameterValue("Mix"));

    // Update the tone and reverb for this block using the current smoothed values.
    updateFiltersAndReverb(posYSmoothed.getCurrentValue(), depthSmoothed.getCurrentValue());

    // Apply the front/back brightness shaping.
    juce::dsp::AudioBlock<float> block(buffer);
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    highShelfL.process(juce::dsp::ProcessContextReplacing<float>(leftBlock));
    highShelfR.process(juce::dsp::ProcessContextReplacing<float>(rightBlock));

    // Add a simple reverb for the "behind" position.
    reverb.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1), buffer.getNumSamples());

    auto* wetL = buffer.getWritePointer(0);
    auto* wetR = buffer.getWritePointer(1);
    const auto* dryL = dryBuffer.getReadPointer(0);
    const auto* dryR = dryBuffer.getReadPointer(1);

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const float posX = posXSmoothed.getNextValue();
        const float posY = posYSmoothed.getNextValue();
        const float depth = depthSmoothed.getNextValue();
        const float mix = mixSmoothed.getNextValue();

        // Equal-power pan: -1 = left, +1 = right.
        const float theta = (posX + 1.0f) * (juce::MathConstants<float>::pi / 4.0f);
        const float leftGain = std::cos(theta);
        const float rightGain = std::sin(theta);

        // Front/back level: front slightly louder, back slightly quieter.
        const float levelDb = posY * depth * kMaxLevelDb;
        const float level = juce::Decibels::decibelsToGain(levelDb);

        const float processedL = wetL[sample] * leftGain * level;
        const float processedR = wetR[sample] * rightGain * level;

        // Standard wet/dry crossfade. Mix = 0 -> dry only, Mix = 1 -> fully processed.
        wetL[sample] = dryL[sample] + (processedL - dryL[sample]) * mix;
        wetR[sample] = dryR[sample] + (processedR - dryR[sample]) * mix;
    }
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
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
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

    return { params.begin(), params.end() };
}

void OrbitalPanAudioProcessor::updateFiltersAndReverb(float posY, float depth)
{
    // Tone mapping: front is brighter, back is darker.
    const float shelfDb = posY * depth * kMaxBrightnessDb;
    const float shelfGain = juce::Decibels::decibelsToGain(shelfDb);

    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        currentSampleRate, kShelfFrequencyHz, kShelfQ, shelfGain);

    *highShelfL.coefficients = *coeffs;
    *highShelfR.coefficients = *coeffs;

    // Reverb mapping: more reverb the further "back" the sound is.
    const float backAmount = juce::jlimit(0.0f, 1.0f, (-posY) * depth);

    reverbParams.roomSize = 0.2f + backAmount * 0.5f;
    reverbParams.damping = 0.5f;
    reverbParams.wetLevel = backAmount * kMaxReverbWet;
    reverbParams.dryLevel = 1.0f;
    reverbParams.width = 1.0f;
    reverbParams.freezeMode = 0.0f;

    reverb.setParameters(reverbParams);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OrbitalPanAudioProcessor();
}
