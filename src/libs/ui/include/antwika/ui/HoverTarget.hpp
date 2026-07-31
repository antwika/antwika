#pragma once

#include <cstddef>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    using antwika::gfx::Color;
    using antwika::gfx::Rect;

    /**
     * @brief One widget a hover pointer is allowed to recolour, and the
     * one command that recolouring would rewrite.
     *
     * The bridge between a laid-out frame and a hover pass that may
     * touch nothing but the picture. Everything needed to answer "is the
     * pointer over this, and what colour would that make it" is copied
     * out here, so applyHover() never sees the arena, the interactions
     * or the frame -- only a list of these and a draw list.
     *
     * Collected for every named widget that works its own appearance
     * out, which is exactly the set resolve() would have dressed.
     * A widget the caller dressed itself carries none: it was told how
     * to look, and being told is the end of the matter.
     */
    struct HoverTarget
    {
        /**
         * @brief Which widget this is.
         *
         * Carried so a caller can see what a hover pass would light up
         * without re-deriving it, and never read by applyHover(), which
         * decides on geometry alone.
         */
        WidgetId id = kNoWidget;

        /**
         * @brief The area the layout arranged that widget into.
         *
         * The rectangle the picture was drawn from, so a hover hit-test
         * and the picture cannot disagree.
         */
        Rect rect{};

        /**
         * @brief Which command in the frame's draw list fills that area.
         *
         * An index rather than a pointer, so a frame stays a plain value
         * that can be copied, compared and asserted on.
         */
        std::size_t command = 0;

        /**
         * @brief The colour this widget shows when nothing is on it.
         */
        Color idle{};

        /**
         * @brief The colour this widget shows when the pointer is on it.
         */
        Color hovered{};

        /**
         * @brief Whether the recorded pointer is holding this widget
         * down.
         *
         * A press is real input: it is recorded, it replays, and it is
         * resolved inside the tick path like every other interaction.
         * So a held widget is already showing the appearance the
         * simulation's own input gave it, and a hover pass steps over it
         * rather than repainting it -- which is what keeps a button
         * looking pressed while it is being pressed.
         */
        bool held = false;

        /**
         * @brief Compare two targets.
         * @param other The target to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const HoverTarget &other) const =
            default;
    };

} // namespace antwika::ui
