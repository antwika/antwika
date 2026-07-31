#pragma once

#include "antwika/game/BuildTool.hpp"
#include "antwika/game/Cell.hpp"

namespace antwika::game
{

    /**
     * @brief Where the selected tool would land, if it were clicked now.
     *
     * A picture rather than a fact about the world: nothing is placed
     * until a press arrives, and this is only what one would place. It
     * still cannot be worked out by a renderer, because which cell a
     * pixel means is a function of the camera, which is simulation state
     * -- so GridSink works it out with the same screenToCell() a click
     * goes through, and the scene is handed the answer.
     *
     * It is one component on one entity rather than a field on the
     * snapshot's producer, because the snapshot is taken from the World
     * and nothing else reaches the renderer -- see SceneSnapshot.
     *
     * **The gate in main.cpp is why this only moves on a click, a wheel
     * or a key**: input::IdleMotionSource holds back pointer movement
     * while no button is held, so a freely moving pointer reports
     * nothing to follow.
     */
    struct BuildGhost
    {
        Cell at{};
        BuildTool tool = BuildTool::Road;

        /**
         * @brief Whether there is anywhere to draw it at all.
         *
         * False when nothing has said where the pointer is, when it is
         * over the toolbar, or when the cell under it is off the grid.
         */
        bool visible = false;

        /**
         * @brief Compare two ghosts.
         * @param other The ghost to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(
            const BuildGhost &other) const = default;
    };

} // namespace antwika::game
