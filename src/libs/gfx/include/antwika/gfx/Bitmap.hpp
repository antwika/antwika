#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

    inline constexpr std::size_t kBytesPerPixel = 4;

    struct Bitmap final
    {
        Size size;
        std::vector<std::uint8_t> pixels;

        [[nodiscard]] bool isComplete() const noexcept;

        [[nodiscard]] bool operator==(const Bitmap &other) const = default;
    };

}
