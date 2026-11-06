#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <juce_dsp/juce_dsp.h>

namespace ScopeConstants
{
constexpr int bufferSize = 2048;
constexpr double targetRateHz = 4000.0;
constexpr float lowCrossoverHz = 250.0f;
constexpr float highCrossoverHz = 4000.0f;
constexpr double persistenceSeconds = 0.35;
}

struct ScopeFrame
{
    std::array<float, 3> left {}, right {};
};

class ScopeBuffer
{
public:
    // Only the producer's DSP state is reset here. Fixed FIFO storage and indices
    // remain valid if the host prepares audio while the editor is reading.
    void prepare(double sampleRate, int maximumBlockSize)
    {
        juce::dsp::ProcessSpec spec { sampleRate,
            static_cast<juce::uint32>(std::max(1, maximumBlockSize)), 2 };
        lowSplit.prepare(spec);
        highSplit.prepare(spec);
        lowSplit.setCutoffFrequency(std::min(ScopeConstants::lowCrossoverHz, static_cast<float>(sampleRate * 0.2)));
        highSplit.setCutoffFrequency(std::min(ScopeConstants::highCrossoverHz, static_cast<float>(sampleRate * 0.45)));
        downsampleFactor = std::max(1, static_cast<int>(std::round(sampleRate / ScopeConstants::targetRateHz)));
        downsampleCounter = 0;
        peakFrame = {};
    }

    void push(const float* left, const float* right, int numSamples) noexcept
    {
        int localWrite = writeIndex.load(std::memory_order_relaxed);
        const int localRead = readIndex.load(std::memory_order_acquire);

        for (int i = 0; i < numSamples; ++i)
        {
            ScopeFrame frame;
            split(0, left[i], frame.left);
            split(1, right[i], frame.right);
            // Analyse at the full audio rate BEFORE reducing display traffic.
            // Keep a simultaneous L/R peak pair per band; treble cannot disappear
            // merely because the display's sampling interval hits its zeroes.
            for (size_t band = 0; band < 3; ++band)
                if (frame.left[band] * frame.left[band] + frame.right[band] * frame.right[band]
                    >= peakFrame.left[band] * peakFrame.left[band] + peakFrame.right[band] * peakFrame.right[band])
                {
                    peakFrame.left[band] = frame.left[band];
                    peakFrame.right[band] = frame.right[band];
                }

            if (++downsampleCounter >= downsampleFactor)
            {
                const int nextWrite = (localWrite + 1) % ScopeConstants::bufferSize;
                if (nextWrite != localRead)
                {
                    frames[static_cast<size_t>(localWrite)] = peakFrame;
                    localWrite = nextWrite;
                }
                peakFrame = {};
                downsampleCounter = 0;
            }
        }
        writeIndex.store(localWrite, std::memory_order_release);
    }

    int pull(ScopeFrame* destination, int maximumFrames) noexcept
    {
        int localRead = readIndex.load(std::memory_order_relaxed);
        const int localWrite = writeIndex.load(std::memory_order_acquire);
        int count = 0;
        while (localRead != localWrite && count < maximumFrames)
        {
            destination[count++] = frames[static_cast<size_t>(localRead)];
            localRead = (localRead + 1) % ScopeConstants::bufferSize;
        }
        readIndex.store(localRead, std::memory_order_release);
        return count;
    }

    // Consumer thread only; never move the read index from the audio thread.
    void discard() noexcept
    {
        readIndex.store(writeIndex.load(std::memory_order_acquire), std::memory_order_release);
    }

private:
    void split(int channel, float input, std::array<float, 3>& bands) noexcept
    {
        float upper;
        lowSplit.processSample(channel, input, bands[0], upper);
        highSplit.processSample(channel, upper, bands[1], bands[2]);
    }

    std::array<ScopeFrame, ScopeConstants::bufferSize> frames {};
    std::atomic<int> writeIndex { 0 }, readIndex { 0 };
    juce::dsp::LinkwitzRileyFilter<float> lowSplit, highSplit;
    ScopeFrame peakFrame;
    int downsampleFactor = 1, downsampleCounter = 0;
};
