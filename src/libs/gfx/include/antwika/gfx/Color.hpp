#pragma once

#include <cstdint>

namespace antwika::gfx
{

    /**
     * @brief A straight (non-premultiplied) 8-bit-per-channel RGBA colour.
     */
    struct Color
    {
        std::uint8_t red = 0;
        std::uint8_t green = 0;
        std::uint8_t blue = 0;
        std::uint8_t alpha = 255;

        /**
         * @brief Compare two colours channel by channel.
         * @param other The colour to compare against.
         * @return True when every channel matches.
         */
        [[nodiscard]] bool operator==(const Color &other) const = default;
    };

} // namespace antwika::gfx
