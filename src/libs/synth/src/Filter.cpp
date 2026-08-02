#include "antwika/synth/Filter.hpp"

#include <algorithm>
#include <cmath>

#include <antwika/sound/WaveFormat.hpp>

namespace antwika::synth
{

    namespace
    {
        constexpr double kPi = 3.14159265358979323846;

        // A Chamberlin filter is stable up to a coefficient of one.
        // That coefficient is 2 sin(pi r) for a cutoff ratio r.
        // So r may not pass one sixth.
        // Clamping here rather than refusing in trigger() is meant.
        // trigger() checks a cutoff against the Nyquist frequency.
        // That is a statement about what a caller meant.
        // This is one about what the arithmetic can survive.
        constexpr double kMaxRatio = 1.0 / 6.0;
    } // namespace

    FilterCoefficients filterCoefficientsFor(
        const FilterDesc &desc, SampleRate rate) noexcept
    {
        if (desc.mode == FilterMode::None)
        {
            return FilterCoefficients{};
        }

        const auto ratio = desc.cutoff / static_cast<double>(rate);
        const auto clamped = std::clamp(ratio, 0.0, kMaxRatio);

        return FilterCoefficients{
            .frequency = 2.0 * std::sin(kPi * clamped),
            .damping = desc.resonance};
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

        // A mode no enumerator names, which trigger() cannot produce.
        // Only a caller casting an integer in reaches this.
        // It gets the sample back rather than a terminate.
        return input;
    }

} // namespace antwika::synth
