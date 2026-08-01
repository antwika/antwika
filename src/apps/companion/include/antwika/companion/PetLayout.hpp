#pragma once

#include <cstdint>
#include <optional>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

namespace antwika::companion
{

    using antwika::gfx::Point;
    using antwika::gfx::Rect;
    using antwika::gfx::Size;

    /**
     * @brief How many whole units a side the picture is laid out on.
     *
     * A window whose sides are a whole multiple of this gives every
     * rectangle a whole number of pixels, which is why main.cpp derives
     * its window size from this constant rather than naming a pixel
     * count that happens to divide by it.
     * Doubling the window therefore doubles a unit and everything drawn
     * from one, the readout's glyphs included, and leaves nothing to
     * keep in step by hand.
     *
     * Whole units rather than fractions of the canvas is what keeps
     * every rectangle the same integer on every toolchain.
     */
    inline constexpr std::uint32_t kSceneUnits = 32;

    /**
     * @brief The grid everything is drawn on: how big a unit is, and
     * where the grid's top-left corner sits in the canvas.
     */
    struct SceneLayout
    {
        std::uint32_t unit = 0;
        Point origin{};

        /**
         * @brief Compare two layouts.
         * @param other The layout to compare against.
         * @return True when the unit and the origin both match.
         */
        [[nodiscard]] bool operator==(const SceneLayout &other) const
            = default;
    };

    /**
     * @brief Work out the grid for a canvas.
     *
     * A square grid of kSceneUnits whole units, centred, so a canvas
     * that is not square leaves a margin rather than stretching
     * anything.
     *
     * @param canvas The size being drawn into.
     * @return The grid, or nothing when the canvas is too small to give
     * a unit even one pixel -- in which case there is nothing to draw
     * and nothing to hit.
     */
    [[nodiscard]] std::optional<SceneLayout> layoutFor(Size canvas);

    /**
     * @brief Find where a point of the grid falls in the canvas.
     * @param layout The grid.
     * @param x The unit column.
     * @param y The unit row.
     * @return The pixel.
     */
    [[nodiscard]] Point point(
        const SceneLayout &layout, std::int32_t x, std::int32_t y);

    /**
     * @brief Find where a box of the grid falls in the canvas.
     * @param layout The grid.
     * @param x The unit column of its left edge.
     * @param y The unit row of its top edge.
     * @param width How many units wide it is.
     * @param height How many units high it is.
     * @return The rectangle.
     */
    [[nodiscard]] Rect box(
        const SceneLayout &layout,
        std::int32_t x,
        std::int32_t y,
        std::uint32_t width,
        std::uint32_t height);

    /**
     * @brief Find the "new companion" button on a grid already worked
     * out.
     *
     * The overload whatever is drawing calls, since it has the grid in
     * hand and asking for the button by canvas again would be a second
     * "is this canvas big enough?" that its own answer already settled
     * -- a branch no input could take both ways.
     *
     * @param layout The grid.
     * @return The button.
     */
    [[nodiscard]] Rect reviveButtonBox(const SceneLayout &layout);

    /**
     * @brief Find the "new companion" button.
     *
     * **This is the one function shared by what is drawn and what is
     * hit**, in life::BoardLayout's sense and for its reason: PetScene
     * paints this rectangle and ReviveSink tests a press against this
     * rectangle, so a button somebody can see and a button somebody can
     * press cannot drift apart. Working the same box out twice is
     * exactly the drift apps/poker's card art once had.
     *
     * It is laid out against the *configured* canvas rather than the
     * size a window reports, for the reason life::PointerToggleSink
     * gives about cells: a hit-test is a function of the layout, and a
     * layout against a resized window would resolve a recorded press to
     * a different answer on the machine replaying it.
     *
     * Where it sits is decided here rather than by whichever of the two
     * asked first: left of the grave, under the gauges and above the
     * readout, so it covers nothing that says anything.
     *
     * @param canvas The size the picture is laid out against.
     * @return The button, or nothing when the canvas is too small for a
     * grid at all -- a window that cannot draw the button has no button
     * to press.
     */
    [[nodiscard]] std::optional<Rect> reviveButtonRect(Size canvas);

    /**
     * @brief Check whether a press landed on the "new companion"
     * button.
     *
     * Half-open in both axes -- the top-left pixel is inside and the
     * bottom-right one is not -- so two boxes sharing an edge cannot
     * both claim the pixel on it.
     *
     * @param canvas The size the picture is laid out against.
     * @param at Where the press landed.
     * @return Whether that pixel is the button's.
     */
    [[nodiscard]] bool withinReviveButton(Size canvas, Point at);

} // namespace antwika::companion
