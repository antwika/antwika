#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "antwika/ui/TextAreaSpec.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui::detail
{

    /**
     * @brief One text area, and everything the pointer needs to be
     * resolved against it once there is a layout.
     *
     * A text area is the one widget whose answer cannot be worked out
     * where it is declared. Which character a click landed on is a
     * function of where the area ended up, and so is how many of its
     * lines are showing -- and neither is known until the tree has been
     * arranged. So the area is noted here as it is written, and read
     * again by resolve().
     *
     * The characters are a view, on the same terms TextAreaSpec::text
     * and Keyboard::typed are: the caller owns them for as long as the
     * Context lives.
     */
    struct Area
    {
        /** @brief Which widget this is, for reporting against. */
        WidgetId id = kNoWidget;

        /** @brief The node holding the lines that are showing. */
        std::size_t column = 0;

        /** @brief The scrollbar's channel, or kNoNode without one. */
        std::size_t track = 0;

        /** @brief The part of that channel standing for what shows. */
        std::size_t thumb = 0;

        /** @brief The whole document. */
        std::string_view text{};

        /** @brief The line drawn at the top, as it was drawn. */
        std::size_t scroll = 0;

        /**
         * @brief The line the caller asked to have at the top.
         *
         * What the report is compared against rather than the line
         * above, so a caller asking for one a long way past the end is
         * told the line it actually got.
         */
        std::size_t requested = 0;

        /** @brief How many lines the document has. */
        std::size_t lines = 1;

        /** @brief Where the caret is, brought inside the text. */
        std::size_t cursor = 0;

        /** @brief The selection's other end, brought inside it. */
        std::size_t anchor = 0;

        /** @brief Where the drag in progress began, as the spec said. */
        DragHome dragging = DragHome::None;

        /** @brief Pixels from one line's top edge to the next one's. */
        std::uint32_t lineHeight = 1;

        /** @brief Pixels from one glyph's cell to the next one's. */
        std::uint32_t advance = 1;

        /** @brief Whether the typing belongs to this area. */
        bool focused = false;
    };

} // namespace antwika::ui::detail
