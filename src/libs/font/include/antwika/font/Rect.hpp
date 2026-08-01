#pragma once

#include <cstdint>

namespace antwika::font
{

    /**
     * @brief A rectangle of an atlas, given by its top-left corner.
     *
     * Deliberately not antwika::gfx::Rect, for Coverage's reason: this
     * library names no graphics type.
     * It is unsigned because it addresses a Coverage, which has no
     * pixels left of zero, where gfx::Rect is signed because a window
     * has somewhere off-screen to be.
     */
    struct Rect
    {
        std::uint32_t x = 0;
        std::uint32_t y = 0;
        std::uint32_t width = 0;
        std::uint32_t height = 0;

        /**
         * @brief Compare two rectangles.
         * @param other The rectangle to compare against.
         * @return True when all four fields match.
         */
        [[nodiscard]] bool operator==(const Rect &other) const = default;
    };

} // namespace antwika::font
