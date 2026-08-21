#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

    inline constexpr std::size_t kBytesPerPixel = 4;

    struct Bitmap final
    {
        Size size;
        std::vector<std::uint8_t> pixels;

        [[nodiscard]] bool isValid() const noexcept;

        [[nodiscard]] bool operator==(const Bitmap &other) const = default;
    };

    [[nodiscard]] std::optional<Color> colorAt(
        const Bitmap &bitmap, std::int32_t x, std::int32_t y);

    void setColorAt(
        Bitmap &bitmap, std::int32_t x, std::int32_t y, Color color);

}
