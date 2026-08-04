#pragma once

#include <cstdint>

namespace antwika::geometry
{

    /**
     * @brief A width and height in pixels.
     */
    struct Size
    {
        std::uint32_t width = 0;
        std::uint32_t height = 0;

        /**
         * @brief Compare two sizes.
         * @param other The size to compare against.
         * @return True when both dimensions match.
         */
        [[nodiscard]] bool operator==(const Size &other) const = default;
    };

} // namespace antwika::geometry
