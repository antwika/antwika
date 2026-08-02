#pragma once

#include <cstddef>
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
         * @brief Whether this is the area the typing belongs to.
         *
         * The caret is drawn only for a focused area, and only a focused
         * area reports an edit.
         */
        bool focused = false;
    };

} // namespace antwika::ui
