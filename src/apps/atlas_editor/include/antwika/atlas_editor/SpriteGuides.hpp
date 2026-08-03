#pragma once

#include <cstdint>
#include <optional>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/atlas_editor/TileGrid.hpp"

namespace antwika::atlas_editor
{

    using antwika::gfx::Point;
    using antwika::gfx::Size;

    /**
     * @brief How much of a slot is margin either side of the diamond.
     *
     * The three numbers below are the shape every sprite of the game's
     * sheets is drawn to, said once: see wiki/apps/game-texture-atlas.md,
     * which states them as 16 pixels of margin either side, 48 above the
     * diamond's top corner for headroom, and 32 below its bottom corner
     * for the base block's skirt and its padding.
     * They are the same at all three sprite sizes, which is what lets the
     * guides below be arithmetic over one tile size rather than a table
     * of three.
     */
    inline constexpr std::uint32_t kSpriteSideMargin = 16;

    /**
     * @brief How much of a slot is headroom above the diamond.
     */
    inline constexpr std::uint32_t kSpriteHeadroom = 48;

    /**
     * @brief How much of a slot hangs below the pivot.
     */
    inline constexpr std::uint32_t kSpriteSkirtBand = 32;

    /**
     * @brief Where one slot's footprint diamond is, inside the slot.
     *
     * **A drawing aid and nothing else**, exactly as TileGrid is: no
     * tool, no click and no saved byte depends on it, so a slot size
     * these come out wrong for shows a misleading picture and cannot
     * damage a sheet.
     *
     * What they buy is the one thing the slot grid alone cannot say.
     * The grid says which sprite a pixel is in; these say where inside
     * that sprite the cell it lands on actually is -- and getting that
     * wrong is what reads as a seam at one zoom level and not another,
     * which is the failure wiki/apps/game-texture-atlas.md exists to
     * describe and which nothing in this editor could previously show.
     */
    struct SpriteGuides
    {
        /**
         * @brief The diamond's bottom corner, in slot pixels.
         *
         * The point a blit anchors to its block's own bottom corner on
         * screen, so headroom rises above it and the skirt hangs below.
         */
        Point pivot{};

        /**
         * @brief How wide and tall the diamond is, in pixels.
         */
        Size footprint{};

        /**
         * @brief Compare two sets of guides.
         * @param other The guides to compare against.
         * @return True when the pivot and the footprint both match.
         */
        [[nodiscard]] bool operator==(const SpriteGuides &other) const =
            default;
    };

    /**
     * @brief Work out where a slot of this size puts its diamond.
     *
     * **Refused rather than approximated when the numbers do not come
     * out**, which is the whole of why this returns an optional.
     * The margins and the two bands leave a diamond of a definite size,
     * and an isometric diamond is twice as wide as it is tall; a slot
     * size where those two disagree is one this contract does not
     * describe, and drawing a diamond that is not the one the game
     * projects would be worse than drawing none -- an artist would paint
     * to it.
     *
     * @param tile How big one slot is.
     * @return The guides, or nothing when a slot that size has no room
     * for the margins and the bands, or when what is left of it is not
     * an isometric diamond.
     */
    [[nodiscard]] std::optional<SpriteGuides> guidesForTile(
        TileGrid tile) noexcept;

} // namespace antwika::atlas_editor
