#pragma once

#include <vector>

#include "antwika/ui/HoverTarget.hpp"

namespace antwika::ui
{

    /**
     * @brief Every widget of one frame a hover pointer may recolour.
     *
     * In the order the frame paints them, so the last one a position
     * falls inside is the frontmost -- the same answer resolve() gets by
     * scanning the arena backwards, off the same ordering, rather than a
     * second depth rule that could drift from it.
     *
     * Empty for a frame that named no interactive widget, which is what
     * it stays for every caller that never draws a button.
     */
    using HoverTargets = std::vector<HoverTarget>;

} // namespace antwika::ui
