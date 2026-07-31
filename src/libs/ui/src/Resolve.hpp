#pragma once

#include "antwika/ui/Interactions.hpp"
#include "antwika/ui/Keyboard.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/WidgetId.hpp"

#include "LayoutTree.hpp"

namespace antwika::ui::detail
{

    /**
     * @brief Work out what the pointer and the keyboard are on, and dress
     * every widget accordingly.
     *
     * Runs after layout(), because a hit-test needs somewhere to hit, and
     * before flatten(), because what it decides is a background colour
     * and a border that flattening then emits.
     *
     * The topmost named node under the pointer wins, where topmost means
     * the highest index: ascending index is paint order, so descending
     * index is front to back. Layout keeps every child inside its parent,
     * so the frontmost hit is also the deepest one, and one loop answers
     * both questions.
     *
     * Focus is the other way round: it walks the arena in ascending
     * index, which is declaration order, so Tab goes down the layout in
     * the order it was written and wraps round at the end.
     *
     * The two are made to agree rather than left to disagree. A pointer
     * press moves focus to whatever it activated, so tabbing on from a
     * clicked button carries on from there rather than from wherever the
     * keyboard happened to be left.
     *
     * @param tree The arranged arena; every styled node's background and
     * the focused node's border are written.
     * @param pointer What the caller reports about the pointer.
     * @param keyboard The key edges the caller reports, in arrival order.
     * @param focus The widget the caller had focused going in, which is
     * last frame's Interactions::focused. Nothing is remembered here, so
     * this is the only way a frame knows where Tab starts from.
     * @return What the pointer and the keyboard did to the widgets.
     */
    Interactions resolve(
        LayoutTree &tree,
        const Pointer &pointer,
        const Keyboard &keyboard = {},
        WidgetId focus = kNoWidget);

} // namespace antwika::ui::detail
