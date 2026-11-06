#include "../Source/PluginEditor.h"
#include <iostream>
#include <random>
#include <stdexcept>

namespace
{
void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void setParameter(OrbitalPanAudioProcessor& processor, const char* id, float value)
{
    auto* parameter = processor.apvts.getParameter(id);
    parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}

void testWidener()
{
    for (const double rate : { 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        StereoWidener widener;
        widener.prepare(rate);
        std::mt19937 random(42);
        std::uniform_real_distribution<float> noise(-0.8f, 0.8f);
        for (int i = 0; i < 50000; ++i)
        {
            const float originalL = noise(random), originalR = noise(random);
            float l = originalL, r = originalR;
            const float width = static_cast<float>(i % 201) * 0.01f;
            widener.process(l, r, width);
            require(std::isfinite(l) && std::isfinite(r), "Widener produced non-finite samples");
            require(std::abs((l + r) - (originalL + originalR)) < 0.000001f, "Width changed mono sum");
            if (width == 0)
                require(l == originalL && r == originalR, "Zero width is not transparent");
        }

        auto sideEnergy = [rate](double frequency)
        {
            StereoWidener w;
            w.prepare(rate);
            double energy = 0;
            for (int i = 0; i < static_cast<int>(rate); ++i)
            {
                const float mono = 0.5f * static_cast<float>(std::sin(juce::MathConstants<double>::twoPi * frequency * i / rate));
                float l = mono, r = mono;
                w.process(l, r, 2.0f);
                if (i > static_cast<int>(rate * 0.1))
                    energy += (l - r) * (l - r);
            }
            return energy;
        };
        const double midEnergy = sideEnergy(1000);
        require(midEnergy > 1.0, "Mono input did not gain stereo width");
        require(sideEnergy(40) < midEnergy * 0.01, "Deep bass widening is not sufficiently attenuated");
    }
    std::cout << "PASS width transparency, mono sum, mono widening and bass protection at four sample rates\n";
}

void testProcessor()
{
    juce::MidiBuffer midi;
    for (const double rate : { 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        OrbitalPanAudioProcessor processor;
        processor.prepareToPlay(rate, 64);
        processor.setScopeCaptureEnabled(true);
        for (const int size : { 0, 1, 17, 64, 511, 4096 })
        {
            juce::AudioBuffer<float> buffer(2, size);
            for (int i = 0; i < size; ++i)
            {
                buffer.setSample(0, i, 0.23f * std::sin(static_cast<float>(i) * 0.13f));
                buffer.setSample(1, i, 0.31f * std::cos(static_cast<float>(i) * 0.17f));
            }
            juce::AudioBuffer<float> original;
            original.makeCopyOf(buffer);
            processor.processBlock(buffer, midi);
            for (int channel = 0; channel < 2; ++channel)
                for (int i = 0; i < size; ++i)
                    require(std::abs(buffer.getSample(channel, i) - original.getSample(channel, i)) < 0.000002f,
                            "Neutral processor changed audio");
        }

        setParameter(processor, "Mix", 0);
        setParameter(processor, "Width", 200);
        setParameter(processor, "PosY", -0.8f);
        setParameter(processor, "PosX", 0.6f);
        processor.prepareToPlay(rate, 32);
        juce::AudioBuffer<float> dry(2, 8192);
        for (int i = 0; i < dry.getNumSamples(); ++i)
        {
            dry.setSample(0, i, 0.2f);
            dry.setSample(1, i, -0.3f);
        }
        processor.processBlock(dry, midi);
        for (int i = 0; i < dry.getNumSamples(); ++i)
            require(dry.getSample(0, i) == 0.2f && dry.getSample(1, i) == -0.3f, "Mix=0 is not bit-transparent");
    }
    std::cout << "PASS neutral unity, dry bypass, empty/variable/oversized host blocks\n";

    // Width's mono preservation must also hold AFTER pan/depth/reverb and mixing.
    OrbitalPanAudioProcessor narrow, wide;
    for (auto* processor : { &narrow, &wide })
    {
        setParameter(*processor, "PosX", -0.7f);
        setParameter(*processor, "PosY", -0.6f);
        setParameter(*processor, "Depth", 0.9f);
        setParameter(*processor, "Mix", 0.63f);
        processor->prepareToPlay(48000, 127);
    }
    setParameter(wide, "Width", 200);
    juce::AudioBuffer<float> a(2, 127), b(2, 127);
    for (int block = 0; block < 500; ++block)
    {
        if (block % 37 == 0)
            setParameter(wide, "Width", static_cast<float>(block % 201));
        for (int i = 0; i < 127; ++i)
        {
            a.setSample(0, i, 0.2f * std::sin(static_cast<float>(block * 127 + i) * 0.17f));
            a.setSample(1, i, 0.3f * std::cos(static_cast<float>(block * 127 + i) * 0.11f));
        }
        b.makeCopyOf(a, true);
        narrow.processBlock(a, midi);
        wide.processBlock(b, midi);
        for (int i = 0; i < 127; ++i)
            require(std::abs(a.getSample(0, i) + a.getSample(1, i)
                             - b.getSample(0, i) - b.getSample(1, i)) < 0.000002f,
                    "Automated width changed full processor mono sum");
    }
    std::cout << "PASS full-chain mono preservation with width automation, pan, depth and mix\n";
}

void testState()
{
    OrbitalPanAudioProcessor source, restored;
    setParameter(source, "Width", 137.5f);
    setParameter(source, "Depth", 0.8f);
    juce::MemoryBlock state;
    source.getStateInformation(state);
    restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    require(std::abs(restored.apvts.getRawParameterValue("Width")->load() - 137.5f) < 0.01f, "Width did not round-trip");

    auto legacy = source.apvts.copyState();
    legacy.removeChild(legacy.getChildWithProperty("id", "Width"), nullptr);
    const auto xml = legacy.createXml();
    juce::AudioProcessor::copyXmlToBinary(*xml, state);
    restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    require(restored.apvts.getRawParameterValue("Width")->load() == 0.0f, "Legacy state did not reset Width");
    require(std::abs(restored.apvts.getRawParameterValue("Depth")->load() - 0.8f) < 0.001f, "Legacy Depth was lost");
    std::cout << "PASS state round-trip and old-session Width migration\n";
}

void testScope()
{
    for (const double rate : { 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        for (size_t expectedBand = 0; expectedBand < 3; ++expectedBand)
        {
            const std::array<double, 3> frequencies { 80, 1000, 10000 };
            ScopeBuffer scope;
            scope.prepare(rate, 512);
            std::array<float, 512> left {}, right {};
            std::array<ScopeFrame, ScopeConstants::bufferSize> frames;
            std::array<double, 3> energy {};
            for (int block = 0; block < 100; ++block)
            {
                for (size_t i = 0; i < left.size(); ++i)
                {
                    left[i] = static_cast<float>(std::sin(juce::MathConstants<double>::twoPi
                        * frequencies[expectedBand] * (block * 512 + static_cast<int>(i)) / rate)) * 0.5f;
                    right[i] = left[i] * 0.5f;
                }
                scope.push(left.data(), right.data(), static_cast<int>(left.size()));
                const int count = scope.pull(frames.data(), static_cast<int>(frames.size()));
                if (block < 10)
                    continue;
                for (int i = 0; i < count; ++i)
                    for (size_t band = 0; band < 3; ++band)
                    {
                        const auto& frame = frames[static_cast<size_t>(i)];
                        energy[band] += frame.left[band] * frame.left[band];
                        require(std::abs(frame.right[band] - 0.5f * frame.left[band]) < 0.00001f,
                                "Scope lost the simultaneous stereo pair");
                    }
            }
            for (size_t band = 0; band < 3; ++band)
                if (band != expectedBand)
                    require(energy[expectedBand] > energy[band] * 100.0, "Frequency colour assigned to wrong band");
            require(energy[expectedBand] > 1.0, "Scope missed a tone");
            // At 192 kHz the display emits one frame per 48 audio samples.
            // Supply enough audio to fill the FIFO even at that sample rate.
            for (int i = 0; i < 300; ++i)
                scope.push(left.data(), right.data(), 512);
            require(scope.pull(frames.data(), static_cast<int>(frames.size())) == ScopeConstants::bufferSize - 1,
                    "Scope overflow corrupted the queue");
            scope.discard();
            require(scope.pull(frames.data(), 10) == 0, "Scope discard left stale samples");
        }
    }
    std::cout << "PASS three-band frequency separation, stereo pairing, FIFO wrap/overflow/discard at four sample rates\n";
}
void renderPreview(const juce::File& destination)
{
    OrbitalPanAudioProcessor processor;
    setParameter(processor, "Width", 110);
    processor.prepareToPlay(48000, 512);
    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    juce::AudioBuffer<float> audio(2, 512);
    juce::MidiBuffer midi;
    for (int block = 0; block < 24; ++block)
    {
        for (int i = 0; i < audio.getNumSamples(); ++i)
        {
            const double time = (block * 512 + i) / 48000.0;
            const auto tone = [time](double hz, double phase)
            { return static_cast<float>(std::sin(juce::MathConstants<double>::twoPi * hz * time + phase)); };
            audio.setSample(0, i, 0.38f * tone(80, 0) + 0.26f * tone(1000, 0) + 0.23f * tone(10000, 0));
            audio.setSample(1, i, 0.38f * tone(80, 0) + 0.33f * tone(1000, 0.7) + 0.25f * tone(10000, 1.7));
        }
        processor.processBlock(audio, midi);
    }
    std::array<ScopeFrame, ScopeConstants::bufferSize> frames;
    const int count = processor.getScopeOutputBuffer().pull(frames.data(), static_cast<int>(frames.size()));
    require(count > 0, "Editor did not enable scope capture");
    // Use software rendering for a deterministic offscreen export, and prime
    // font resources before starting the short-lived trail.
    juce::Image snapshot(juce::Image::ARGB, editor->getWidth() * 2, editor->getHeight() * 2,
                         true, juce::SoftwareImageType());
    juce::Graphics graphics(snapshot);
    graphics.addTransform(juce::AffineTransform::scale(2.0f));
    editor->paintEntireComponent(graphics, true);
    bool foundPad = false;
    for (auto* child : editor->getChildren())
        if (auto* pad = dynamic_cast<XYPad*>(child))
        {
            foundPad = true;
            pad->pushScopeSamples(frames.data(), count);
        }
    require(foundPad, "Editor preview could not find the scope");
    editor->paintEntireComponent(graphics, true);
    auto stream = destination.createOutputStream();
    require(stream != nullptr, "Could not open UI preview file");
    stream->setPosition(0);
    stream->truncate();
    require(juce::PNGImageFormat().writeImageToStream(snapshot, *stream), "Could not save UI preview");
    std::cout << "Saved UI preview: " << destination.getFullPathName() << '\n';
}

}

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    try
    {
        testWidener();
        testProcessor();
        testState();
        testScope();
        if (argc > 1)
            renderPreview(juce::File(juce::String::fromUTF8(argv[1])));
        std::cout << "All audio regression tests passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
