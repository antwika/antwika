#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/gfx/Point.hpp>

namespace antwika::game
{

    using antwika::gfx::Point;

    /**
     * @brief The tile half-widths a camera may be zoomed to.
     *
     * A table of whole pixel counts rather than a scale factor, because
     * zoom feeds screenToCell(), which decides which cell a click meant,
     * which is simulation state. A float scale would probably agree across
     * GNU, LLVM and MinGW and between a recording and its replay; integers
     * agree provably.
     *
     * Half-height is always half of these, which is what makes the
     * projection the standard 2:1 isometric ratio at every level.
     */
    inline constexpr std::array<std::uint32_t, 5> kZoomHalfWidths{
        4, 8, 16, 32, 64};

    /**
     * @brief Which zoom level a camera starts at.
     */
    inline constexpr std::size_t kDefaultZoomLevel = 3;

    /**
     * @brief Where the grid is being looked at from, and how closely.
     *
     * **Simulation state, not render state**, which is the one surprising
     * thing about it. A click arrives as a pixel, and which cell that pixel
     * means depends entirely on this. A camera owned by the renderer would
     * leave a replay resolving recorded clicks against whatever camera it
     * happened to have, and putting the paths somewhere else -- still
     * deterministically, just deterministically wrong. So this is folded
     * from replayable input like any other state, and the renderer only
     * reads it.
     *
     * The pan is in screen pixels and is what the projection is anchored
     * to. Deliberately not the canvas centre: that would make the window's
     * size an input to screenToCell(), so a resize would change which cell
     * a pixel meant and would itself have to become replayable input.
     *
     * A plain comparable value, so a run's final camera can be asserted
     * equal between a live run and its replay.
     */
    class Camera final
    {
    public:
        /**
         * @brief Construct a camera at the default zoom.
         * @param pan Where the cell at the origin has its top corner.
         * @param zoomLevel Index into kZoomHalfWidths; clamped to it.
         */
        constexpr explicit Camera(
            Point pan = {}, std::size_t zoomLevel = kDefaultZoomLevel)
            : panOffset(pan), zoom(clampZoom(zoomLevel))
        {
        }

        /**
         * @brief Get the pan offset, in screen pixels.
         * @return The offset the projection is anchored at.
         */
        [[nodiscard]] constexpr Point pan() const noexcept
        {
            return panOffset;
        }

        /**
         * @brief Get the current zoom level.
         * @return An index into kZoomHalfWidths.
         */
        [[nodiscard]] constexpr std::size_t zoomLevel() const noexcept
        {
            return zoom;
        }

        /**
         * @brief Get half a tile's width at this zoom, in pixels.
         * @return The half-width; never zero.
         */
        [[nodiscard]] constexpr std::uint32_t halfWidth() const noexcept
        {
            return kZoomHalfWidths[zoom];
        }

        /**
         * @brief Get half a tile's height at this zoom, in pixels.
         * @return The half-height, always half the half-width.
         */
        [[nodiscard]] constexpr std::uint32_t halfHeight() const noexcept
        {
            return halfWidth() / 2;
        }

        /**
         * @brief Move the camera by a screen-pixel offset.
         * @param dx Pixels to add to the pan's x.
         * @param dy Pixels to add to the pan's y.
         */
        constexpr void panBy(std::int32_t dx, std::int32_t dy) noexcept
        {
            panOffset.x += dx;
            panOffset.y += dy;
        }

        /**
         * @brief Put the camera's anchor at an exact offset.
         *
         * Exists for zooming about a fixed point, which has to correct the
         * pan after changing the level -- see zoomedAt().
         *
         * @param pan The offset to anchor at.
         */
        constexpr void setPan(Point pan) noexcept
        {
            panOffset = pan;
        }

        /**
         * @brief Zoom in one level, stopping at the closest.
         */
        constexpr void zoomIn() noexcept
        {
            if (zoom + 1 < kZoomHalfWidths.size())
            {
                ++zoom;
            }
        }

        /**
         * @brief Zoom out one level, stopping at the furthest.
         */
        constexpr void zoomOut() noexcept
        {
            if (zoom > 0)
            {
                --zoom;
            }
        }

        /**
         * @brief Compare two cameras.
         * @param other The camera to compare against.
         * @return True when both the pan and the zoom level match.
         */
        [[nodiscard]] bool operator==(const Camera &other) const = default;

    private:
        [[nodiscard]] static constexpr std::size_t clampZoom(
            std::size_t level) noexcept
        {
            return level < kZoomHalfWidths.size()
                       ? level
                       : kZoomHalfWidths.size() - 1;
        }

        Point panOffset;
        std::size_t zoom;
    };

} // namespace antwika::game
