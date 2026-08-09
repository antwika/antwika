#pragma once

#include <cstdint>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Footprint.hpp"

namespace antwika::game
{

    using antwika::gfx::Point;
    using antwika::gfx::Rect;
    using antwika::gfx::Size;

    [[nodiscard]] constexpr std::int64_t floorDiv(
        std::int64_t numerator, std::int64_t denominator) noexcept
    {
        const auto quotient = numerator / denominator;
        const auto remainder = numerator % denominator;

        if (remainder != 0 && ((remainder < 0) != (denominator < 0)))
        {
            return quotient - 1;
        }

        return quotient;
    }

    [[nodiscard]] constexpr Point cellToScreen(
        Cell cell, const Camera &camera) noexcept
    {
        const auto halfWidth = static_cast<std::int32_t>(camera.halfWidth());
        const auto halfHeight =
            static_cast<std::int32_t>(camera.halfHeight());

        return Point{
            .x = (cell.x - cell.y) * halfWidth + camera.pan().x,
            .y = (cell.x + cell.y) * halfHeight + camera.pan().y};
    }

    [[nodiscard]] constexpr Point cellCentre(
        Cell cell, const Camera &camera) noexcept
    {
        const auto top = cellToScreen(cell, camera);

        return Point{
            .x = top.x,
            .y = top.y + static_cast<std::int32_t>(camera.halfHeight())};
    }

    [[nodiscard]] constexpr Cell screenToCell(
        Point screen, const Camera &camera) noexcept
    {
        const std::int64_t halfWidth = camera.halfWidth();
        const std::int64_t halfHeight = camera.halfHeight();

        const std::int64_t u = screen.x - camera.pan().x;
        const std::int64_t v = screen.y - camera.pan().y;

        const auto divisor = 2 * halfWidth * halfHeight;

        return Cell{
            .x = static_cast<std::int32_t>(
                floorDiv(u * halfHeight + v * halfWidth, divisor)),
            .y = static_cast<std::int32_t>(
                floorDiv(v * halfWidth - u * halfHeight, divisor))};
    }

    [[nodiscard]] constexpr Size tileSize(const Camera &camera) noexcept
    {
        return Size{
            .width = 2 * camera.halfWidth(),
            .height = 2 * camera.halfHeight()};
    }

    [[nodiscard]] constexpr Rect cellBounds(
        Cell cell, const Camera &camera) noexcept
    {
        const auto top = cellToScreen(cell, camera);

        return Rect{
            .origin =
                {.x = top.x - static_cast<std::int32_t>(camera.halfWidth()),
                 .y = top.y},
            .size = tileSize(camera)};
    }

    [[nodiscard]] constexpr Rect footprintBounds(
        Cell origin, Footprint footprint, const Camera &camera) noexcept
    {
        const auto top = cellToScreen(origin, camera);
        const auto half = static_cast<std::int32_t>(camera.halfWidth());
        const auto cells = footprint.width + footprint.height;

        return Rect{
            .origin = {.x = top.x - footprint.height * half, .y = top.y},
            .size = {
                .width = static_cast<std::uint32_t>(cells)
                    * camera.halfWidth(),
                .height = static_cast<std::uint32_t>(cells)
                    * camera.halfHeight()}};
    }

    static_assert(
        footprintBounds(Cell{.x = 3, .y = 4}, Footprint{1, 1}, Camera())
        == cellBounds(Cell{.x = 3, .y = 4}, Camera()));

    [[nodiscard]] constexpr Camera zoomedAt(
        Camera camera, Point cursor, std::int32_t notches) noexcept
    {
        const auto anchor = screenToCell(cursor, camera);
        const auto before = cellToScreen(anchor, camera);
        const std::int64_t wasHalfWidth = camera.halfWidth();

        for (std::int32_t step = 0; step < notches; ++step)
        {
            camera.zoomIn();
        }

        for (std::int32_t step = 0; step > notches; --step)
        {
            camera.zoomOut();
        }

        const std::int64_t nowHalfWidth = camera.halfWidth();
        const auto after = cellToScreen(anchor, camera);

        const auto intoX =
            floorDiv((cursor.x - before.x) * nowHalfWidth, wasHalfWidth);
        const auto intoY =
            floorDiv((cursor.y - before.y) * nowHalfWidth, wasHalfWidth);

        const auto wanted = Point{
            .x = static_cast<std::int32_t>(after.x + intoX),
            .y = static_cast<std::int32_t>(after.y + intoY)};

        camera.setPan(
            Point{
                .x = camera.pan().x + cursor.x - wanted.x,
                .y = camera.pan().y + cursor.y - wanted.y});

        return camera;
    }

}
