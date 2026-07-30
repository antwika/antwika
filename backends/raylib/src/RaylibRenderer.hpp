#pragma once

#include <cstdint>
#include <string_view>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

namespace antwika::gfx::raylib
{

    /**
     * @brief Draws into raylib's one window.
     *
     * raylib wants drawing bracketed by BeginDrawing/EndDrawing, which
     * IRenderer has no equivalent of. The bracket is opened lazily by the
     * first drawing call and closed by present(), so callers keep the
     * clear/draw/present shape every other backend uses.
     */
    class RaylibRenderer final : public IRenderer
    {
    public:
        RaylibRenderer() = default;

        RaylibRenderer(const RaylibRenderer &) = delete;
        RaylibRenderer(RaylibRenderer &&) = delete;

        RaylibRenderer &operator=(const RaylibRenderer &) = delete;
        RaylibRenderer &operator=(RaylibRenderer &&) = delete;

        /**
         * @brief Fill the whole drawable area with one colour.
         * @param color The colour to fill with.
         */
        void clear(Color color) override;

        /**
         * @brief Fill a rectangle with one colour.
         * @param rect The rectangle to fill.
         * @param color The colour to fill it with.
         */
        void drawRect(Rect rect, Color color) override;

        /**
         * @brief Draw a line of text in the built-in fixed-cell font.
         *
         * Painted from gfx::glyphRow() as filled rectangles rather than
         * with raylib's own DrawText, even though raylib ships a default
         * font that would make that a one-liner. That font is not
         * fixed-cell, so using it would break the metrics gfx::textSize()
         * promises and make this backend draw a different picture from
         * every other one.
         * @param origin Top-left corner of the first glyph's cell.
         * @param text The characters to draw.
         * @param scale Pixels per glyph pixel.
         * @param color The colour to draw the lit pixels in.
         */
        void drawText(
            Point origin,
            std::string_view text,
            std::uint32_t scale,
            Color color) override;

        /**
         * @brief Close the drawing bracket, presenting the frame.
         */
        void present() override;

        /**
         * @brief Close any open bracket before the window goes away.
         */
        void detach();

    private:
        void beginIfNeeded();

        bool drawing = false;
        bool attached = true;
    };

} // namespace antwika::gfx::raylib
