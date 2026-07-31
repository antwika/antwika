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

    /**
     * @brief How many corners a block's outline has.
     *
     * Four, and that is a fact about the projection rather than a
     * simplification: a footprint is square, a cell's box is 2:1 because
     * halfHeight is halfWidth / 2, so a square block's silhouette is one
     * big diamond -- see Footprint.hpp.
     * A 2x3 block would be a hexagon and would need six.
     */
    inline constexpr std::size_t kOutlineCorners = 4;

    /**
     * @brief Trace the outline of a block on screen.
     *
     * **Derived from footprintBounds() rather than worked out afresh**,
     * so the outline and the tile blitted into that box cannot show two
     * different extents -- which is the whole reason to draw a border at
     * all.
     *
     * The corners are inclusive: the east one is the box's last pixel
     * column rather than one past it, so the outline lands *on* the
     * block rather than a pixel outside it. An even-sided box has no
     * exact middle column, so the north and south corners sit at
     * width / 2, which is where a diamond tile's own apex is drawn.
     *
     * @param origin The minimum-x, minimum-y cell of the block.
     * @param footprint How many cells across and down it covers; square,
     * or the four points below are not the shape's corners.
     * @param camera Supplies the zoom and the pan.
     * @return The north, east, south and west corners, in that order --
     * a closed loop, so joining each to the next and the last to the
     * first draws the whole outline.
     */
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

    // The block's top corner is the origin cell's top corner.
    // That holds because the footprint is square, and only then.
    // Stating it here is what catches a footprint that stops being one.
    static_assert(
        footprintOutline(Cell{.x = 3, .y = 4}, Footprint{2, 2}, Camera())[0]
        == cellToScreen(Cell{.x = 3, .y = 4}, Camera()));

    // One cell is a footprint of one, so the two answers must agree.
    static_assert(
        footprintOutline(Cell{}, Footprint{1, 1}, Camera())[0]
        == cellToScreen(Cell{}, Camera()));

} // namespace antwika::game
