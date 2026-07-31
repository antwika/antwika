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

    // Every Node carries a std::string, as Context.cpp says.
    // That is all the GCOVR_EXCL_LINE markers below cover.

    namespace
    {
        using detail::FocusRing;
        using detail::Interactive;
        using detail::Node;

        // The mark on the closed box, in the font this library has.
        constexpr std::string_view kMarker = "v";

        /**
         * @brief Work out what one option is called.
         * @param base The id the first option carries.
         * @param index Which option this is.
         * @return The option's id; kNoWidget when the options were left
         * unnamed, which only costs them their hover appearance.
         */
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
    } // namespace

    void Context::dropdown(const DropdownSpec &spec)
    {
        const Interactive style{
            .idle = themeValue.buttonIdle,
            .hovered = themeValue.buttonHovered,
            .pressed = themeValue.buttonPressed};

        // The box and every option are stops in the tab order.
        // A closed list has one stop, and an open one has its options.
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

        // Anything outside the options shows the placeholder.
        // So kNoOption needs no arm of its own.
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

        // The list is an overlay.
        // It is out of its parent's flow.
        // It hangs beneath the box it dropped from.
        // It is painted after every other command, and hit before them.
        // That is the only depth antwika::gfx allows anybody.
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

        // Marked after the fact rather than at each call.
        // A subtree is appended in one run.
        // And label() knows nothing of overlays.
        for (auto index = first; index < tree->size(); ++index)
        {
            tree->node(index).overlay = true;
        }
    }

} // namespace antwika::ui
