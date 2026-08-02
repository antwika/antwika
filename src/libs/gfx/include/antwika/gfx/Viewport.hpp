#pragma once

#include <cstdint>

#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/Rect.hpp"
#include "antwika/gfx/Size.hpp"

namespace antwika::gfx
{

    /**
     * @brief Where a fixed-size canvas is blitted inside a window, and
     * how much it is enlarged getting there.
     *
     * The generalisation of the offset docs/resizable-windows.md already
     * sanctions, from a translation to a translation and a uniform
     * scale. Everything an application lays out, hit-tests and simulates
     * stays a function of the canvas -- a number the application chose,
     * and therefore the same on the machine that recorded a session and
     * on the one replaying it. This is applied afterwards, to place that
     * picture inside a drawable area of whatever size the window system
     * happens to report.
     *
     * **The scale is a ratio of two integers rather than a float**, and
     * every coordinate it produces is worked out with integer
     * arithmetic. Rendering may use floating point freely, since nothing
     * drawn is read back -- but toCanvas() runs the transform backwards
     * on a pointer position, and that position is recorded input. A
     * value whose last bit differed between two toolchains would cost a
     * divergent session rather than a misplaced pixel, which is the same
     * argument AtlasText.hpp makes for the font's metrics.
     *
     * A default-constructed Viewport is the identity: nothing is moved
     * and nothing is resized, which is what a window reporting exactly
     * the size it was asked for -- every headless run, under the null
     * backend -- must produce.
     */
    struct Viewport
    {
        /** @brief Where the canvas's top-left corner lands. */
        Point offset{};

        /** @brief The scale's numerator; never zero. */
        std::uint32_t numerator = 1;

        /** @brief The scale's denominator; never zero. */
        std::uint32_t denominator = 1;

        /**
         * @brief Compare two viewports.
         * @param other The viewport to compare against.
         * @return True when offset and both terms of the scale match.
         */
        [[nodiscard]] bool operator==(const Viewport &other) const = default;

        /**
         * @brief Place a point on the canvas inside the window.
         * @param point Where it is on the canvas.
         * @return Where it is drawn.
         */
        [[nodiscard]] Point toWindow(Point point) const noexcept;

        /**
         * @brief Place a rectangle on the canvas inside the window.
         *
         * The far corner is transformed rather than the size scaled, so
         * two rectangles sharing an edge on the canvas go on sharing it
         * afterwards; scaling each size on its own leaves a seam of
         * unpainted pixels wherever the division rounded down.
         *
         * @param rect Where it is on the canvas.
         * @return Where it is drawn.
         */
        [[nodiscard]] Rect toWindow(Rect rect) const noexcept;

        /**
         * @brief Read a point in the window as a point on the canvas.
         *
         * The inverse of toWindow(), and the one direction of this
         * transform whose result may reach a decision -- which is why
         * every step of it is integer. A point in the letterboxed
         * remainder answers with a canvas coordinate outside the canvas,
         * which is exactly what it means: the pointer is not over the
         * picture.
         *
         * @param point Where it is in the window.
         * @return Where it is on the canvas.
         */
        [[nodiscard]] Point toCanvas(Point point) const noexcept;

        /**
         * @brief Scale a glyph scale, in whole pixels per glyph pixel.
         *
         * Never below one for text that was going to be drawn at all,
         * because IRenderer::drawText() draws nothing at a scale of
         * zero: a window half the height of the canvas would otherwise
         * lose every label on it rather than merely showing small ones.
         * A caller that passed zero still gets zero, since that caller
         * had already asked for nothing to be drawn.
         *
         * @param scale The scale the caller asked for.
         * @return The scale to draw at.
         */
        [[nodiscard]] std::uint32_t toWindowScale(
            std::uint32_t scale) const noexcept;

        /**
         * @brief Get the rectangle a canvas of this size is drawn into.
         * @param canvas The canvas being placed.
         * @return Its box in window pixels.
         */
        [[nodiscard]] Rect frame(Size canvas) const noexcept;
    };

    /**
     * @brief Work out where a canvas goes inside a drawable area.
     *
     * **The height is what drives the scale**, so a window's width
     * decides how much of it is bar and never how big the picture is: a
     * narrow monitor and a wide one of the same height draw the game
     * equally tall, where a scale taken from the width would make the
     * wide one enormous and the narrow one tiny.
     *
     * The width caps it in the one case where honouring the height would
     * push the canvas past the window's edges, since a toolbar drawn off
     * screen is a toolbar nobody can click and no pointer mapping can
     * give it back. The aspect ratio is fixed either way and the
     * remainder is left as bars, so every anchor on the canvas -- an
     * edge, a corner, a centre -- stays a function of the canvas.
     *
     * Widening the canvas with the window instead was considered and
     * rejected where this is documented: it would make a layout a
     * function of the reported size, which is the escape
     * docs/resizable-windows.md already refused.
     *
     * @param reported What IWindow::size() answers.
     * @param canvas The fixed size everything was laid out against.
     * @return The transform to draw through, or the identity when
     * either size is degenerate.
     */
    [[nodiscard]] Viewport viewportFor(Size reported, Size canvas) noexcept;

} // namespace antwika::gfx
