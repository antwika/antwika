#pragma once

#include <optional>

#include "antwika/ui/ButtonState.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    /**
     * @brief What a button is being asked for.
     *
     * ContainerSpec's counterpart, and for the same reason: options
     * accumulate, and a fourth positional argument reads as nothing at
     * all at the call site.
     */
    struct ButtonSpec
    {
        /**
         * @brief What to call this button when reporting what the pointer
         * did to it.
         *
         * A button left unnamed still draws, and still takes the theme's
         * button colours, but nothing can hover or activate it.
         */
        WidgetId id = kNoWidget;

        /**
         * @brief How wide, defaulting to fitting the label.
         */
        Sizing width = kFit;

        /**
         * @brief How the button must look, whatever the pointer is doing.
         *
         * Set this for a button whose appearance is the application's to
         * decide -- the one whose turn it is, or one that is not to look
         * available. Left unset, the appearance is worked out from the
         * pointer.
         *
         * Appearance only: a button forced to look a certain way still
         * activates when it is pressed.
         */
        std::optional<ButtonState> state{};
    };

} // namespace antwika::ui
