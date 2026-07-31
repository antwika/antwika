#pragma once

#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/Interactions.hpp"

namespace antwika::ui
{

    /**
     * @brief One finished frame: the picture, and what the input did to
     * it.
     *
     * Both come out of one call, because both are read off one layout.
     * Handing them back separately would invite describing the UI twice
     * and letting the two answers drift.
     */
    struct Frame
    {
        /**
         * @brief The drawing commands, in the order they are drawn.
         */
        DrawList commands;

        /**
         * @brief What this frame's pointer and keyboard did to this
         * frame's widgets.
         */
        Interactions interactions;
    };

} // namespace antwika::ui
