#include "antwika/synth/Filter.hpp"

#include <algorithm>
#include <cmath>

#include <antwika/sound/WaveFormat.hpp>

namespace antwika::synth
{

    namespace
    {
        constexpr double kPi = 3.14159265358979323846;

        // A Chamberlin filter is stable while f*f + 2*f*q < 4.
        // The frequency coefficient f is 2 sin(pi r) for a ratio r.
        // Clamping r to one sixth holds f itself to one.
        // Clamping here rather than refusing in trigger() is meant.
        // trigger() checks a cutoff against the Nyquist frequency.
        // That is a statement about what a caller meant.
        // This is one about what the arithmetic can survive.
        constexpr double kMaxRatio = 1.0 / 6.0;

        // How far inside the stability bound the damping stays.
        // On the bound itself the recurrence rings without decay.
        constexpr double kDampingMargin = 1.0 / 1024.0;
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

        const auto frequency = 2.0 * std::sin(kPi * clamped);

        // Stability binds the pair, not the frequency alone.
        // A damping past (4 - f*f) / 2f grows without limit.
        // Every sample of such a voice reaches the speakers.
        // So the resonance a caller asked for is held below it.
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

        // A mode no enumerator names, which trigger() cannot produce.
        // Only a caller casting an integer in reaches this.
        // It gets the sample back rather than a terminate.
        return input;
    }

} // namespace antwika::synth
