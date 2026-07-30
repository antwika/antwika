#pragma once

#include <cstdint>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"

namespace antwika::game
{

    using antwika::gfx::Point;
    using antwika::gfx::Rect;
    using antwika::gfx::Size;

    /**
     * @brief Divide, rounding toward negative infinity.
     *
     * **Not operator/.** C++ integer division truncates toward zero, so
     * -1 / 4 is 0 where the floor is -1. The grid reaches negative screen
     * coordinates as soon as the camera pans, and truncation there makes
     * the cells straddling each axis twice as wide as every other cell --
     * a bug invisible until somebody pans past the origin, which is to say
     * not in the first test anybody writes.
     *
     * @param numerator The value to divide.
     * @param denominator The value to divide by; must not be zero.
     * @return The quotient, rounded down.
     */
    [[nodiscard]] constexpr std::int64_t floorDiv(
        std::int64_t numerator, std::int64_t denominator) noexcept
    {
        const auto quotient = numerator / denominator;
        const auto remainder = numerator % denominator;

        // Truncation and flooring differ only on a remainder.
        // And then only when the signs disagree.
        if (remainder != 0 && ((remainder < 0) != (denominator < 0)))
        {
            return quotient - 1;
        }

        return quotient;
    }

    /**
     * @brief Get where a cell's diamond has its **top** corner on screen.
     *
     * The top corner rather than the centre, because that is the point
     * screenToCell() inverts exactly: the region of the screen belonging to
     * a cell is the diamond hanging below this point, so the round trip
     * screenToCell(cellToScreen(c)) == c holds for every cell at every
     * zoom. cellCentre() is there for drawing.
     *
     * @param cell The cell to place.
     * @param camera Supplies the zoom and the pan.
     * @return The diamond's top corner, in screen pixels.
     */
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

    /**
     * @brief Get the centre of a cell's diamond on screen.
     * @param cell The cell to place.
     * @param camera Supplies the zoom and the pan.
     * @return The diamond's centre, in screen pixels.
     */
    [[nodiscard]] constexpr Point cellCentre(
        Cell cell, const Camera &camera) noexcept
    {
        const auto top = cellToScreen(cell, camera);

        return Point{
            .x = top.x,
            .y = top.y + static_cast<std::int32_t>(camera.halfHeight())};
    }

    /**
     * @brief Get the cell whose diamond a screen pixel falls in.
     *
     * Exact, and integer throughout: the two equations behind
     * cellToScreen() are multiplied through by the half-height and
     * half-width before dividing, so there is one division at the end
     * rather than two roundings on the way. Intermediates are 64-bit, so
     * no pan a session can reach overflows them.
     *
     * @param screen The pixel to place.
     * @param camera Supplies the zoom and the pan.
     * @return The cell containing that pixel. Coordinates may be negative,
     * and may fall outside any GridExtent -- testing that is the caller's
     * job.
     */
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

    /**
     * @brief Get the size of one tile's bounding box at this zoom.
     * @param camera Supplies the zoom.
     * @return Twice the half-width by twice the half-height.
     */
    [[nodiscard]] constexpr Size tileSize(const Camera &camera) noexcept
    {
        return Size{
            .width = 2 * camera.halfWidth(),
            .height = 2 * camera.halfHeight()};
    }

    /**
     * @brief Get the bounding box of a cell's diamond on screen.
     *
     * What culling tests against, so a cell wholly off the canvas costs
     * nothing but this.
     *
     * @param cell The cell to bound.
     * @param camera Supplies the zoom and the pan.
     * @return The box enclosing the diamond.
     */
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

    /**
     * @brief Zoom a camera by whole levels, keeping one pixel's cell put.
     *
     * Zooming has to be anchored to something, and this is the only anchor
     * available: anchoring to the canvas centre would put the window's size
     * back into screenToCell(), which is exactly what
     * Camera's pan is arranged to avoid.
     *
     * Anchored to the nearest cell rather than the exact pixel, since a
     * pixel is not a grid position and pretending otherwise would need
     * fractional pan.
     *
     * @param camera The camera to zoom; taken by value and returned.
     * @param cursor The pixel to keep pointing at the same cell.
     * @param notches Levels to zoom in by; negative zooms out, zero
     * returns the camera unchanged.
     * @return The zoomed camera.
     */
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

        // How far into its cell the cursor was, rescaled to the new tile.
        // Correcting to the cell's corner would snap the pan instead.
        // That would move the camera even at zero notches.
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

} // namespace antwika::game
