#pragma once

#include <cstdint>

namespace antwika::enums::tests
{

    enum class Colour : std::uint8_t
    {
        Red = 0,
        Green,
        Blue,
    };

    [[nodiscard]] constexpr Colour enumBound(Colour) noexcept
    {
        return Colour::Blue;
    }

    enum class Side : std::uint8_t
    {
        Near = 0,
        Far,
    };

    [[nodiscard]] constexpr Side enumBound(Side) noexcept
    {
        return Side::Far;
    }

}
