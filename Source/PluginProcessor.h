#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class OrbitalPanAudioProcessor : public juce::AudioProcessor
{
public:
    OrbitalPanAudioProcessor();
    ~OrbitalPanAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    void updateFiltersAndReverb(float posY, float depth);

    juce::AudioBuffer<float> dryBuffer;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> posXSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> posYSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> depthSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed;

    // JUCE IIR filters used for a simple high-shelf tone change (front = brighter, back = darker).
    juce::dsp::IIR::Filter<float> highShelfL;
    juce::dsp::IIR::Filter<float> highShelfR;

    // JUCE's lightweight algorithmic reverb for the "behind" effect.
    juce::Reverb reverb;
    juce::Reverb::Parameters reverbParams;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrbitalPanAudioProcessor)
};
