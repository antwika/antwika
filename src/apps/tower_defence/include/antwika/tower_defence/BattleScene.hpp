#pragma once

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/tower_defence/BattleSnapshot.hpp"

namespace antwika::tower_defence
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;

    /**
     * @brief Draws a battle: the ground, the run, the towers and their
     * reach, and one marker per mob.
     *
     * Stateless and deterministic, like life::BoardScene.
     * The same snapshot and canvas always produce the same drawing calls
     * in the same order, which is what makes the picture assertable
     * against a mock renderer instead of having to be looked at.
     *
     * It neither clears the score bar's strip nor presents the frame:
     * the bar is painted over the top afterwards, and presenting belongs
     * to whoever owns the window.
     */
    class BattleScene final
    {
    public:
        /**
         * @brief Draw one frame.
         * @param renderer Receives the drawing calls.
         * @param canvas The size of the area being drawn into.
         * @param snapshot What to draw.
         */
        void draw(
            IRenderer &renderer,
            Size canvas,
            const BattleSnapshot &snapshot) const;
    };

} // namespace antwika::tower_defence
