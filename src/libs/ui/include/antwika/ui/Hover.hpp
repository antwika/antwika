#pragma once

#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/HoverPointer.hpp"
#include "antwika/ui/HoverTargets.hpp"

namespace antwika::ui
{

    /**
     * @brief Repaint a finished picture to show what the pointer is
     * over.
     *
     * **Appearance, and nothing else.** This is handed a draw list and a
     * list of targets and it is handed nothing else -- no Frame, no
     * Interactions, no arena -- so there is no field it could write an
     * activated, focused, edited or chosen widget into even if it tried.
     * A hover cannot decide what a run computes here in the way a
     * `const` promises it will not: the reference it is given is to the
     * picture.
     *
     * That matters because the position a caller passes is not one a
     * replay reproduces. A free-moving pointer reaches an application on
     * antwika::input::PointerHintChannel, outside the recorded event
     * stream, so a live run and its replay disagree about it by design.
     * Everything a replay does reproduce -- which button a press
     * activated, where focus went, what a field became -- is decided by
     * Context::finish() from the recorded ui::Pointer, inside the tick
     * path, and is already settled before this is called.
     * See docs/hover-is-not-simulation.md.
     *
     * Called with a hover pointer reporting no position, this changes
     * nothing at all, so a caller that never opts in gets byte for byte
     * the picture Context::finish() produced.
     *
     * Called with one, it decides the appearance of every target rather
     * than only the one under the pointer: the frontmost target the
     * position falls inside is painted hovered and every other is
     * painted idle. Lighting one up without putting the others out would
     * leave the widget the recorded pointer last passed over lit for the
     * rest of the run, which is the very thing an application attaches a
     * hint channel to fix.
     *
     * A target sharing the frontmost one's id is painted hovered too,
     * because two nodes carrying one id are one widget -- the rule the
     * tab order and Context::finish()'s own dressing both keep. A
     * widget declared as two boxes would otherwise light up half of
     * itself here and all of itself there.
     *
     * A held target is stepped over either way. A press is recorded
     * input and its appearance was resolved from it, so a button being
     * pressed goes on looking pressed.
     *
     * Total, and never throws: a target naming a command past the end of
     * the list, or one that is not a fill, changes nothing. The two are
     * independent values a caller may pair as it likes, so a mismatched
     * pair is an ordinary thing to be handed rather than a precondition
     * to police.
     *
     * @param commands The picture to repaint, in place. Only the colour
     * of a fill a target names is ever written.
     * @param targets Which widgets may be recoloured, in paint order;
     * ui::Frame::hoverTargets is where a frame's own come from.
     * @param hover Where the free-moving pointer is, if anywhere.
     */
    void applyHover(
        DrawList &commands,
        const HoverTargets &targets,
        HoverPointer hover);

} // namespace antwika::ui
