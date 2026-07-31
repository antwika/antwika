#pragma once

#include <cstddef>
#include <limits>
#include <string_view>

#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    /**
     * @brief A caret index meaning "after the last character".
     *
     * The default, so a caller that only wants somewhere to type does
     * not have to track a caret at all.
     */
    inline constexpr std::size_t kCaretAtEnd =
        std::numeric_limits<std::size_t>::max();

    /**
     * @brief What a text field is being asked for.
     *
     * **The field's characters are not the library's.** They arrive here
     * as a view the caller owns, and an edit is reported back through
     * Interactions::edit rather than applied: nothing in antwika::ui is
     * retained between frames, so a field that owned what was typed
     * would be state a replay could not regenerate.
     */
    struct TextFieldSpec
    {
        /**
         * @brief What to call this field when reporting what happened.
         *
         * A field left unnamed still draws and still edits, but nothing
         * can hover it, and the edit it reports names no field.
         */
        WidgetId id = kNoWidget;

        /**
         * @brief How wide, defaulting to filling the room across.
         */
        Sizing width = kGrow;

        /**
         * @brief The characters the field currently holds.
         *
         * The caller owns the buffer and must keep it alive for as long
         * as the Context is.
         */
        std::string_view text{};

        /**
         * @brief What to show instead while the field is empty.
         *
         * Drawn in the theme's muted colour, since it is not content.
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
         * @brief Whether this is the field the typing belongs to.
         *
         * The caret is drawn only for a focused field, and only a
         * focused field reports an edit. Which field has focus is the
         * application's to decide -- usually by watching for its id in
         * Interactions::activated.
         */
        bool focused = false;
    };

} // namespace antwika::ui
