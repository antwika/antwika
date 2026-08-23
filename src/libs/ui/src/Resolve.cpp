#include "Resolve.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "antwika/ui/Keyboard.hpp"
#include "antwika/ui/OptionChoice.hpp"
#include "antwika/ui/TextEdit.hpp"
#include "antwika/ui/WidgetId.hpp"

#include "Contains.hpp"
#include "ResolveWidgets.hpp"

namespace antwika::ui::detail
{

    namespace
    {
        [[nodiscard]] std::vector<WidgetId> getFocusableIds(
            const LayoutTree &tree)
        {
            std::vector<WidgetId> widgetIds;

            for (std::size_t index = 0; index < tree.getSize(); ++index)
            {
                const auto &node = tree.getNode(index);

                if (!node.focusStyle || node.widgetId == kNoWidget)
                {
                    continue;
                }

                if (std::ranges::find(widgetIds, node.widgetId)
                    == widgetIds.end())
                {
                    widgetIds.push_back(node.widgetId);
                }
            }

            return widgetIds;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] bool isListed(
            const std::vector<WidgetId> &widgetIds, WidgetId widgetId)
        {
            return std::ranges::find(widgetIds, widgetId) != widgetIds.end();
        }

        [[nodiscard]] WidgetId getStep(
            const std::vector<WidgetId> &widgetIds,
            WidgetId fromWidgetId,
            bool forward)
        {
            if (widgetIds.empty())
            {
                return kNoWidget;
            }

            if (fromWidgetId == kNoWidget)
            {
                return forward ? widgetIds.front() : widgetIds.back();
            }

            const auto foundWidget = std::ranges::find(widgetIds, fromWidgetId);
            const auto index =
                static_cast<std::size_t>(foundWidget - widgetIds.begin());
            const auto nextIndex =
                forward ? index + 1 : index + widgetIds.size() - 1;

            return widgetIds[nextIndex % widgetIds.size()];
        }

        [[nodiscard]] bool hitTest(
            const LayoutTree &tree,
            const Pointer &pointer,
            Interactions &interactions)
        {
            std::optional<OptionChoice> option;

            const auto scan = [&](bool overlay) {
                bool contained = false;

                for (std::size_t index = tree.getSize(); index-- > 0;)
                {
                    const auto &node = tree.getNode(index);

                    if (node.overlay != overlay
                        || !contains(node.arrangedRect, *pointer.positionPoint))
                    {
                        continue;
                    }

                    contained = true;

                    if (node.backgroundColor)
                    {
                        interactions.pointerOverUi = true;
                    }

                    if (node.widgetId != kNoWidget
                        && interactions.hoveredWidget == kNoWidget)
                    {
                        interactions.hoveredWidget = node.widgetId;
                    }

                    if (node.optionOwnerWidget != kNoWidget && !option)
                    {
                        option = OptionChoice{
                            .dropdownWidget = node.optionOwnerWidget,
                            .index = node.optionIndex};
                    }
                }

                return contained;
            };

            bool underOverlay = false;

            if (pointer.positionPoint)
            {
                underOverlay = scan(true);

                if (!underOverlay)
                {
                    scan(false);
                }
            }

            if (pointer.pressed)
            {
                interactions.activatedWidget = interactions.hoveredWidget;
                interactions.chosenChoice = option;
            }

            return underOverlay;
        }

        [[nodiscard]] std::optional<OptionChoice> optionFor(
            const LayoutTree &tree, WidgetId widget)
        {
            for (std::size_t index = 0; index < tree.getSize(); ++index)
            {
                const auto &node = tree.getNode(index);

                if (node.widgetId == widget
                    && node.optionOwnerWidget != kNoWidget)
                {
                    return OptionChoice{
                        .dropdownWidget = node.optionOwnerWidget,
                        .index = node.optionIndex};
                }
            }

            return std::nullopt;
        }

        void resolveFocus(
            const LayoutTree &tree,
            const Keyboard &keyboard,
            WidgetId focusWidget,
            Interactions &interactions)
        {
            const bool focusInPlay =
                focusWidget != kNoWidget || !keyboard.keys.empty();

            if (focusInPlay && interactions.activatedWidget != kNoWidget)
            {
                focusWidget = interactions.activatedWidget;
            }

            const auto focusables = getFocusableIds(tree);

            if (!isListed(focusables, focusWidget))
            {
                focusWidget = kNoWidget;
            }

            for (const auto key : keyboard.keys)
            {
                if (key == Key::Activate)
                {
                    if (focusWidget != kNoWidget)
                    {
                        interactions.activatedWidget = focusWidget;

                        if (auto option = optionFor(tree, focusWidget))
                        {
                            interactions.chosenChoice = option;
                        }
                    }

                    continue;
                }

                if (key == Key::FocusNext || key == Key::FocusPrevious)
                {
                    focusWidget =
                        getStep(focusables, focusWidget, key == Key::FocusNext);
                }
            }

            interactions.focusedWidget = focusWidget;
        } // GCOVR_EXCL_LINE
    }

    Interactions resolve(
        LayoutTree &tree,
        const Pointer &pointer,
        const Keyboard &keyboard,
        WidgetId focusWidget,
        std::optional<TextEdit> &edit,
        std::uint32_t thumbWidth)
    {
        Interactions interactions;

        const bool underOverlay = hitTest(tree, pointer, interactions);

        resolveFocus(tree, keyboard, focusWidget, interactions);
        resolveAreas(tree, pointer, underOverlay, interactions, edit);
        resolveRails(
            tree, pointer, underOverlay, thumbWidth, interactions);
        resolveBars(tree, pointer, underOverlay, interactions);
        applyVisualState(tree, interactions, pointer.down);

        return interactions;
    } // GCOVR_EXCL_LINE

}
