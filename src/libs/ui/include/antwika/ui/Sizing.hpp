#pragma once

#include <cstdint>

namespace antwika::ui
{

    enum class SizeMode : std::uint8_t
    {
        Fixed = 0,

        Fit,

        Grow,
    };

    struct Sizing final
    {
        SizeMode mode = SizeMode::Fit;
        std::uint32_t pixels = 0;

        [[nodiscard]] bool operator==(const Sizing &other) const = default;
    };

    inline constexpr Sizing kFitSizing{.mode = SizeMode::Fit};

    inline constexpr Sizing kGrowSizing{.mode = SizeMode::Grow};

    [[nodiscard]] constexpr Sizing fixedSize(std::uint32_t pixels) noexcept
    {
        return Sizing{.mode = SizeMode::Fixed, .pixels = pixels};
    }

}
