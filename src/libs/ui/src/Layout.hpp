#pragma once

#include <antwika/gfx/Size.hpp>

#include "antwika/ui/WidgetRects.hpp"

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
     * Where each named node ended up is read out of the arranging pass
     * itself rather than from a pass of its own.
     * By the time that loop reaches a node, the node's own area is final
     * -- a parent sits at a lower index and is what placed it -- so the
     * rectangle collected here is the one flatten() is about to draw
     * from, by construction rather than by agreement.
     *
     * @param tree The arena to lay out, modified in place.
     * @param canvas The area the root node fills.
     * @param rects Receives one entry per distinct named id, or null to
     * collect nothing. A caller that wants no mapping builds none: the
     * cost of asking for one is a comparison per node and a vector that
     * stays empty until a node is named.
     */
    void layout(
        LayoutTree &tree, Size canvas, WidgetRects *rects = nullptr);

} // namespace antwika::ui::detail
