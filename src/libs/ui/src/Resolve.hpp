#pragma once

#include "antwika/ui/Interactions.hpp"
#include "antwika/ui/Pointer.hpp"

#include "LayoutTree.hpp"

namespace antwika::ui::detail
{

    /**
     * @brief Work out what the pointer is on, and dress every widget
     * accordingly.
     *
     * Runs after layout(), because a hit-test needs somewhere to hit, and
     * before flatten(), because what it decides is a background colour
     * that flattening then emits.
     *
     * The topmost named node under the pointer wins, where topmost means
     * the highest index: ascending index is paint order, so descending
     * index is front to back. Layout keeps every child inside its parent,
     * so the frontmost hit is also the deepest one, and one loop answers
     * both questions.
     *
     * @param tree The arranged arena; every styled node's background is
     * written.
     * @param pointer What the caller reports about the pointer.
     * @return What the pointer did to the widgets.
     */
    Interactions resolve(LayoutTree &tree, const Pointer &pointer);

} // namespace antwika::ui::detail
