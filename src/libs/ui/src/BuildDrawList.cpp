#include "BuildDrawList.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/text/TextLayout.hpp>

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
            DrawList &drawList, const Rect &boxRect, const FocusRing &ring)
        {
            const auto width = boxRect.size.width;
            const auto height = boxRect.size.height;
            const auto ringHeight = std::min(ring.thickness, height);
            const auto ringWidth = std::min(ring.thickness, width);

            const auto left = boxRect.originPoint.x;
            const auto top = boxRect.originPoint.y;

            for (const auto &bar : {
                     Rect{
                         .originPoint = {.x = left, .y = top},
                         .size = {.width = width, .height = ringHeight}},
                     Rect{
                         .originPoint =
                             {.x = left,
                              .y = offset(top, height, ringHeight)},
                         .size = {.width = width, .height = ringHeight}},
                     Rect{
                         .originPoint = {.x = left, .y = top},
                         .size = {.width = ringWidth, .height = height}},
                     Rect{
                         .originPoint =
                             {.x = offset(left, width, ringWidth), .y = top},
                         .size = {.width = ringWidth, .height = height}}})
            {
                drawList.push_back(
                    FillRect{.rect = bar, .color = ring.color});
            }
        }

        [[nodiscard]] std::size_t subtreeEnd(
            const LayoutTree &tree, const std::size_t index)
        {
            auto commandIndex = index;

            while (commandIndex != kNoNode)
            {
                if (tree.node(commandIndex).nextSibling != kNoNode)
                {
                    return tree.node(commandIndex).nextSibling;
                }

                commandIndex = tree.node(commandIndex).parent;
            }

            return tree.size();
        }

        void emitLayer(
            const LayoutTree &tree,
            bool overlay,
            DrawList &drawList,
            HoverTargets *targets)
        {
            std::vector<std::size_t> clippedNodes;

            for (std::size_t index = 0; index < tree.size(); ++index)
            {
                while (!clippedNodes.empty()
                       && clippedNodes.back() == index)
                {
                    drawList.push_back(PopClip{});
                    clippedNodes.pop_back();
                }

                const auto &node = tree.node(index);

                if (node.overlay != overlay)
                {
                    continue;
                }

                if (node.clips)
                {
                    drawList.push_back(
                        PushClip{.rect = node.arrangedRect});
                    clippedNodes.push_back(subtreeEnd(tree, index));
                }

                if (node.backgroundColor)
                {
                    if (targets != nullptr && node.styleColors
                        && node.widgetId != kNoWidget)
                    {
                        targets->push_back(HoverTarget{
                            .widgetId = node.widgetId,
                            .rect = node.arrangedRect,
                            .command = drawList.size(),
                            .idleColor = node.styleColors->idleColor,
                            .hoveredColor = node.styleColors->hoveredColor,
                            .held = node.pressed});
                    }

                    drawList.push_back(FillRect{
                        .rect = node.arrangedRect,
                        .color = *node.backgroundColor});
                }

                if (node.kind == NodeKind::Image)
                {
                    if (node.imageIcon.sheetTexture != nullptr)
                    {
                        drawList.push_back(DrawTexture{
                            .texture = node.imageIcon.sheetTexture,
                            .sourceRect = node.imageIcon.sourceRect,
                            .destinationRect = node.arrangedRect,
                            .tintColor = node.tintColor});
                    }

                    continue;
                }

                if (node.kind != NodeKind::Text)
                {
                    continue;
                }

                const auto measuredSize =
                    antwika::text::textSize(node.text, node.textScale);

                if (node.arrangedRect.size.height < measuredSize.height)
                {
                    continue;
                }

                const auto cell =
                    antwika::gfx::scaledGlyphAdvance(node.textScale);
                const auto cells =
                    cell > 0 ? node.arrangedRect.size.width / cell : 0U;

                if (cells == 0)
                {
                    continue;
                }

                drawList.push_back(DrawText{ // GCOVR_EXCL_LINE
                    .originPoint = node.arrangedRect.originPoint,
                    .text = node.text.substr(0, cells),
                    .scale = node.textScale,
                    .color = node.textColor});
            }

            while (!clippedNodes.empty())
            {
                drawList.push_back(PopClip{});
                clippedNodes.pop_back();
            }

            for (std::size_t index = 0; index < tree.size(); ++index)
            {
                const auto &node = tree.node(index);

                if (node.overlay != overlay || !node.focusRing
                    || node.focusRing->thickness == 0)
                {
                    continue;
                }

                appendRing(drawList, node.arrangedRect, *node.focusRing);
            }
        }
    }

    DrawList buildDrawList(const LayoutTree &tree, HoverTargets *targets)
    {
        DrawList drawList;

        emitLayer(tree, false, drawList, targets);
        emitLayer(tree, true, drawList, targets);

        return drawList;
    } // GCOVR_EXCL_LINE

}
