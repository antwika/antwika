#include "Flatten.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/TextLayout.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/ui/DrawCommand.hpp"

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
            // An int32 origin plus a uint32 extent is what wraps.
            // So it is done wide, as Resolve's hit-test does it.
            const auto edge = static_cast<std::int64_t>(start)
                              + extent - bar;

            return static_cast<std::int32_t>(edge);
        }

        /**
         * @brief Append the four bars that make one focus border.
         *
         * antwika::gfx has no stroke and no scissor, so a border is
         * drawn rather than described: two full-width bars and two
         * full-height ones, overlapping at the corners, which is one
         * colour over itself and so invisible.
         *
         * Each bar is clamped to the box it surrounds, so a widget
         * thinner than the ring is filled by it instead of being
         * escaped by it.
         *
         * @param commands The picture to append to.
         * @param box The widget's arranged area.
         * @param ring The colour and thickness to draw.
         */
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
    } // namespace

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

        // Every border comes last, not after the widget it rings.
        // A container declared later would otherwise cover it.
        // One widget is focused, so this only tops that one's edges.
        for (std::size_t index = 0; index < tree.size(); ++index)
        {
            const auto &node = tree.node(index);

            if (!node.focusRing || node.focusRing->thickness == 0)
            {
                continue;
            }

            appendRing(commands, node.arranged, *node.focusRing);
        }

        return commands;
        // Only an unwind destroys commands at this brace.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

} // namespace antwika::ui::detail
