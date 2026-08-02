#pragma once

#include <vector>

#include <antwika/animation/Progress.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/SceneSnapshot.hpp"
#include "antwika/game/Service.hpp"

namespace antwika::game
{

    using antwika::animation::Progress;
    using antwika::gfx::Color;
    using antwika::gfx::Rect;

    /**
     * @brief One small vertical bar: a track, and how full it is.
     *
     * A value rather than a pair of drawing calls, so what a bar comes
     * out as can be asserted with EXPECT_EQ instead of being looked at
     * -- the same reason ui::Frame is a value.
     *
     * The fill rises from the track's bottom edge, which is why it is a
     * rectangle of its own rather than a fraction the drawing code would
     * have to turn into one twice.
     * An empty bar's fill has zero height and is not drawn.
     */
    struct ResourceBar
    {
        /** @brief Which resource this bar counts. */
        Resource resource = Resource::Food;

        /** @brief The whole bar, drawn in kBarTrack. */
        Rect track;

        /** @brief The part of it that is full, in resourceColour(). */
        Rect fill;

        /**
         * @brief Compare two bars.
         * @param other The bar to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const ResourceBar &other) const
            = default;
    };

    /**
     * @brief What an empty bar is drawn in.
     *
     * Dark and see-through, so a bar over a tile reads as a gauge on top
     * of the art rather than as a hole cut in it.
     */
    inline constexpr Color kBarTrack{
        .red = 12, .green = 14, .blue = 18, .alpha = 170};

    /**
     * @brief Get the colour a resource's fill is drawn in.
     *
     * The one crossing between a resource and a colour, so the bar over
     * a house and the line in the hover panel cannot come out in two
     * different greens.
     *
     * It lives here rather than in Resource.hpp because that header is
     * the simulation's vocabulary and a colour is a fact about the
     * picture; a gfx type in there would put a render concern into the
     * component every building carries.
     *
     * @param resource The resource to colour.
     * @return Its colour, opaque.
     */
    [[nodiscard]] Color resourceColour(Resource resource) noexcept;

    /**
     * @brief Get the colour a service is written in.
     *
     * resourceColour()'s counterpart, here for its reason: the one
     * crossing between a service and a colour, so a coverage line and
     * whatever else ever gauges the same service cannot come out in two
     * different blues.
     *
     * @param service The service to colour.
     * @return Its colour, opaque.
     */
    [[nodiscard]] Color serviceColour(Service service) noexcept;

    /**
     * @brief Get the bars to draw over one building.
     *
     * One per resource the building **depends on**, in Resource order,
     * which is every resource for a house and none for anything else --
     * consumes() is the one statement of that, and a source that keeps
     * stock nobody drains has nothing a gauge could say.
     *
     * Placed from buildingSpriteBounds(), the very box the building's
     * own art is blitted into, so the bars cannot become a second
     * layout that drifts from the first; they sit immediately above the
     * sprite's box, centred on it, which is clear of the art at every
     * footprint and every zoom -- headroom included, since the box is
     * the whole sprite rather than the block's diamond.
     *
     * @param building The building to gauge.
     * @param camera Supplies the zoom and the pan.
     * @return Its bars, left to right; empty for a building that
     * depends on nothing.
     */
    [[nodiscard]] std::vector<ResourceBar> buildingBars(
        const BuildingSprite &building, const Camera &camera);

    /**
     * @brief Get the bars to draw over one walker.
     *
     * One for the resource its kind carries, whatever is left of it --
     * an empty bar over a food walker says it has handed everything out,
     * which is worth seeing -- and none at all for a fireman or an
     * architect, who carry nothing.
     *
     * Placed from walkerBounds(), which is where the walker itself is
     * drawn this frame rather than the cell it belongs to, so a bar
     * slides across a cell with the walker under it.
     *
     * @param walker The walker to gauge.
     * @param camera Supplies the zoom and the pan.
     * @param subTick How far through the tick this frame falls.
     * @return Its bar, or nothing for a walker that carries nothing.
     */
    [[nodiscard]] std::vector<ResourceBar> walkerBars(
        const WalkerSprite &walker,
        const Camera &camera,
        Progress subTick);

} // namespace antwika::game
