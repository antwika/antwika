#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include "antwika/ui/Alignment.hpp"
#include "antwika/ui/Axis.hpp"
#include "antwika/ui/Context.hpp"
#include "antwika/ui/DropdownSpec.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/WidgetId.hpp"

#include "FocusRing.hpp"
#include "StateColors.hpp"
#include "LayoutTree.hpp"
#include "Node.hpp"

namespace antwika::ui
{

    namespace
    {
        using detail::FocusRing;
        using detail::StateColors;
        using detail::Node;

        constexpr std::string_view kMarker = "v";

        WidgetId optionId(WidgetId baseWidget, std::size_t index) noexcept
        {
            if (baseWidget == kNoWidget)
            {
                return kNoWidget;
            }

            return WidgetId{
                static_cast<std::uint64_t>(baseWidget)
                + static_cast<std::uint64_t>(index)};
        }
    }

    void Context::dropdown(const DropdownSpec &spec)
    {
        const StateColors styleColors{
            .idleColor = themeValue.buttonIdleColor,
            .hoveredColor = themeValue.buttonHoveredColor,
            .pressedColor = themeValue.buttonPressedColor};

        const FocusRing ring{
            .color = themeValue.focusRingColor,
            .thickness = themeValue.focusRingThickness};

        const auto anchor = tree->open(Node{ // GCOVR_EXCL_LINE
            .axis = Axis::Row,
            .widthSizing = spec.widthSizing,
            .heightSizing = kFitSizing,
            .crossAlignment = Alignment::Center,
            .padding = themeValue.buttonPadding,
            .gap = themeValue.gap,
            .backgroundColor = themeValue.buttonIdleColor,
            .widgetId = spec.widgetId,
            .styleColors = styleColors,
            .focusStyle = ring});

        const auto shownText = spec.selectedIndex < spec.options.size()
                             ? spec.options[spec.selectedIndex]
                             : spec.placeholder;

        label(shownText, themeValue.buttonTextColor);
        spacer(kGrowSizing);
        label(kMarker, themeValue.buttonTextColor);

        closeContainer();

        if (!spec.open)
        {
            return;
        }

        const auto first = tree->open(Node{ // GCOVR_EXCL_LINE
            .axis = Axis::Column,
            .widthSizing = kFitSizing,
            .heightSizing = kFitSizing,
            .gap = 0,
            .backgroundColor = themeValue.panelColor,
            .overlayAnchor = anchor});

        for (std::size_t index = 0; index < spec.options.size(); ++index)
        {
            tree->open(Node{ // GCOVR_EXCL_LINE
                .axis = Axis::Row,
                .widthSizing = kGrowSizing,
                .heightSizing = kFitSizing,
                .crossAlignment = Alignment::Center,
                .padding = themeValue.buttonPadding,
                .backgroundColor = themeValue.buttonIdleColor,
                .widgetId = optionId(spec.optionIdBaseWidget, index),
                .styleColors = styleColors,
                .focusStyle = ring,
                .optionOwnerWidget = spec.widgetId,
                .optionIndex = index});

            const auto mark = index < spec.markedOptions.size()
                            ? spec.markedOptions[index]
                            : OptionMark::None;

            if (!spec.markedOptions.empty() && mark != OptionMark::None)
            {
                checkbox(mark == OptionMark::On);
            }

            label(spec.options[index], themeValue.buttonTextColor);

            closeContainer();
        }

        closeContainer();

        for (auto index = first; index < tree->size(); ++index)
        {
            tree->node(index).overlay = true;
        }
    }

}
