#pragma once

#include <cstdint>
#include <string_view>

#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/Rect.hpp"

namespace antwika::gfx
{

    /**
     * @brief Draws into one window's drawable area.
     *
     * Drawing is deliberately a write-only projection of application
     * state: nothing here reports back into the simulation, so rendering
     * cannot influence what a replay reproduces.
     */
    class IRenderer
    {
    public:
        virtual ~IRenderer() = default;

        /**
         * @brief Fill the whole drawable area with one colour.
         * @param color The colour to fill with.
         */
        virtual void clear(Color color) = 0;

        /**
         * @brief Fill a rectangle with one colour.
         * @param rect The rectangle to fill.
         * @param color The colour to fill it with.
         */
        virtual void drawRect(Rect rect, Color color) = 0;

        /**
         * @brief Draw a line of text in the built-in fixed-cell font.
         *
         * Every backend draws the same glyphs at the same metrics, which
         * is why gfx::textSize() can lay text out without asking anyone.
         * There is no other font and none can be loaded.
         * @param origin Top-left corner of the first glyph's cell.
         * @param text The characters to draw; one the font has no glyph
         * for draws as a blank cell of the same width.
         * @param scale Pixels per glyph pixel; zero draws nothing.
         * @param color The colour to draw the lit pixels in.
         */
        virtual void drawText(
            Point origin,
            std::string_view text,
            std::uint32_t scale,
            Color color) = 0;

        /**
         * @brief Make everything drawn since the last present visible.
         */
        virtual void present() = 0;
    };

} // namespace antwika::gfx
