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
#include "Interactive.hpp"
#include "LayoutTree.hpp"
#include "Node.hpp"

namespace antwika::ui
{

    namespace
    {
        using detail::FocusRing;
        using detail::Interactive;
        using detail::Node;

        constexpr std::string_view kMarker = "v";

        WidgetId optionId(WidgetId base, std::size_t index) noexcept
        {
            if (base == kNoWidget)
            {
                return kNoWidget;
            }

            return WidgetId{
                static_cast<std::uint64_t>(base)
                + static_cast<std::uint64_t>(index)};
        }
    }

    void Context::dropdown(const DropdownSpec &spec)
    {
        const Interactive style{
            .idle = themeValue.buttonIdle,
            .hovered = themeValue.buttonHovered,
            .pressed = themeValue.buttonPressed};

        const FocusRing ring{
            .color = themeValue.focusRing,
            .thickness = themeValue.focusRingThickness};

        const auto anchor = tree->open(Node{ // GCOVR_EXCL_LINE
            .axis = Axis::Row,
            .width = spec.width,
            .height = kFit,
            .cross = Alignment::Center,
            .padding = themeValue.buttonPadding,
            .gap = themeValue.gap,
            .background = themeValue.buttonIdle,
            .id = spec.id,
            .style = style,
            .focusStyle = ring});

        const auto shown = spec.selected < spec.options.size()
                               ? spec.options[spec.selected]
                               : spec.placeholder;

        label(shown, themeValue.buttonText);
        spacer(kGrow);
        label(kMarker, themeValue.buttonText);

        closeContainer();

        if (!spec.open)
        {
            return;
        }

        const auto first = tree->open(Node{ // GCOVR_EXCL_LINE
            .axis = Axis::Column,
            .width = kFit,
            .height = kFit,
            .gap = 0,
            .background = themeValue.panel,
            .overlayAnchor = anchor});

        for (std::size_t index = 0; index < spec.options.size(); ++index)
        {
            tree->open(Node{ // GCOVR_EXCL_LINE
                .axis = Axis::Row,
                .width = kGrow,
                .height = kFit,
                .cross = Alignment::Center,
                .padding = themeValue.buttonPadding,
                .background = themeValue.buttonIdle,
                .id = optionId(spec.optionIdBase, index),
                .style = style,
                .focusStyle = ring,
                .optionOwner = spec.id,
                .optionIndex = index});

            label(spec.options[index], themeValue.buttonText);

            closeContainer();
        }

        closeContainer();

        for (auto index = first; index < tree->size(); ++index)
        {
            tree->node(index).overlay = true;
        }
    }

}
