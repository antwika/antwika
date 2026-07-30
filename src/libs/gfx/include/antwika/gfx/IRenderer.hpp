#pragma once

#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/Rect.hpp"

namespace antwika::gfx
{

    /**
     * @brief Draws into one window's drawable area.
     *
     * Drawing is deliberately a write-only projection of application
     * state: nothing here reports back into the simulation, so rendering
     * cannot influence what a replay reproduces.
     */
    class IRenderer
    {
    public:
        virtual ~IRenderer() = default;

        /**
         * @brief Fill the whole drawable area with one colour.
         * @param color The colour to fill with.
         */
        virtual void clear(Color color) = 0;

        /**
         * @brief Fill a rectangle with one colour.
         * @param rect The rectangle to fill.
         * @param color The colour to fill it with.
         */
        virtual void drawRect(Rect rect, Color color) = 0;

        /**
         * @brief Make everything drawn since the last present visible.
         */
        virtual void present() = 0;
    };

} // namespace antwika::gfx
