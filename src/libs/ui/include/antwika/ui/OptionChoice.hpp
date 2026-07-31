#pragma once

#include <cstddef>

#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    /**
     * @brief The option a press landed on this frame.
     *
     * Reported by index rather than by text, since the index is what the
     * caller handed the options in and what it can look one up with.
     */
    struct OptionChoice
    {
        /**
         * @brief Which dropdown the option belongs to.
         */
        WidgetId dropdown = kNoWidget;

        /**
         * @brief Which of that dropdown's options was pressed.
         */
        std::size_t index = 0;

        /**
         * @brief Compare two choices.
         * @param other The choice to compare against.
         * @return True when the dropdown and the index both match.
         */
        [[nodiscard]] bool operator==(const OptionChoice &other) const =
            default;
    };

} // namespace antwika::ui
