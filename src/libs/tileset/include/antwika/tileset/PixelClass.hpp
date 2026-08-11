#pragma once

#include <cstdint>
#include <string_view>

namespace antwika::tileset
{

    enum class PixelClass : std::uint8_t
    {
        Blank = 0,
        Paper,
        Ink,
    };

    [[nodiscard]] constexpr PixelClass enumBound(PixelClass) noexcept
    {
        return PixelClass::Ink;
    }

    [[nodiscard]] std::string_view toString(PixelClass pixel) noexcept;

}
