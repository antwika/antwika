#pragma once

#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/Interactions.hpp"
#include "antwika/ui/WidgetRects.hpp"

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

        /**
         * @brief Where each of this frame's named widgets was laid out.
         *
         * The third answer off the one layout, for the same reason the
         * other two come out together: an application that has to place
         * something of its own against the UI can read where the UI went
         * instead of computing a second layout that agrees with this one
         * only until either changes.
         *
         * Empty when nothing was named, which is what it stays for every
         * caller that never names anything.
         * See WidgetRects.
         */
        WidgetRects rects;
    };

} // namespace antwika::ui
