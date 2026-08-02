#pragma once

#include <cstddef>
#include <string>

#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    /**
     * @brief What this frame's typing did to a text field.
     *
     * The whole of a field's answer, because the library keeps nothing
     * between frames: it never owns the characters a field shows, so it
     * cannot edit them either. It reports what they would become, and
     * the application stores that where a replay regenerates it.
     *
     * The picture beside this was drawn from the text the caller passed
     * in, which predates the edit -- the same rule Context::finish()
     * gives for an activated button. Describe the UI again after
     * applying an edit to show it in the same frame.
     */
    struct TextEdit
    {
        /**
         * @brief Which field this edit belongs to.
         */
        WidgetId field = kNoWidget;

        /**
         * @brief What the field's characters would become.
         */
        std::string text{};

        /**
         * @brief Where the caret would sit, as an index into text.
         */
        std::size_t cursor = 0;

        /**
         * @brief Where the selection's other end would sit.
         *
         * Equal to cursor when nothing is selected, which is what an
         * ordinary move leaves it as: a selection is the characters
         * between the two, whichever of them is the lower.
         *
         * Two indices rather than a start and a length, because which
         * end the caret is on is what decides where Shift+Left goes
         * next, and a length cannot say.
         */
        std::size_t anchor = 0;

        /**
         * @brief The characters Copy or Cut asked for, if either did.
         *
         * Empty when neither arrived, and when one did with nothing
         * selected. **Putting them somewhere is the caller's**: this
         * library holds nothing between frames, so it can report what
         * was copied but never keep it. See Key::Copy.
         */
        std::string copied{};

        /**
         * @brief Whether Enter was pressed while the field had focus.
         */
        bool submitted = false;

        /**
         * @brief Whether Escape was pressed while the field had focus.
         *
         * What cancelling means is the application's to decide, since
         * only it knows what the field held before it was opened.
         */
        bool cancelled = false;

        /**
         * @brief Compare two edits.
         * @param other The edit to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const TextEdit &other) const =
            default;
    };

} // namespace antwika::ui
