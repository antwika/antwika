#include "Flatten.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/TextLayout.hpp>

#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/HoverTarget.hpp"
#include "antwika/ui/WidgetId.hpp"

#include "FocusRing.hpp"
#include "NodeKind.hpp"

namespace antwika::ui::detail
{

    namespace
    {
        using antwika::gfx::Rect;

        [[nodiscard]] std::int32_t offset(
            std::int32_t start, std::uint32_t extent,
            std::uint32_t bar) noexcept
        {
            const auto edge = static_cast<std::int64_t>(start)
                              + extent - bar;

            return static_cast<std::int32_t>(edge);
        }

        void appendRing(
            DrawList &commands, const Rect &box, const FocusRing &ring)
        {
            const auto width = box.size.width;
            const auto height = box.size.height;
            const auto tall = std::min(ring.thickness, height);
            const auto wide = std::min(ring.thickness, width);

            const auto left = box.origin.x;
            const auto top = box.origin.y;

            for (const auto &bar : {
                     Rect{
                         .origin = {.x = left, .y = top},
                         .size = {.width = width, .height = tall}},
                     Rect{
                         .origin =
                             {.x = left,
                              .y = offset(top, height, tall)},
                         .size = {.width = width, .height = tall}},
                     Rect{
                         .origin = {.x = left, .y = top},
                         .size = {.width = wide, .height = height}},
                     Rect{
                         .origin =
                             {.x = offset(left, width, wide), .y = top},
                         .size = {.width = wide, .height = height}}})
            {
                commands.push_back(
                    FillRect{.rect = bar, .color = ring.color});
            }
        }

        void emitLayer(
            const LayoutTree &tree,
            bool overlay,
            DrawList &commands,
            HoverTargets *targets)
        {
            for (std::size_t index = 0; index < tree.size(); ++index)
            {
                const auto &node = tree.node(index);

                if (node.overlay != overlay)
                {
                    continue;
                }

                if (node.background)
                {
                    if (targets != nullptr && node.style
                        && node.id != kNoWidget)
                    {
                        targets->push_back(HoverTarget{
                            .id = node.id,
                            .rect = node.arranged,
                            .command = commands.size(),
                            .idle = node.style->idle,
                            .hovered = node.style->hovered,
                            .held = node.pressed});
                    }

                    commands.push_back(FillRect{
                        .rect = node.arranged,
                        .color = *node.background});
                }

                if (node.kind != NodeKind::Text)
                {
                    continue;
                }

                const auto measured =
                    antwika::gfx::textSize(node.text, node.textScale);

                if (node.arranged.size.height < measured.height)
                {
                    continue;
                }

                const auto cell =
                    antwika::gfx::scaledGlyphAdvance(node.textScale);
                const auto cells =
                    cell > 0 ? node.arranged.size.width / cell : 0U;

                if (cells == 0)
                {
                    continue;
                }

                commands.push_back(DrawText{ // GCOVR_EXCL_LINE
                    .origin = node.arranged.origin,
                    .text = node.text.substr(0, cells),
                    .scale = node.textScale,
                    .color = node.textColor});
            }

            for (std::size_t index = 0; index < tree.size(); ++index)
            {
                const auto &node = tree.node(index);

                if (node.overlay != overlay || !node.focusRing
                    || node.focusRing->thickness == 0)
                {
                    continue;
                }

                appendRing(commands, node.arranged, *node.focusRing);
            }
        }
    }

    DrawList flatten(const LayoutTree &tree, HoverTargets *targets)
    {
        DrawList commands;

        emitLayer(tree, false, commands, targets);
        emitLayer(tree, true, commands, targets);

        return commands;
    } // GCOVR_EXCL_LINE

}
