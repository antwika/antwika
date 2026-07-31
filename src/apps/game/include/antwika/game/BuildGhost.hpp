#pragma once

#include <optional>

#include <antwika/input/PointerHint.hpp>

#include "antwika/game/BuildTool.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"

namespace antwika::game
{

    /**
     * @brief Where the selected tool would land, if it were clicked now.
     *
     * **A picture, and nothing but a picture.** Nothing is placed until a
     * press arrives; this is only what one would place. It used to be a
     * component on a World entity, staged and committed like anything
     * else the simulation holds, and that is exactly what it may not be
     * now: it is worked out from input::PointerHintChannel, which a
     * replay does not reproduce, so folding it into the World would make
     * a run and its replay disagree there -- silently, with the symptom
     * nowhere near the cause.
     *
     * So it lives on the render side, as a value the renderer works out
     * afresh each frame and hands to the scene, and no sink ever sees
     * one.
     *
     * **Reading simulation state in order to draw is fine; the reverse
     * is not.** ghostFor() reads the camera, which is simulation state,
     * because which cell a pixel means is a function of it -- that is
     * the same screenToCell() a click goes through, and reading it
     * decides only what is drawn. What must never happen is a sink
     * writing state from a hint.
     *
     * It follows a freely moving pointer now, which it could not while
     * input::IdleMotionSource was the only thing carrying motion: the
     * gate still thins the recording and the channel still publishes,
     * and an app wanting a hover wants both.
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
         * @brief Whether clicking here would actually put it up.
         *
         * Worked out from the very predicate GridSink places through, so
         * what a preview promises and what a click delivers cannot come
         * apart.
         *
         * **Reading simulation state in order to draw is allowed**, and
         * is not what this file's rule forbids: what may not happen is a
         * *sink* writing state from a hint, since the hint is the one
         * value no replay reproduces.
         */
        bool valid = false;

        /**
         * @brief Compare two ghosts.
         * @param other The ghost to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(
            const BuildGhost &other) const = default;
    };

    /**
     * @brief Work out the ghost to draw this frame.
     *
     * A pure function rather than a value somebody keeps, so there is no
     * stale ghost to draw and nothing to keep in step: the answer is a
     * function of where the pointer is, where the camera is and what the
     * palette has selected, and every one of those is available where it
     * is drawn.
     *
     * The "covered" answer comes *from* UiOverlay rather than the other
     * way round, deliberately: UiOverlay is derived from recorded input
     * and a sink may read it, where this may not be read by any sink at
     * all.
     *
     * @param hint Where the pointer is, off the channel a replay does
     * not reproduce; nullopt draws no ghost, which is a run whose
     * pointer has not been seen.
     * @param camera The camera the pixel is resolved through.
     * @param extent The bounds a placement may reach.
     * @param tool What the palette has selected.
     * @param coveredByUi Whether the UI is under the pointer, as
     * UiOverlay reports it; what the bar covers, it covers from the
     * ghost too.
     * @return What to draw, invisible when there is nothing to draw.
     */
    [[nodiscard]] BuildGhost ghostFor(
        const std::optional<antwika::input::PointerHint> &hint,
        const Camera &camera,
        GridExtent extent,
        BuildTool tool,
        bool coveredByUi,
        const PathIndex &paths,
        const BuildingIndex &built);

} // namespace antwika::game
