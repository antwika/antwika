#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/gfx/Point.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/IsoProjection.hpp"

namespace antwika::game
{

    using antwika::gfx::Point;

    inline constexpr std::size_t kOutlineCorners = 4;

    [[nodiscard]] constexpr std::array<Point, kOutlineCorners>
    footprintOutline(
        Cell origin, Footprint footprint, const Camera &camera) noexcept
    {
        const auto box = footprintBounds(origin, footprint, camera);

        const auto width = static_cast<std::int32_t>(box.size.width);
        const auto height = static_cast<std::int32_t>(box.size.height);

        const auto middle = Point{
            .x = box.origin.x + width / 2, .y = box.origin.y + height / 2};

        return {
            Point{.x = middle.x, .y = box.origin.y},
            Point{.x = box.origin.x + width - 1, .y = middle.y},
            Point{.x = middle.x, .y = box.origin.y + height - 1},
            Point{.x = box.origin.x, .y = middle.y}};
    }

    static_assert(
        footprintOutline(Cell{.x = 3, .y = 4}, Footprint{2, 2}, Camera())[0]
        == cellToScreen(Cell{.x = 3, .y = 4}, Camera()));

    static_assert(
        footprintOutline(Cell{}, Footprint{1, 1}, Camera())[0]
        == cellToScreen(Cell{}, Camera()));

}
