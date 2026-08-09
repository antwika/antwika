#include "antwika/synth/Filter.hpp"

#include <algorithm>
#include <cmath>

#include <antwika/sound/WaveFormat.hpp>

namespace antwika::synth
{

    namespace
    {
        constexpr double kPi = 3.14159265358979323846;

        constexpr double kMaxRatio = 1.0 / 6.0;

        constexpr double kDampingMargin = 1.0 / 1024.0;
    }

    FilterCoefficients filterCoefficientsFor(
        const FilterDesc &desc, SampleRate rate) noexcept
    {
        if (desc.mode == FilterMode::None)
        {
            return FilterCoefficients{};
        }

        const auto ratio = desc.cutoff / static_cast<double>(rate);
        const auto clamped = std::clamp(ratio, 0.0, kMaxRatio);

        const auto frequency = 2.0 * std::sin(kPi * clamped);

        const auto ceiling =
            (4.0 - frequency * frequency) / (2.0 * frequency)
            - kDampingMargin;

        return FilterCoefficients{
            .frequency = frequency,
            .damping = std::min(desc.resonance, ceiling)};
    }

    float filterSample(
        FilterMode mode,
        const FilterCoefficients &coefficients,
        FilterState &state,
        float input) noexcept
    {
        if (mode == FilterMode::None)
        {
            return input;
        }

        state.low += coefficients.frequency * state.band;

        const auto high = static_cast<double>(input) - state.low
            - coefficients.damping * state.band;

        state.band += coefficients.frequency * high;

        if (mode == FilterMode::LowPass)
        {
            return static_cast<float>(state.low);
        }

        if (mode == FilterMode::HighPass)
        {
            return static_cast<float>(high);
        }

        if (mode == FilterMode::BandPass)
        {
            return static_cast<float>(state.band);
        }

        return input;
    }

}
