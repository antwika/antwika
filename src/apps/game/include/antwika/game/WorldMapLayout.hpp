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

    inline constexpr std::int32_t kWorldTileSize = 24;

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

}
