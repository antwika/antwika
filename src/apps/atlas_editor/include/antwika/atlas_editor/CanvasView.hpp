#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/atlas_editor/Pixel.hpp"

namespace antwika::atlas_editor
{

    using antwika::gfx::Point;
    using antwika::gfx::Rect;
    using antwika::gfx::Size;

    /**
     * @brief How many screen pixels one image pixel is drawn as, at each
     * zoom level.
     *
     * **Whole numbers rather than a scale factor**, for the reason
     * game::Camera holds whole tile sizes: which pixel a click lands on
     * is a function of this, so a replay resolving a recorded click has
     * to divide by exactly the same number the recording multiplied by.
     * An integer table cannot round differently on another toolchain,
     * and a float factor could.
     */
    inline constexpr std::array<std::uint32_t, 6> kZoomScales{
        1, 2, 3, 4, 6, 8};

    /**
     * @brief Where the image is on the canvas, and how big it is drawn.
     *
     * **This is simulation state, not render state**, in exactly the
     * sense apps/game's camera is: a click arrives as a screen position
     * and which pixel of the sheet it means depends entirely on this, so
     * a view the renderer owned would leave a replay resolving recorded
     * clicks against a different one.
     */
    struct CanvasView
    {
        /**
         * @brief Where the image's top-left pixel is drawn, in canvas
         * pixels.
         *
         * Signed, because zooming in on the middle of a sheet puts its
         * corner off the left of the window, which is ordinary rather
         * than an error.
         */
        Point pan{};

        /**
         * @brief Which of kZoomScales the image is drawn at.
         */
        std::size_t zoom = 0;

        /**
         * @brief Compare two views.
         * @param other The view to compare against.
         * @return True when the pan and the zoom level both match.
         */
        [[nodiscard]] bool operator==(const CanvasView &other) const =
            default;
    };

    /**
     * @brief Get how many screen pixels one image pixel occupies.
     * @param view The view to read.
     * @return The scale, never zero -- a view carrying a zoom level the
     * table has no entry for reads as the closest one it does.
     */
    [[nodiscard]] std::uint32_t scaleOf(CanvasView view) noexcept;

    /**
     * @brief Put an image in the middle of a canvas.
     * @param canvas The area being drawn into.
     * @param image How big the image is.
     * @param zoom Which of kZoomScales to draw it at.
     * @return The view, with its pan worked out from the two sizes.
     */
    [[nodiscard]] CanvasView centredView(
        Size canvas, Size image, std::size_t zoom) noexcept;

    /**
     * @brief Zoom one step in, keeping the pixel under a point put.
     *
     * Anchoring on the pointer rather than on the canvas's middle is
     * what makes a wheel usable at all: an artist zooms in on the tile
     * they are looking at, not on the tile that happens to be central.
     *
     * @param view The view to zoom.
     * @param anchor The canvas position to keep still.
     * @return The zoomed view, or the same one at the closest level.
     */
    [[nodiscard]] CanvasView zoomedIn(CanvasView view, Point anchor) noexcept;

    /**
     * @brief Zoom one step out, keeping the pixel under a point put.
     * @param view The view to zoom.
     * @param anchor The canvas position to keep still.
     * @return The zoomed view, or the same one at the widest level.
     */
    [[nodiscard]] CanvasView zoomedOut(
        CanvasView view, Point anchor) noexcept;

    /**
     * @brief Slide the image across the canvas.
     * @param view The view to move.
     * @param by How far to move it, in canvas pixels.
     * @return The moved view.
     */
    [[nodiscard]] CanvasView pannedBy(CanvasView view, Point by) noexcept;

    /**
     * @brief Work out which image pixel a canvas position falls on.
     *
     * Floored rather than truncated, so the pixel to the left of the
     * image's corner is -1 rather than a second 0 -- which is the whole
     * reason apps/game has a floorDiv() of its own.
     *
     * @param view Where the image is.
     * @param point The canvas position, which may be anywhere.
     * @return The pixel, which may lie outside the image; ask the Canvas
     * whether it holds it.
     */
    [[nodiscard]] Pixel pixelAt(CanvasView view, Point point) noexcept;

    /**
     * @brief Get the canvas area one image pixel is drawn into.
     * @param view Where the image is.
     * @param pixel The pixel to place; it need not be inside the image.
     * @return Its rectangle on the canvas.
     */
    [[nodiscard]] Rect pixelRect(CanvasView view, Pixel pixel) noexcept;

    /**
     * @brief Get the canvas area the whole image is drawn into.
     * @param view Where the image is.
     * @param image How big the image is.
     * @return Its rectangle on the canvas.
     */
    [[nodiscard]] Rect imageRect(CanvasView view, Size image) noexcept;

} // namespace antwika::atlas_editor
