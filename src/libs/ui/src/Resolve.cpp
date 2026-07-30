#include "Resolve.hpp"

#include <cstddef>
#include <cstdint>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/ui/WidgetId.hpp"

#include "Interactive.hpp"

namespace antwika::ui::detail
{

    namespace
    {
        using antwika::gfx::Point;

        [[nodiscard]] bool contains(const Rect &rect, Point point) noexcept
        {
            // In 64 bits because a right edge is an int32 origin plus a
            // uint32 extent, which is exactly the sum that wraps.
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

        if (pointer.position)
        {
            for (std::size_t index = tree.size(); index-- > 0;)
            {
                const auto &node = tree.node(index);

                if (!contains(node.arranged, *pointer.position))
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
            }
        }

        // Nothing hovered means nothing to activate, so a press with the
        // pointer over no widget copies kNoWidget and needs no guard.
        if (pointer.pressed)
        {
            interactions.activated = interactions.hovered;
        }

        for (std::size_t index = 0; index < tree.size(); ++index)
        {
            auto &node = tree.node(index);

            if (!node.style)
            {
                continue;
            }

            // An unnamed widget can never be the hovered one, and
            // comparing two kNoWidgets would say otherwise.
            const bool under = node.id != kNoWidget
                               && node.id == interactions.hovered;

            node.background = fillFor(*node.style, under, pointer.down);
        }

        return interactions;
    }

} // namespace antwika::ui::detail
