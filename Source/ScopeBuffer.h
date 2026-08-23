#pragma once

#include <algorithm>
#include <atomic>
#include <vector>

namespace ScopeConstants
{
constexpr int bufferSize = 4096;
constexpr int minDownsample = 8;
constexpr int maxDownsample = 32;
constexpr double targetRateHz = 4000.0;
}

class ScopeBuffer
{
public:
    void prepare(int capacity, int downsampleFactor)
    {
        capacity = std::max(1, capacity);
        bufferL.resize(static_cast<size_t>(capacity));
        bufferR.resize(static_cast<size_t>(capacity));
        size = capacity;
        writeIndex.store(0, std::memory_order_relaxed);
        readIndex.store(0, std::memory_order_relaxed);
        setDownsampleFactor(downsampleFactor);
    }

    void setDownsampleFactor(int factor)
    {
        downsampleFactor = std::max(1, factor);
        downsampleCounter = 0;
    }

    void push(const float* left, const float* right, int numSamples) noexcept
    {
        if (size == 0 || left == nullptr || right == nullptr || numSamples <= 0)
            return;

        int localWrite = writeIndex.load(std::memory_order_relaxed);
        const int localRead = readIndex.load(std::memory_order_acquire);
        int counter = downsampleCounter;

        for (int i = 0; i < numSamples; ++i)
        {
            if (counter == 0)
            {
                const int nextWrite = (localWrite + 1 == size) ? 0 : localWrite + 1;
                if (nextWrite != localRead)
                {
                    bufferL[static_cast<size_t>(localWrite)] = left[i];
                    bufferR[static_cast<size_t>(localWrite)] = right[i];
                    localWrite = nextWrite;
                }

                counter = downsampleFactor - 1;
            }
            else
            {
                --counter;
            }
        }

        downsampleCounter = counter;
        writeIndex.store(localWrite, std::memory_order_release);
    }

    int pull(float* destLeft, float* destRight, int maxSamples) noexcept
    {
        if (size == 0 || destLeft == nullptr || destRight == nullptr || maxSamples <= 0)
            return 0;

        int localRead = readIndex.load(std::memory_order_relaxed);
        const int localWrite = writeIndex.load(std::memory_order_acquire);

        const int available = (localWrite >= localRead)
            ? (localWrite - localRead)
            : (size - localRead + localWrite);

        const int toRead = std::min(maxSamples, available);

        for (int i = 0; i < toRead; ++i)
        {
            destLeft[i] = bufferL[static_cast<size_t>(localRead)];
            destRight[i] = bufferR[static_cast<size_t>(localRead)];
            localRead = (localRead + 1 == size) ? 0 : localRead + 1;
        }

        readIndex.store(localRead, std::memory_order_release);
        return toRead;
    }

    int getCapacity() const noexcept
    {
        return size;
    }

private:
    std::vector<float> bufferL;
    std::vector<float> bufferR;
    std::atomic<int> writeIndex{ 0 };
    std::atomic<int> readIndex{ 0 };
    int size = 0;
    int downsampleFactor = ScopeConstants::minDownsample;
    int downsampleCounter = 0;
};
