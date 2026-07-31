#include "Resolve.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>

#include "antwika/ui/Keyboard.hpp"
#include "antwika/ui/OptionChoice.hpp"
#include "antwika/ui/WidgetId.hpp"

#include "Contains.hpp"
#include "FocusRing.hpp"
#include "Interactive.hpp"

namespace antwika::ui::detail
{

    namespace
    {
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

        /**
         * @brief Stage one: what the pointer is on and what it pressed.
         *
         * Reads the arranged arena and the pointer, and writes the
         * hovered widget, the covered flag, and -- on a press -- the
         * activated widget and the chosen option.
         * Touches no node, so nothing it decides depends on anything a
         * later stage writes.
         *
         * @param tree The arranged arena.
         * @param pointer What the caller reports about the pointer.
         * @param interactions Receives this stage's answers.
         */
        void hitTest(
            const LayoutTree &tree,
            const Pointer &pointer,
            Interactions &interactions)
        {
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
            // So a press over no widget copies kNoWidget, no guard.
            if (pointer.pressed)
            {
                interactions.activated = interactions.hovered;
                interactions.chosen = option;
            }
        }

        /**
         * @brief Stage two: where focus ends up, and what Enter did.
         *
         * Runs after the hit-test because a press may take focus with
         * it, and before the dressing because the ring goes on whatever
         * this decides. Enter is reported through Interactions::
         * activated, which the hit-test may already have written, so
         * this stage adds to that answer rather than replacing it.
         *
         * @param tree The arena, for the tab order it declares.
         * @param keyboard The key edges, in arrival order.
         * @param focus The widget focused going in.
         * @param interactions Receives the focused widget, and the
         * activated one when Enter arrived.
         */
        void resolveFocus(
            const LayoutTree &tree,
            const Keyboard &keyboard,
            WidgetId focus,
            Interactions &interactions)
        {
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
            // kNoWidget is never listed, so nothing-focused stays put.
            if (!listed(focusables, focus))
            {
                focus = kNoWidget;
            }

            for (const auto key : keyboard.keys)
            {
                if (key == Key::Activate)
                {
                    // Reported through the field a press uses.
                    // So a click and a key land in one place.
                    if (focus != kNoWidget)
                    {
                        interactions.activated = focus;
                    }

                    continue;
                }

                // The editing keys belong to whatever is focused.
                // They are read where a text field is declared.
                // So focus only ever moves on the two that name it.
                if (key == Key::FocusNext || key == Key::FocusPrevious)
                {
                    focus =
                        step(focusables, focus, key == Key::FocusNext);
                }
            }

            interactions.focused = focus;
            // Only an unwind destroys focusables at this brace.
            // Nothing after its construction throws.
        } // GCOVR_EXCL_LINE

        /**
         * @brief Stage three: write every node's resolved appearance.
         *
         * The only stage that writes to the arena, and it reads nothing
         * out of it that the first two did not already decide. Both
         * appearances are written on every node rather than only on the
         * ones that changed, so resolving the same arena twice leaves it
         * saying the same thing.
         *
         * @param tree The arranged arena; backgrounds and rings are
         * written.
         * @param interactions What the first two stages decided.
         * @param down Whether a pointer button is being held.
         */
        void dress(
            LayoutTree &tree,
            const Interactions &interactions,
            bool down)
        {
            for (std::size_t index = 0; index < tree.size(); ++index)
            {
                auto &node = tree.node(index);

                if (node.style)
                {
                    // An unnamed widget can never be the hovered one.
                    // Comparing two kNoWidgets would say otherwise.
                    const bool under =
                        node.id != kNoWidget
                        && node.id == interactions.hovered;

                    node.background = fillFor(*node.style, under, down);

                    // Written beside the colour it explains.
                    // A hover pass reads it to step over a held widget.
                    node.pressed = under && down;
                }

                // The focused id is a listed one or kNoWidget.
                // No listed id is kNoWidget, so unnamed nodes stay bare.
                const bool focused =
                    interactions.focused != kNoWidget
                    && node.id == interactions.focused;

                node.focusRing = focused ? node.focusStyle
                                         : std::optional<FocusRing>{};
            }
        }
    } // namespace

    Interactions resolve(
        LayoutTree &tree,
        const Pointer &pointer,
        const Keyboard &keyboard,
        WidgetId focus)
    {
        // Three stages, and only this order works.
        // A press may move focus, so the pointer is read first.
        // The ring goes on whatever focus ended up being.
        // So focus is settled before anything is dressed.
        // Dressing is the only one that writes to the arena.
        Interactions interactions;

        hitTest(tree, pointer, interactions);
        resolveFocus(tree, keyboard, focus, interactions);
        dress(tree, interactions, pointer.down);

        return interactions;
        // Interactions carries an optional edit, and so a string.
        // Only an unwind destroys one at this brace.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

} // namespace antwika::ui::detail
