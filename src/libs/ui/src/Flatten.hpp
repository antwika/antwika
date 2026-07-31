#pragma once

#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/HoverTargets.hpp"

#include "LayoutTree.hpp"

namespace antwika::ui::detail
{

    /**
     * @brief Turn a laid-out arena into the picture it describes.
     *
     * Walks the arena in index order, which is declaration order, and so
     * emits a container's own fill before anything nested inside it.
     *
     * Text is cut to fit here rather than when it is declared, because
     * this is the first point at which both the string and the room it
     * ended up with are known.
     * A line too wide for its box is cut to whole glyph cells, and a line
     * too tall for its box is left out altogether, since antwika::gfx can
     * draw no part of a glyph.
     *
     * A focus border is four filled bars rather than a stroke, since
     * antwika::gfx has neither a stroke nor a scissor.
     *
     * The arena is walked twice, once per layer, and each layer's
     * borders come after that layer's widgets. So the commands arrive
     * in exactly four runs, in this order and no other:
     *
     * 1. every widget not in an overlay;
     * 2. the focus border of one of those, if that is where focus is;
     * 3. every widget of an open dropdown's overlay;
     * 4. the focus border of one of those, if that is where focus is.
     *
     * Two layers because antwika::gfx has no depth of any kind, so
     * being on top is being last.
     * Borders after widgets within a layer because a container declared
     * later would otherwise paint over one.
     * Borders inside their layer rather than all at the end because an
     * overlay must cover the ring of what it drops over, exactly as it
     * covers the widget itself.
     *
     * Which widgets a hover pointer may recolour is collected here too,
     * and here rather than in a pass of its own for the reason layout()
     * collects its rectangles where it does: a target names a command by
     * index, so the only place that index is certain is the loop that
     * appends it.
     *
     * @param tree The arena, already laid out.
     * @param targets Receives one entry per named widget that works its
     * own appearance out, in paint order, or null to collect none.
     * @return The commands, in the order they are drawn.
     */
    [[nodiscard]] DrawList flatten(
        const LayoutTree &tree, HoverTargets *targets = nullptr);

} // namespace antwika::ui::detail
