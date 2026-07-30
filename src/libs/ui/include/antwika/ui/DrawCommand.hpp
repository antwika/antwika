#pragma once

#include <cstdint>
#include <string>
#include <variant>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

namespace antwika::ui
{

    using antwika::gfx::Color;
    using antwika::gfx::Point;
    using antwika::gfx::Rect;

    /**
     * @brief Fill one rectangle with one colour.
     */
    struct FillRect
    {
        Rect rect{};
        Color color{};

        /**
         * @brief Compare two fills.
         * @param other The fill to compare against.
         * @return True when the rectangle and the colour both match.
         */
        [[nodiscard]] bool operator==(const FillRect &other) const = default;
    };

    /**
     * @brief Draw one line of text.
     *
     * The text is already cut to what fits, so whatever draws this does
     * not need to measure anything or know how wide the container was.
     */
    struct DrawText
    {
        Point origin{};
        std::string text{};
        std::uint32_t scale = 0;
        Color color{};

        /**
         * @brief Compare two pieces of text.
         * @param other The text to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const DrawText &other) const = default;
    };

    /**
     * @brief One thing to draw.
     *
     * A value rather than a call into a renderer, which is what lets a
     * whole picture be compared against an expected one in a test
     * without a renderer, a window or a graphics framework existing.
     */
    using DrawCommand = std::variant<FillRect, DrawText>;

} // namespace antwika::ui
