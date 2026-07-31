#include "Resolve.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/ui/Keyboard.hpp"
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

        /**
         * @brief List what the keyboard can reach, in declaration order.
         *
         * Ascending index is the order the caller wrote the widgets in,
         * so this is the tab order and no second one has to be kept in
         * step with the layout.
         *
         * A repeated id is listed once: two nodes sharing an id are one
         * widget, so Tab must not stop at it twice.
         *
         * @param tree The arena, already laid out.
         * @return The focusable ids, in declaration order.
         */
        [[nodiscard]] std::vector<WidgetId> focusableIds(
            const LayoutTree &tree)
        {
            std::vector<WidgetId> ids;

            for (std::size_t index = 0; index < tree.size(); ++index)
            {
                const auto &node = tree.node(index);

                if (!node.focusStyle || node.id == kNoWidget)
                {
                    continue;
                }

                if (std::ranges::find(ids, node.id) == ids.end())
                {
                    ids.push_back(node.id);
                }
            }

            return ids;
            // Only an unwind destroys ids at this brace.
        } // GCOVR_EXCL_LINE

        /**
         * @brief Check whether an id is in the tab order.
         * @param ids The tab order.
         * @param id The id to look for.
         * @return True when the id is one the keyboard can reach.
         */
        [[nodiscard]] bool listed(
            const std::vector<WidgetId> &ids, WidgetId id)
        {
            return std::ranges::find(ids, id) != ids.end();
        }

        /**
         * @brief Move focus one widget along, wrapping at either end.
         *
         * @param ids The tab order.
         * @param from The widget focused now, which is either kNoWidget
         * or one of ids: resolve() drops anything else first.
         * @param forward True for Tab, false for Shift+Tab.
         * @return The widget to focus next.
         */
        [[nodiscard]] WidgetId step(
            const std::vector<WidgetId> &ids, WidgetId from, bool forward)
        {
            if (ids.empty())
            {
                return kNoWidget;
            }

            // Nothing focused yet, so both arrive from outside.
            // Tab takes the first widget and Shift+Tab the last.
            if (from == kNoWidget)
            {
                return forward ? ids.front() : ids.back();
            }

            const auto at = std::ranges::find(ids, from);
            const auto index = static_cast<std::size_t>(at - ids.begin());
            const auto next = forward ? index + 1 : index + ids.size() - 1;

            return ids[next % ids.size()];
        }
    } // namespace

    Interactions resolve(
        LayoutTree &tree,
        const Pointer &pointer,
        const Keyboard &keyboard,
        WidgetId focus)
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

        // Nothing hovered means there is nothing to activate.
        // So a press over no widget copies kNoWidget, needing no guard.
        if (pointer.pressed)
        {
            interactions.activated = interactions.hovered;
        }

        // Focus is in play once the caller has some or sends a key.
        // Only then does the pointer take focus along with it.
        // So tabbing on carries on from the button that was clicked.
        // A caller using the pointer alone is left exactly as it was.
        const bool focusInPlay =
            focus != kNoWidget || !keyboard.keys.empty();

        if (focusInPlay && interactions.activated != kNoWidget)
        {
            focus = interactions.activated;
        }

        const auto focusables = focusableIds(tree);

        // A focus on a widget this frame did not declare is dropped.
        // The layout is described afresh, so a widget gone is gone.
        // kNoWidget is never listed, so nothing-focused stays as it was.
        if (!listed(focusables, focus))
        {
            focus = kNoWidget;
        }

        for (const auto key : keyboard.keys)
        {
            if (key == Key::Activate)
            {
                // Reported through the field a press already uses.
                // So a click and a keystroke are handled in one place.
                if (focus != kNoWidget)
                {
                    interactions.activated = focus;
                }

                continue;
            }

            focus = step(focusables, focus, key == Key::FocusNext);
        }

        interactions.focused = focus;

        for (std::size_t index = 0; index < tree.size(); ++index)
        {
            auto &node = tree.node(index);

            if (node.style)
            {
                // An unnamed widget can never be the hovered one.
                // Comparing two kNoWidgets would say otherwise.
                const bool under = node.id != kNoWidget
                                   && node.id == interactions.hovered;

                node.background =
                    fillFor(*node.style, under, pointer.down);
            }

            // focus is a listed id or kNoWidget.
            // No listed id is kNoWidget, so unnamed nodes stay bare.
            if (focus != kNoWidget && node.id == focus)
            {
                node.focusRing = node.focusStyle;
            }
        }

        return interactions;
    }

} // namespace antwika::ui::detail
