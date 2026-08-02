#pragma once

#include <cstddef>

#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    /**
     * @brief Which line a text area is actually showing at its top.
     *
     * Reported only when it is not the line the caller asked for, so a
     * caller that stores it back and hands it in again is handed
     * nothing the next frame. Three things move it: a press or a drag
     * on the area's scrollbar, a caret that has walked out of view, and
     * a requested line so far down that there is nothing left to show.
     *
     * A line rather than a pixel offset, for TextAreaSpec::scroll's
     * reason: what an area shows is whole lines.
     */
    struct ScrollChange
    {
        /**
         * @brief Which area this is about.
         */
        WidgetId area = kNoWidget;

        /**
         * @brief The line now drawn at the top of it.
         */
        std::size_t line = 0;

        /**
         * @brief Compare two reports.
         * @param other The report to compare against.
         * @return True when both name the same area and the same line.
         */
        [[nodiscard]] bool operator==(const ScrollChange &other) const =
            default;
    };

} // namespace antwika::ui
