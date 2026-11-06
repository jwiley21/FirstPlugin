#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI initialise;
    try
    {
        require(argc == 3, "Usage: OrbitalPanPluginLoadTests <absolute bundle path> <version>");
        juce::AudioPluginFormatManager manager;
        auto* format = new juce::VST3PluginFormat();
        manager.addFormat(format);
        juce::OwnedArray<juce::PluginDescription> descriptions;
        format->findAllTypesForFile(descriptions, juce::String::fromUTF8(argv[1]));
        require(descriptions.size() == 1, "Expected exactly one loadable VST3 effect");
        const auto& description = *descriptions[0];
        require(description.name == "OrbitalPan", "Unexpected plugin identity");
        require(description.version == juce::String::fromUTF8(argv[2]), "Plugin version mismatch");
        juce::String error;
        auto instance = manager.createPluginInstance(description, 48000.0, 512, error);
        if (!instance)
            throw std::runtime_error(error.toStdString());
        instance->setPlayConfigDetails(2, 2, 48000.0, 512);
        instance->prepareToPlay(48000.0, 512);
        require(instance->getTotalNumInputChannels() == 2
                    && instance->getTotalNumOutputChannels() == 2, "Expected stereo IO");
        juce::AudioBuffer<float> buffer(2, 512);
        juce::MidiBuffer midi;
        double energy = 0.0;
        for (int block = 0; block < 8; ++block)
        {
            for (int sample = 0; sample < 512; ++sample)
                for (int channel = 0; channel < 2; ++channel)
                    buffer.setSample(channel, sample, 0.1f * std::sin(
                        static_cast<float>(block * 512 + sample) * 0.05f));
            instance->processBlock(buffer, midi);
            for (int channel = 0; channel < 2; ++channel)
                for (int sample = 0; sample < 512; ++sample)
                {
                    const auto value = buffer.getSample(channel, sample);
                    require(std::isfinite(value), "Non-finite plugin output");
                    energy += static_cast<double>(value) * value;
                }
        }
        require(energy > 0.0, "Plugin output is silent");
        juce::MemoryBlock state;
        instance->getStateInformation(state);
        require(state.getSize() > 0, "Plugin returned empty state");
        instance->setStateInformation(state.getData(), static_cast<int>(state.getSize()));
        std::unique_ptr<juce::AudioProcessorEditor> editor(instance->createEditor());
        require(editor != nullptr, "Plugin editor could not be created");
        editor.reset();
        instance->releaseResources();
        std::cout << "PASS VST3 scan, version, instantiate, stereo processing, state, editor: "
                  << argv[1] << '\n';
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
