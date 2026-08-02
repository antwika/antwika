#pragma once

#include <cstdint>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/TileAtlas.hpp"

namespace antwika::game
{

    /**
     * @brief Half of kIsoTileSize's width, the zoom the art is drawn at.
     *
     * The camera's halfWidth() over this is the scale every sprite is
     * blitted at, so kZoomHalfWidths' entry equal to it is the level
     * where one art pixel is one screen pixel.
     */
    inline constexpr std::int32_t kBaseHalfWidth =
        static_cast<std::int32_t>(kIsoTileSize.width) / 2;

    // Every sprite metric must scale to a whole pixel at every zoom.
    // The furthest level divides every metric by four.
    // A stray odd margin would round and shear the art off its diamond.
    static_assert(
        []
        {
            for (const auto halfWidth : kZoomHalfWidths)
            {
                for (const auto &spec : kAtlasSpecs)
                {
                    const auto exact = [halfWidth](std::int32_t value)
                    {
                        return (value * static_cast<std::int32_t>(halfWidth))
                                   % kBaseHalfWidth
                            == 0;
                    };

                    if (!exact(static_cast<std::int32_t>(
                            spec.spriteSize.width))
                        || !exact(static_cast<std::int32_t>(
                            spec.spriteSize.height))
                        || !exact(spec.pivot.x) || !exact(spec.pivot.y))
                    {
                        return false;
                    }
                }
            }

            return true;
        }(),
        "every sprite metric must scale exactly at every zoom level");

    /**
     * @brief Scale one art-pixel count to this camera's zoom.
     *
     * Multiply before dividing, so the fractional zoom levels stay
     * exact; the static_assert above is what guarantees the division
     * leaves no remainder.
     *
     * @param value The count in art pixels.
     * @param camera Supplies the zoom.
     * @return The count in screen pixels.
     */
    [[nodiscard]] constexpr std::int32_t scaledToZoom(
        std::int32_t value, const Camera &camera) noexcept
    {
        return value * static_cast<std::int32_t>(camera.halfWidth())
            / kBaseHalfWidth;
    }

    /**
     * @brief Get where a block's diamond has its bottom corner on screen.
     *
     * The point a sprite's pivot is anchored to.  The bottom corner
     * rather than the top one, because a sprite's height varies with its
     * art while what it stands on does not: every sheet states where its
     * footprint diamond bottoms out, and anchoring there is what keeps a
     * tall sprite's headroom above the block and its skirt below it.
     *
     * @param origin The minimum-x, minimum-y cell of the block.
     * @param footprint How many cells across and down it covers.
     * @param camera Supplies the zoom and the pan.
     * @return The block diamond's bottom corner, in screen pixels.
     */
    [[nodiscard]] constexpr Point blockAnchor(
        Cell origin, Footprint footprint, const Camera &camera) noexcept
    {
        const auto top = cellToScreen(origin, camera);
        const auto cells = footprint.width + footprint.height;

        return Point{
            .x = top.x
                + (footprint.width - footprint.height)
                    * static_cast<std::int32_t>(camera.halfWidth()),
            .y = top.y
                + cells * static_cast<std::int32_t>(camera.halfHeight())};
    }

    /**
     * @brief Get the box a sheet's sprite is blitted into at an anchor.
     *
     * The whole sprite cell rather than its diamond: the art may rise
     * into its headroom and hang its skirt below the anchor, and the
     * source rectangle is the whole cell, so the box has to be too.
     *
     * @param kind The sheet the sprite is from.
     * @param anchor Where the sprite's pivot goes, in screen pixels.
     * @param camera Supplies the zoom.
     * @return The box to blit the sprite into.
     */
    [[nodiscard]] constexpr Rect spriteBounds(
        AtlasKind kind, Point anchor, const Camera &camera) noexcept
    {
        const auto spec = atlasSpec(kind);

        return Rect{
            .origin =
                {.x = anchor.x - scaledToZoom(spec.pivot.x, camera),
                 .y = anchor.y - scaledToZoom(spec.pivot.y, camera)},
            .size = {
                .width = static_cast<std::uint32_t>(
                    scaledToZoom(
                        static_cast<std::int32_t>(spec.spriteSize.width),
                        camera)),
                .height = static_cast<std::uint32_t>(
                    scaledToZoom(
                        static_cast<std::int32_t>(spec.spriteSize.height),
                        camera))}};
    }

    /**
     * @brief Get the box a 1x1 sprite standing on one cell blits into.
     *
     * The ground, a road or a walker standing still; a walker between
     * two cells anchors the same box at a tweened point instead -- see
     * walkerBounds().
     *
     * @param cell The cell the sprite stands on.
     * @param camera Supplies the zoom and the pan.
     * @return The box to blit the sprite into.
     */
    [[nodiscard]] constexpr Rect tileSpriteBounds(
        Cell cell, const Camera &camera) noexcept
    {
        return spriteBounds(
            AtlasKind::OneByOne,
            blockAnchor(cell, Footprint{}, camera),
            camera);
    }

    /**
     * @brief Get the box a building's sprite is blitted into.
     *
     * The sheet is the kind's own -- buildingAtlasOf() -- so the box
     * grows with the footprint exactly as the art does, and the anchor
     * is the whole block's bottom corner rather than the origin cell's.
     *
     * @param origin The minimum-x, minimum-y cell of the block.
     * @param kind The building's kind.
     * @param camera Supplies the zoom and the pan.
     * @return The box to blit the building's sprite into.
     */
    [[nodiscard]] constexpr Rect buildingSpriteBounds(
        Cell origin, BuildingKind kind, const Camera &camera) noexcept
    {
        return spriteBounds(
            buildingAtlasOf(kind),
            blockAnchor(origin, footprintOf(kind), camera),
            camera);
    }

    // One cell's anchor is its own diamond's bottom corner.
    // Saying it against cellBounds() pins the two projections together.
    static_assert(
        blockAnchor(Cell{.x = 3, .y = 4}, Footprint{}, Camera())
        == Point{
            .x = cellBounds(Cell{.x = 3, .y = 4}, Camera()).origin.x
                + static_cast<std::int32_t>(Camera().halfWidth()),
            .y = cellBounds(Cell{.x = 3, .y = 4}, Camera()).origin.y
                + 2 * static_cast<std::int32_t>(Camera().halfHeight())});

    // The default zoom doubles the art.
    // One pinned example, so the arithmetic cannot silently flip.
    static_assert(
        tileSpriteBounds(Cell{}, Camera())
        == Rect{
            .origin = {.x = -64, .y = -96},
            .size = {.width = 128, .height = 192}});

} // namespace antwika::game
