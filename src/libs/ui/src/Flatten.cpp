#include "Flatten.hpp"

#include <cstddef>
#include <cstdint>

#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/TextLayout.hpp>

#include "antwika/ui/DrawCommand.hpp"

#include "NodeKind.hpp"

namespace antwika::ui::detail
{

    DrawList flatten(const LayoutTree &tree)
    {
        DrawList commands;

        for (std::size_t index = 0; index < tree.size(); ++index)
        {
            const auto &node = tree.node(index);

            if (node.background)
            {
                commands.push_back(FillRect{
                    .rect = node.arranged, .color = *node.background});
            }

            if (node.kind != NodeKind::Text)
            {
                continue;
            }

            const auto measured =
                antwika::gfx::textSize(node.text, node.textScale);

            // Nothing here can draw part of a glyph.
            // So a line too tall for its box is left out, not clipped.
            if (node.arranged.size.height < measured.height)
            {
                continue;
            }

            const auto cell =
                antwika::gfx::kGlyphAdvance * node.textScale;
            const auto cells =
                cell > 0 ? node.arranged.size.width / cell : 0U;

            if (cells == 0)
            {
                continue;
            }

            // substr takes what is there when asked for more.
            // The marker is for the unwind path alone.
            // It would destroy the temporaries if this append threw.
            commands.push_back(DrawText{ // GCOVR_EXCL_LINE
                .origin = node.arranged.origin,
                .text = node.text.substr(0, cells),
                .scale = node.textScale,
                .color = node.textColor});
        }
        return commands;
        // Only an unwind destroys commands at this brace.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

} // namespace antwika::ui::detail
