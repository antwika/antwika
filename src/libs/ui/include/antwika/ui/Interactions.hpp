#pragma once

#include <optional>

#include "antwika/ui/OptionChoice.hpp"
#include "antwika/ui/TextEdit.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    /**
     * @brief What this frame's pointer and keyboard did to this frame's
     * widgets.
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
         *
         * A press is resolved while the frame is being laid out, so this
         * is known only once the picture beside it has been decided.
         * Whatever the caller then changes in response is therefore not
         * in that picture: to show it in the same frame, describe the UI
         * again after acting and draw the second frame instead. See
         * Context::finish().
         */
        WidgetId activated = kNoWidget;

        /**
         * @brief The widget the keyboard is on once this frame is done.
         *
         * Focus is the one thing about a UI that has to survive a frame:
         * Tab means "the one after the one I am on". This library still
         * remembers nothing, so the state lives with the caller instead
         * -- last frame's focus goes in through Context, this frame's
         * comes back out here, and the caller hands it round.
         *
         * That is deliberate rather than convenient. Focus kept inside
         * the library would be state a replay could not regenerate; kept
         * in application state it falls out of the recorded key presses
         * the same way a score does, which is the same reason a widget
         * activates on the press rather than on a press-then-release
         * match.
         *
         * kNoWidget when nothing is focused, which is also what a focus
         * naming a widget this frame did not declare comes back as.
         */
        WidgetId focused = kNoWidget;

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
         * @brief What this frame's typing did to the focused field.
         *
         * Absent when nothing was typed, or when no field this frame
         * said it had focus. See TextEdit: what is here is what the
         * characters would become, not what they are.
         */
        std::optional<TextEdit> edit{};

        /**
         * @brief The dropdown option a press landed on this frame.
         *
         * Absent unless a press landed on an option of an open list.
         * Separate from activated because an option's index is what the
         * caller needs, and an id alone cannot carry it.
         */
        std::optional<OptionChoice> chosen{};

        /**
         * @brief Compare what two frames' input did.
         * @param other The interactions to compare against.
         * @return True when every field matches, the reported edit and
         * the chosen option included.
         */
        [[nodiscard]] bool operator==(const Interactions &other) const =
            default;
    };

} // namespace antwika::ui
