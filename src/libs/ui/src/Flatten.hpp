#pragma once

#include "antwika/ui/DrawList.hpp"

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
     * @param tree The arena, already laid out.
     * @return The commands, in the order they are drawn.
     */
    [[nodiscard]] DrawList flatten(const LayoutTree &tree);

} // namespace antwika::ui::detail
