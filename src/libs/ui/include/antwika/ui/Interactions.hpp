#pragma once

#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    /**
     * @brief What this frame's pointer did to this frame's widgets.
     *
     * Resolved against the layout the same frame draws, so what a click
     * hit is what was on screen when it was clicked.
     */
    struct Interactions
    {
        /**
         * @brief The topmost named widget under the pointer.
         */
        WidgetId hovered = kNoWidget;

        /**
         * @brief The widget a press landed on this frame.
         *
         * One pointer produces one press, and one press lands on one
         * topmost widget, so a second activation in a frame is not
         * expressible.
         */
        WidgetId activated = kNoWidget;

        /**
         * @brief Whether the pointer is over anything this UI filled in.
         *
         * A filled panel covers whatever was drawn underneath it, so a
         * click on it must not also reach what it covers. A node that
         * draws nothing covers nothing, so a growing spacer in a
         * transparent row is not a wall.
         */
        bool pointerOverUi = false;

        /**
         * @brief Compare what two frames' pointers did.
         * @param other The interactions to compare against.
         * @return True when the hovered and activated widgets and the
         * cover flag all match.
         */
        [[nodiscard]] bool operator==(const Interactions &other) const =
            default;
    };

} // namespace antwika::ui
