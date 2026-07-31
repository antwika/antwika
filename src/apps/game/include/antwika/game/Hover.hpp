#pragma once

#include <optional>

#include <antwika/input/PointerHint.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/SceneSnapshot.hpp"

namespace antwika::game
{

    /**
     * @brief Work out what the pointer is over this frame.
     *
     * A pure function rather than a value somebody keeps, for the reason
     * ghostFor() is one: there is then no stale readout to draw and
     * nothing to keep in step, since the answer is a function of where
     * the pointer is, where the camera is and what was last snapshotted
     * -- and all three are available where it is drawn.
     *
     * **It reads input::PointerHintChannel, so what comes back may
     * decide what is drawn and nothing else.** A live run and its replay
     * do not agree on the hint, deliberately, so folding a readout into
     * anything a replay reproduces would make the two diverge silently.
     * No sink may be handed one -- see BuildGhost.hpp, which this
     * follows exactly.
     *
     * Which thing is under the pointer is worked out through the same
     * screenToCell() a click goes through, and a building is tested
     * across its whole block rather than its origin cell, so what the
     * pointer reports and what a click would hit cannot drift.
     * Reading the camera in order to draw is fine; it is a sink writing
     * state from a hint that is forbidden.
     *
     * A walker wins a tie with a building, since a walker is drawn on
     * top of one -- though the two cannot in fact share a cell, because
     * a walker stands on a road and nothing lays a road under a block.
     *
     * @param hint Where the pointer is, off the channel a replay does
     * not reproduce; nullopt reports nothing, which is a run whose
     * pointer has not been seen.
     * @param camera The camera the pixel is resolved through.
     * @param snapshot What was last drawn, read for the sprites under
     * the pointer; its own hover member is ignored.
     * @param coveredByUi Whether the UI is under the pointer, as
     * UiOverlay reports it; what the bar covers, it covers from this
     * too.
     * @return What to say about it, empty when there is nothing to say.
     */
    [[nodiscard]] HoverReadout hoverFor(
        const std::optional<antwika::input::PointerHint> &hint,
        const Camera &camera,
        const SceneSnapshot &snapshot,
        bool coveredByUi);

} // namespace antwika::game
