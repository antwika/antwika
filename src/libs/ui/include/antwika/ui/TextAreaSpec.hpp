#pragma once

#include <cstddef>
#include <optional>
#include <string_view>

#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/TextFieldSpec.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    /**
     * @brief What a text area is being asked for.
     *
     * A text field over many lines, and the same bargain as one: the
     * characters are the caller's, they arrive here as a view the caller
     * owns, and an edit is reported back through Interactions::edit
     * rather than applied.
     *
     * **A line break is a character in the text like any other.**
     * So the caret is one index into the whole of it rather than a row
     * and a column, and an application storing a document plus a cursor
     * is storing everything a replay has to regenerate.
     */
    struct TextAreaSpec
    {
        /**
         * @brief What to call this area when reporting what happened.
         *
         * An area left unnamed still draws and still edits, but nothing
         * can hover it, and the edit it reports names no widget.
         */
        WidgetId id = kNoWidget;

        /**
         * @brief How wide, defaulting to filling the room across.
         */
        Sizing width = kGrow;

        /**
         * @brief How tall, defaulting to filling the room down.
         *
         * Unlike a field, which is one line high whatever it is given:
         * an area is somewhere to write, and how much room there is to
         * write in is the container's answer rather than the text's.
         */
        Sizing height = kGrow;

        /**
         * @brief The characters the area currently holds.
         *
         * Split into lines on '\n' as it is drawn, and the caller owns
         * the buffer for as long as the Context is.
         */
        std::string_view text{};

        /**
         * @brief What to show instead while the area is empty.
         */
        std::string_view placeholder{};

        /**
         * @brief Where the caret sits, as an index into text.
         *
         * Past the end is the end, so a caller may hand back the cursor
         * of an edit it has applied without clamping it itself.
         */
        std::size_t cursor = kCaretAtEnd;

        /**
         * @brief Where the selection's other end sits.
         *
         * The characters between this and the cursor are drawn on the
         * theme's selection colour, and are what a typed character, a
         * Backspace, a Delete, a Copy or a Cut acts on instead of one
         * character.
         *
         * Absent -- which is the default -- puts it wherever the caret
         * is, so nothing is selected. Absent rather than a sentinel
         * index, because every index is a place a selection can really
         * end, the end of the text included.
         *
         * Past the end is the end, as cursor is.
         */
        std::optional<std::size_t> anchor{};

        /**
         * @brief Which line of the text is drawn at the top.
         *
         * Lines rather than pixels, so what an area shows is always
         * whole lines and a recorded click always lands on one.
         *
         * Past the last line it can usefully be is brought back, so a
         * caller may add a wheel's notches to it without knowing how
         * many lines fit. The line actually drawn at the top comes back
         * through Interactions::scrolled whenever it differs from this,
         * which is also how the caret is kept in view: an area that
         * reports an edit this frame scrolls to wherever its caret
         * ended up.
         */
        std::size_t scroll = 0;

        /**
         * @brief Whether to draw a bar down the right-hand edge saying
         * how much of the text is showing.
         *
         * It takes its width out of the room the text has, and a press
         * or a drag anywhere on it scrolls, reported through
         * Interactions::scrolled. An area without one still scrolls;
         * there is simply nothing to grab.
         */
        bool scrollbar = false;

        /**
         * @brief Whether this is the area the typing belongs to.
         *
         * The caret is drawn only for a focused area, and only a focused
         * area reports an edit.
         */
        bool focused = false;
    };

} // namespace antwika::ui
