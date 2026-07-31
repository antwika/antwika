#pragma once

#include <cstddef>
#include <limits>
#include <span>
#include <string_view>

#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    /**
     * @brief A selection meaning "nothing is selected yet".
     */
    inline constexpr std::size_t kNoOption =
        std::numeric_limits<std::size_t>::max();

    /**
     * @brief What a dropdown is being asked for.
     *
     * **Whether the list is open is not the library's to remember.** It
     * arrives here, and a press on the closed box arrives back through
     * Interactions::activated, so the application holds the one bit that
     * says which of the two pictures is drawn -- exactly as it holds a
     * text field's characters, and for the same reason.
     */
    struct DropdownSpec
    {
        /**
         * @brief What to call the closed box.
         */
        WidgetId id = kNoWidget;

        /**
         * @brief The id the first option carries.
         *
         * Option `n` carries this plus `n`, so the caller must leave
         * that whole range free among the ids one frame declares.
         *
         * Left unset, the options are unnamed and so never light up
         * under the pointer; pressing one is still reported through
         * Interactions::chosen, which does not go through an id.
         */
        WidgetId optionIdBase = kNoWidget;

        /**
         * @brief How wide the closed box is, defaulting to its content.
         *
         * The open list is at least this wide, whatever its options.
         */
        Sizing width = kFit;

        /**
         * @brief The options to choose between, in the caller's order.
         *
         * The caller owns them and must keep them alive for as long as
         * the Context is.
         */
        std::span<const std::string_view> options{};

        /**
         * @brief Which option is currently selected.
         *
         * Anything outside the options shows the placeholder instead, so
         * kNoOption needs no special handling at a call site.
         */
        std::size_t selected = kNoOption;

        /**
         * @brief What the closed box shows while nothing is selected.
         */
        std::string_view placeholder{};

        /**
         * @brief Whether to draw the list of options.
         */
        bool open = false;
    };

} // namespace antwika::ui
