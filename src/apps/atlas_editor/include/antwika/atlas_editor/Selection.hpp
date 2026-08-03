#pragma once

#include <cstdint>
#include <optional>

#include <antwika/gfx/Size.hpp>

#include "antwika/atlas_editor/Pixel.hpp"

namespace antwika::atlas_editor
{

    using antwika::gfx::Size;

    /**
     * @brief A rectangle of the sheet, in image pixels.
     *
     * **Simulation state, like everything else a click decides**, and
     * regenerated from the presses that drew it rather than persisted:
     * what a recording holds is where the pointer went down and came up,
     * and which pixels that came to is worked out again on replay.
     *
     * Always at least one pixel across and one down, because it is made
     * from two corners that are both *in* it -- a press and a release on
     * the same pixel select that pixel rather than nothing.
     */
    struct Selection
    {
        /** @brief The top-left pixel, which is inside the rectangle. */
        Pixel origin{};

        /** @brief How many pixels across and down, both at least one. */
        Size size{};

        /**
         * @brief Compare two selections.
         * @param other The selection to compare against.
         * @return True when the origin and the size both match.
         */
        [[nodiscard]] bool operator==(const Selection &other) const =
            default;
    };

    /**
     * @brief Make the selection two corners mark out.
     *
     * Both corners are inside the result, whichever way round they were
     * given: a drag up and to the left selects what it crossed exactly as
     * one down and to the right does.
     *
     * @param from One corner.
     * @param to The other.
     * @return The rectangle holding both, never smaller than one pixel.
     */
    [[nodiscard]] Selection selectionBetween(Pixel from, Pixel to) noexcept;

    /**
     * @brief Check whether a pixel is inside a selection.
     * @param selection The selection to ask about.
     * @param pixel The pixel to place.
     * @return True when it falls inside, edges included.
     */
    [[nodiscard]] bool contains(
        const Selection &selection, Pixel pixel) noexcept;

    /**
     * @brief Slide a selection across the sheet.
     * @param selection The selection to move.
     * @param across How far to the right, which may be negative.
     * @param down How far down, which may be negative.
     * @return The moved selection, the same size as it was.
     */
    [[nodiscard]] Selection movedBy(
        const Selection &selection,
        std::int32_t across,
        std::int32_t down) noexcept;

    /**
     * @brief Cut a selection down to the part of it a sheet holds.
     *
     * A drag may leave the sheet, and a paste may land half off it, so a
     * selection is only ever *acted* on through this -- which is what
     * lets everything reading one index the image arithmetically without
     * testing every pixel it touches.
     *
     * @param selection The selection to clamp.
     * @param sheet How big the image is.
     * @return The overlapping part, or nothing when there is none.
     */
    [[nodiscard]] std::optional<Selection> clampedTo(
        const Selection &selection, Size sheet) noexcept;

} // namespace antwika::atlas_editor
