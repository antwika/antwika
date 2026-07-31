#include "Resolve.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/ui/OptionChoice.hpp"
#include "antwika/ui/WidgetId.hpp"

#include "Interactive.hpp"

namespace antwika::ui::detail
{

    namespace
    {
        using antwika::gfx::Point;

        [[nodiscard]] bool contains(const Rect &rect, Point point) noexcept
        {
            // A right edge is an int32 origin plus a uint32 extent.
            // That is exactly the sum that wraps, hence 64 bits.
            const auto left = static_cast<std::int64_t>(rect.origin.x);
            const auto top = static_cast<std::int64_t>(rect.origin.y);
            const auto x = static_cast<std::int64_t>(point.x);
            const auto y = static_cast<std::int64_t>(point.y);

            // Half-open, so two touching rectangles cannot both be hit.
            // A collapsed one is therefore hit by nothing.
            return x >= left && x < left + rect.size.width && y >= top
                   && y < top + rect.size.height;
        }

        [[nodiscard]] Color fillFor(
            const Interactive &style, bool under, bool down) noexcept
        {
            if (!under)
            {
                return style.idle;
            }

            return down ? style.pressed : style.hovered;
        }
    } // namespace

    Interactions resolve(LayoutTree &tree, const Pointer &pointer)
    {
        Interactions interactions;

        std::optional<OptionChoice> option;

        // An overlay is painted after everything else.
        // So it is in front, and so it is hit first.
        // Two passes are what say that.
        // One descending loop can only mean the arena's own order.
        const auto scan = [&](bool overlay) {
            for (std::size_t index = tree.size(); index-- > 0;)
            {
                const auto &node = tree.node(index);

                if (node.overlay != overlay
                    || !contains(node.arranged, *pointer.position))
                {
                    continue;
                }

                if (node.background)
                {
                    interactions.pointerOverUi = true;
                }

                if (node.id != kNoWidget
                    && interactions.hovered == kNoWidget)
                {
                    interactions.hovered = node.id;
                }

                // An option reports its index rather than its id.
                // So it is tracked apart from the hovered widget.
                // An unnamed option therefore still answers.
                if (node.optionOwner != kNoWidget && !option)
                {
                    option = OptionChoice{
                        .dropdown = node.optionOwner,
                        .index = node.optionIndex};
                }
            }
        };

        if (pointer.position)
        {
            scan(true);
            scan(false);
        }

        // Nothing hovered means there is nothing to activate.
        // So a press over no widget copies kNoWidget, needing no guard.
        if (pointer.pressed)
        {
            interactions.activated = interactions.hovered;
            interactions.chosen = option;
        }

        for (std::size_t index = 0; index < tree.size(); ++index)
        {
            auto &node = tree.node(index);

            if (!node.style)
            {
                continue;
            }

            // An unnamed widget can never be the hovered one.
            // Comparing two kNoWidgets would say otherwise.
            const bool under = node.id != kNoWidget
                               && node.id == interactions.hovered;

            node.background = fillFor(*node.style, under, pointer.down);
        }

        return interactions;
        // Only an unwind destroys the reported edit at this brace.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

} // namespace antwika::ui::detail
