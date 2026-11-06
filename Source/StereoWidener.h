#pragma once

#include <array>
#include <algorithm>
#include <cmath>

// Fixed-storage filters: coefficient changes never allocate on the audio thread.
class Biquad
{
public:
    void setCoefficients(const std::array<float, 6>& c) noexcept
    {
        b0 = c[0] / c[3]; b1 = c[1] / c[3]; b2 = c[2] / c[3];
        a1 = c[4] / c[3]; a2 = c[5] / c[3];
    }

    void prepareHighPass(double sampleRate, float frequency) noexcept
    {
        const auto w = 6.283185307179586 * std::min<double>(frequency, sampleRate * 0.45) / sampleRate;
        const auto c = static_cast<float>(std::cos(w));
        const auto alpha = static_cast<float>(std::sin(w) / std::sqrt(2.0));
        setCoefficients({ (1 + c) * 0.5f, -(1 + c), (1 + c) * 0.5f,
                          1 + alpha, -2 * c, 1 - alpha });
        reset();
    }

    float process(float x) noexcept
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    void reset() noexcept { z1 = z2 = 0.0f; }

private:
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
};

// Original mid/side widener. Only an opposite-polarity side signal is added,
// so (L + R) / 2 is unchanged. It also creates width from dual-mono sources.
// The 120 Hz high-pass keeps the added width out of the deepest bass.
class StereoWidener
{
public:
    void prepare(double sampleRate) noexcept
    {
        midHighPass.prepareHighPass(sampleRate, 120.0f);
        sideHighPass.prepareHighPass(sampleRate, 120.0f);
        constexpr std::array<float, 4> frequencies { 350, 900, 2200, 5500 };
        for (size_t i = 0; i < stages.size(); ++i)
        {
            const auto tangent = std::tan(3.141592653589793
                * std::min<double>(frequencies[i], sampleRate * 0.45) / sampleRate);
            stages[i].coefficient = static_cast<float>((tangent - 1.0) / (tangent + 1.0));
            stages[i].state = 0;
        }
    }

    void process(float& left, float& right, float amount) noexcept
    {
        const float mid = midHighPass.process((left + right) * 0.5f);
        const float side = sideHighPass.process((left - right) * 0.5f);
        float shifted = mid;
        for (auto& stage : stages)
            shifted = stage.process(shifted);

        // Run filters even at zero width so automation starts with warm state.
        const float addedSide = amount * (0.25f * (shifted - mid) + 0.5f * side);
        left += addedSide;
        right -= addedSide;
    }

private:
    struct AllPass
    {
        float process(float x) noexcept
        {
            const float y = coefficient * x + state;
            state = x - coefficient * y;
            return y;
        }
        float coefficient = 0, state = 0;
    };

    Biquad midHighPass, sideHighPass;
    std::array<AllPass, 4> stages;
};
