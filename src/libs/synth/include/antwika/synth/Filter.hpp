#pragma once

#include <cstddef>
#include <cstdint>

#include <antwika/sound/WaveFormat.hpp>

#include <antwika/enums/Enumeration.hpp>

namespace antwika::synth
{

    using antwika::sound::SampleRate;

    enum class FilterMode : std::uint8_t
    {
        None = 0,
        LowPass,
        HighPass,
        BandPass,
    };

    [[nodiscard]] constexpr FilterMode enumBound(FilterMode) noexcept
    {
        return FilterMode::BandPass;
    }

    inline constexpr std::size_t kFilterModeCount =
        antwika::enums::kCount<FilterMode>;

    struct FilterDesc final
    {
        FilterMode mode = FilterMode::None;

        double cutoff = 0.0;

        double resonance = 1.0;

        [[nodiscard]] bool operator==(const FilterDesc &other) const
            = default;
    };

    struct FilterCoefficients final
    {
        double frequency = 0.0;
        double damping = 0.0;

        [[nodiscard]] bool operator==(const FilterCoefficients &other) const
            = default;
    };

    struct FilterState final
    {
        double low = 0.0;
        double band = 0.0;

        [[nodiscard]] bool operator==(const FilterState &other) const
            = default;
    };

    [[nodiscard]] FilterCoefficients filterCoefficientsFor(
        const FilterDesc &desc, SampleRate rate) noexcept;

    [[nodiscard]] float filterSample(
        FilterMode mode,
        const FilterCoefficients &coefficients,
        FilterState &state,
        float input) noexcept;

}
