#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/game/Cell.hpp"

namespace antwika::game
{

    using antwika::gfx::Point;
    using antwika::gfx::Rect;
    using antwika::gfx::Size;

    /**
     * @brief How wide and tall one world-map tile is drawn, in pixels.
     *
     * A whole number of pixels rather than a scale factor, for the
     * reason Camera's zoom table gives: this number decides which tile
     * a click landed on, which is simulation state, and integers agree
     * across compilers provably rather than probably.
     *
     * The world map is drawn square rather than isometric, so that the
     * two views are told apart at a glance and so that "which tile did
     * I click" is a division rather than a projection to invert.
     */
    inline constexpr std::int32_t kWorldTileSize = 24;

    /**
     * @brief Where the world map's top-left corner sits on a canvas.
     *
     * The single place the pixels-to-tiles mapping lives, shared by
     * WorldMapScene and WorldMapSink exactly as life::layoutFor() is
     * shared by its scene and its sink -- so where a tile is drawn and
     * which tile a click lands on cannot drift apart.
     *
     * The canvas handed in must be the size the window was *asked*
     * for, never the size a window reports: which city a recorded
     * click selects must not depend on how a window manager sized a
     * window on the day.
     *
     * @param canvas The area the map is centred in.
     * @param width The map's width in tiles.
     * @param height The map's height in tiles.
     * @return The top-left corner to lay the map out from.
     */
    [[nodiscard]] constexpr Point worldOriginFor(
        Size canvas, std::uint32_t width, std::uint32_t height) noexcept
    {
        const std::int32_t spanX =
            static_cast<std::int32_t>(width) * kWorldTileSize;
        const std::int32_t spanY =
            static_cast<std::int32_t>(height) * kWorldTileSize;
        return Point{
            (static_cast<std::int32_t>(canvas.width) - spanX) / 2,
            (static_cast<std::int32_t>(canvas.height) - spanY) / 2};
    }

    /**
     * @brief Get the rectangle one tile occupies.
     * @param canvas The area the map is centred in.
     * @param width The map's width in tiles.
     * @param height The map's height in tiles.
     * @param cell The tile to place.
     * @return Where that tile is drawn.
     */
    [[nodiscard]] constexpr Rect worldTileRect(
        Size canvas,
        std::uint32_t width,
        std::uint32_t height,
        Cell cell) noexcept
    {
        const Point origin = worldOriginFor(canvas, width, height);
        return Rect{
            Point{
                origin.x + cell.x * kWorldTileSize,
                origin.y + cell.y * kWorldTileSize},
            Size{
                static_cast<std::uint32_t>(kWorldTileSize),
                static_cast<std::uint32_t>(kWorldTileSize)}};
    }

    /**
     * @brief Work out which tile a pixel is over.
     *
     * A pixel left of or above the map's origin is rejected before
     * the division rather than divided, because integer division
     * truncates towards zero: -1 / 24 is 0, which would fold the row
     * just outside the map onto the first row inside it.
     *
     * @param canvas The area the map is centred in.
     * @param width The map's width in tiles.
     * @param height The map's height in tiles.
     * @param point The pixel to resolve.
     * @return The tile there, or nullopt when the pixel is off the
     * map.
     */
    [[nodiscard]] constexpr std::optional<Cell> worldCellAt(
        Size canvas,
        std::uint32_t width,
        std::uint32_t height,
        Point point) noexcept
    {
        const Point origin = worldOriginFor(canvas, width, height);
        const std::int32_t offsetX = point.x - origin.x;
        const std::int32_t offsetY = point.y - origin.y;
        if (offsetX < 0 || offsetY < 0)
        {
            return std::nullopt;
        }

        const std::int32_t x = offsetX / kWorldTileSize;
        const std::int32_t y = offsetY / kWorldTileSize;
        if (x >= static_cast<std::int32_t>(width)
            || y >= static_cast<std::int32_t>(height))
        {
            return std::nullopt;
        }
        return Cell{x, y};
    }

} // namespace antwika::game
