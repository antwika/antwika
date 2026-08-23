#pragma once

#include <cstdint>

namespace antwika::enums::tests
{

    enum class Color : std::uint8_t
    {
        Red = 0,
        Green,
        Blue,
    };

    [[nodiscard]] constexpr Color getLastEnumerator(Color) noexcept
    {
        return Color::Blue;
    }

    enum class Side : std::uint8_t
    {
        Near = 0,
        Far,
    };

    [[nodiscard]] constexpr Side getLastEnumerator(Side) noexcept
    {
        return Side::Far;
    }

}
