#pragma once

#include <cstdint>
#include <map>

#include <antwika/ecs/World.hpp>
#include <antwika/gfx/Color.hpp>

#include "antwika/game/Cell.hpp"
#include "antwika/game/Desirability.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/MapView.hpp"

namespace antwika::game
{

    using antwika::ecs::World;
    using antwika::gfx::Color;

    /**
     * @brief How strongly one view paints each cell, as a percentage.
     *
     * **A cell with no entry and a cell entered as zero mean the same
     * thing, so only the first exists** -- DesirabilityField's rule, and
     * it is a std::map for that field's reasons too: what a view paints
     * is sparse against the whole extent, and a map is already ordered
     * by Cell so anything walking it walks a total order.
     *
     * A percentage rather than each view's own units, because the one
     * thing a scene does with this is choose how strong a tint to blit.
     * Turning ticks of coverage, units of food and a desirability into
     * one scale is exactly the crossing this type exists to be.
     */
    using OverlayField = std::map<Cell, std::int32_t>;

    /**
     * @brief Work out what one view paints over the city.
     *
     * **Two shapes, and which one a view has is what the two tables in
     * MapView.hpp say.** Desirability is genuinely per cell and is read
     * straight off the field the serve phase rebuilt. Everything else is
     * a fact about a *building* -- how much food is on its shelves, how
     * much longer a service still reaches it -- and is painted over
     * every cell of that building's block, because a block is the
     * smallest thing any of those numbers is true of.
     *
     * The normal view paints nothing at all, which is why it is the one
     * that comes out empty rather than the one with a branch of its own.
     *
     * @param world Read for the buildings, as of its last commit().
     * @param view Which picture is being looked at.
     * @param desirability The field the serve phase rebuilt this tick.
     * @param extent The bounds to keep the field inside.
     * @return One entry per cell with something to say, as a percentage
     * clamped to 0..100.
     */
    [[nodiscard]] OverlayField overlayFieldOf(
        const World &world,
        MapView view,
        const DesirabilityField &desirability,
        GridExtent extent);

    /**
     * @brief Get the colour a view paints in, at full strength.
     *
     * One per view, and the four service views take serviceColour()'s
     * answer rather than a second one of their own -- so a coverage
     * line in the hover panel and the map painted from the same
     * coverage cannot come out in two different blues.
     *
     * @param view The view to colour; Normal answers a colour nothing
     * ever paints with, since its field is empty.
     * @return Its colour, opaque.
     */
    [[nodiscard]] Color overlayColour(MapView view) noexcept;

    /**
     * @brief What an overlay lays over the whole city behind it.
     *
     * Dark and see-through, so the roads and the blocks still read as
     * a city while the numbers on top of them are what stands out.
     * Painted over every cell rather than only the ones with a value,
     * since a district nothing reaches is exactly what a player looks at
     * one of these to find.
     */
    inline constexpr Color kOverlayScrim{
        .red = 8, .green = 10, .blue = 16, .alpha = 175};

    /**
     * @brief The faintest an overlay's own colour is ever painted.
     *
     * A cell that only just registers still has to be visible against
     * the scrim, or the bottom of every scale reads as nothing at all.
     */
    inline constexpr std::uint8_t kOverlayFaintest = 60;

} // namespace antwika::game
