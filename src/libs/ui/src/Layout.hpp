#pragma once

#include <antwika/gfx/Size.hpp>

#include "LayoutTree.hpp"

namespace antwika::ui::detail
{

    using antwika::gfx::Size;

    /**
     * @brief Give every node in the arena a position and a size.
     *
     * Two flat passes over the arena.
     * Measuring runs from the last node to the first, so every child has
     * reported what its content needs before its parent asks; arranging
     * runs from the first to the last, so every parent has been placed
     * before its children are placed inside it.
     *
     * Every child ends up inside its parent, because antwika::gfx offers
     * no clipping and so a container that let its content escape would
     * draw over its neighbours rather than being cut off at its edge.
     * A container with more content than room shrinks its children in
     * proportion instead of dropping the ones that do not fit.
     *
     * @param tree The arena to lay out, modified in place.
     * @param canvas The area the root node fills.
     */
    void layout(LayoutTree &tree, Size canvas);

} // namespace antwika::ui::detail
