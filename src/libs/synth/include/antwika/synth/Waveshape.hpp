#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include <antwika/enums/Enumeration.hpp>

namespace antwika::synth
{

    enum class Waveshape : std::uint8_t
    {
        Sine = 0,
        Saw,
        Square,
        Triangle,
        Noise,
    };

    [[nodiscard]] constexpr Waveshape enumBound(Waveshape) noexcept
    {
        return Waveshape::Noise;
    }

    inline constexpr std::size_t kWaveshapeCount =
        antwika::enums::kCount<Waveshape>;

    [[nodiscard]] constexpr std::size_t waveshapeIndex(
        const Waveshape shape) noexcept
    {
        return antwika::enums::index(shape);
    }

    [[nodiscard]] constexpr bool isPeriodic(Waveshape shape) noexcept
    {
        return shape != Waveshape::Noise;
    }

    [[nodiscard]] std::string_view waveshapeName(Waveshape shape) noexcept;

}
