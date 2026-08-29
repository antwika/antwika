#include "antwika/ui/Context.hpp"

#include <algorithm>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "antwika/ui/Alignment.hpp"
#include "antwika/ui/Axis.hpp"
#include "antwika/ui/ButtonState.hpp"
#include "antwika/ui/HoverTargets.hpp"
#include "antwika/ui/OccluderRects.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/TextWrap.hpp"
#include "antwika/ui/UiError.hpp"
#include "antwika/ui/WidgetRects.hpp"

#include "BuildDrawList.hpp"
#include "FocusRing.hpp"
#include "StateColors.hpp"
#include "Layout.hpp"
#include "LayoutTree.hpp"
#include "Node.hpp"
#include "NodeKind.hpp"
#include "Resolve.hpp"

namespace antwika::ui
{

    namespace
    {
        using detail::StateColors;
        using detail::Node;

        Color getButtonFill(const Theme &theme, ButtonState state) noexcept
        {
            if (state == ButtonState::Hovered)
            {
                return theme.buttonHoveredColor;
            }

            if (state == ButtonState::Pressed)
            {
                return theme.buttonPressedColor;
            }

            return theme.buttonIdleColor;
        }

        [[nodiscard]] std::size_t getPanesUnder(
            const detail::LayoutTree &tree,
            const std::size_t index,
            const std::size_t divider)
        {
            std::size_t panes = 0;

            for (auto child = tree.getNode(index).firstChild;
                 child != detail::kNoNode;
                 child = tree.getNode(child).nextSibling)
            {
                if (child != divider)
                {
                    ++panes;
                }
            }

            return panes;
        }

        void requireTwoPanes(const detail::LayoutTree &tree)
        {
            for (std::size_t index = 0; index < tree.getSize(); ++index)
            {
                const auto &node = tree.getNode(index);

                if (!node.splitInfo)
                {
                    continue;
                }

                if (getPanesUnder(tree, index, node.splitInfo->divider) != 2)
                {
                    throw UiError{
                        "antwika::ui::Context::build: a split needs "
                        "exactly two panes"};
                }
            }
        }

        [[nodiscard]] OccluderRects occludersOf(const detail::LayoutTree &tree)
        {
            OccluderRects foundRects;

            for (std::size_t index = 0; index < tree.getSize(); ++index)
            {
                const auto &node = tree.getNode(index);

                if (node.overlay)
                {
                    foundRects.push_back(node.arrangedRect);
                }
            }

            return foundRects;
        } // GCOVR_EXCL_LINE
    }

    Context::Context(
        Size canvasSize,
        Theme theme,
        Pointer pointer,
        Keyboard keyboard,
        WidgetId focusWidget)
        : canvasSize{canvasSize},
          themeValue{theme},
          pointerValue{std::move(pointer)},
          keyboardValue{std::move(keyboard)},
          focusedWidget{focusWidget},
          tree{std::make_unique<detail::LayoutTree>(Node{ // GCOVR_EXCL_LINE
              .axis = Axis::Column,
              .widthSizing = kGrowSizing,
              .heightSizing = kGrowSizing,
              .gap = theme.gap})}
    {
    }

    Context::~Context() = default;

    const Theme &Context::getTheme() const noexcept
    {
        return themeValue;
    }

    void Context::setTheme(Theme theme) noexcept
    {
        themeValue = theme;
    }

    ContainerScope Context::row(ContainerSpec spec)
    {
        return openContainer(Axis::Row, spec);
    }

    ContainerScope Context::column(ContainerSpec spec)
    {
        return openContainer(Axis::Column, spec);
    }

    ContainerScope Context::panel(ContainerSpec spec)
    {
        if (!spec.backgroundColor)
        {
            spec.backgroundColor = themeValue.panelColor;
        }

        if (!spec.padding)
        {
            spec.padding = themeValue.padding;
        }

        return column(spec);
    }

    void Context::label(std::string_view text)
    {
        label(text, themeValue.textColor);
    }

    void Context::label(std::string_view text, Color color)
    {
        tree->add(Node{ // GCOVR_EXCL_LINE
            .kind = detail::NodeKind::Text,
            .widthSizing = kFitSizing,
            .heightSizing = kFitSizing,
            .text = std::string{text}, // GCOVR_EXCL_LINE
            .textScale = antwika::gfx::TextScale{
                .face = themeValue.face,
                .multiplier = themeValue.textScale},
            .textColor = color});
    }

    void Context::button(std::string_view text, ButtonSpec spec)
    {
        const auto state = spec.state.value_or(ButtonState::Idle);

        const auto style =
            spec.state || spec.fillColor
                ? std::optional<StateColors>{}
                : std::optional<StateColors>{StateColors{
                      .idleColor = themeValue.buttonIdleColor,
                      .hoveredColor = themeValue.buttonHoveredColor,
                      .pressedColor = themeValue.buttonPressedColor}};

        const detail::FocusRing ring{
            .color = themeValue.focusRingColor,
            .thickness = themeValue.focusRingThickness};

        const auto wrapsText = spec.wrapWidth.has_value();

        tree->open(Node{ // GCOVR_EXCL_LINE
            .axis = wrapsText ? Axis::Column : Axis::Row,
            .widthSizing = spec.widthSizing,
            .heightSizing = kFitSizing,
            .crossAlignment = Alignment::Center,
            .padding = themeValue.buttonPadding,
            .backgroundColor =
                spec.fillColor.value_or(getButtonFill(themeValue, state)),
            .widgetId = spec.widgetId,
            .styleColors = style,
            .focusStyle = ring});

        if (wrapsText)
        {
            for (const auto line :
                 getWrapText(text, getWrapColumns(themeValue, *spec.wrapWidth)))
            {
                label(line, themeValue.buttonTextColor);
            }
        }
        else
        {
            if (spec.labelAlignment != Alignment::Start)
            {
                spacer(kGrowSizing);
            }

            label(text, themeValue.buttonTextColor);

            if (spec.labelAlignment != Alignment::End)
            {
                spacer(kGrowSizing);
            }
        }

        closeContainer();
    }

    void Context::image(const Icon shownIcon, const Color tintColor)
    {
        tree->add(Node{ // GCOVR_EXCL_LINE
            .kind = detail::NodeKind::Image,
            .widthSizing = kFitSizing,
            .heightSizing = kFitSizing,
            .imageIcon = shownIcon,
            .tintColor = tintColor});
    }

    void Context::checkbox(const bool checked)
    {
        const auto side = themeValue.checkboxSize;
        const auto inset =
            std::min(themeValue.checkboxInset, side / 2);

        tree->open(Node{ // GCOVR_EXCL_LINE
            .axis = Axis::Row,
            .widthSizing = getFixedSize(side),
            .heightSizing = getFixedSize(side),
            .crossAlignment = Alignment::Center,
            .padding = inset,
            .backgroundColor = themeValue.fieldColor});

        if (checked)
        {
            tree->add(Node{ // GCOVR_EXCL_LINE
                .widthSizing = kGrowSizing,
                .heightSizing = kGrowSizing,
                .backgroundColor = themeValue.textColor});
        }

        closeContainer();
    }

    void Context::checkbox(
        const std::string_view text, const CheckboxSpec spec)
    {
        const detail::FocusRing ring{
            .color = themeValue.focusRingColor,
            .thickness = themeValue.focusRingThickness};

        tree->open(Node{ // GCOVR_EXCL_LINE
            .axis = Axis::Row,
            .widthSizing = spec.widthSizing,
            .heightSizing = kFitSizing,
            .crossAlignment = Alignment::Center,
            .padding = themeValue.buttonPadding,
            .gap = themeValue.gap,
            .backgroundColor = themeValue.buttonIdleColor,
            .widgetId = spec.widgetId,
            .styleColors =
                StateColors{
                    .idleColor = themeValue.buttonIdleColor,
                    .hoveredColor = themeValue.buttonHoveredColor,
                    .pressedColor = themeValue.buttonPressedColor},
            .focusStyle = ring});

        checkbox(spec.checked);
        label(text, themeValue.buttonTextColor);

        closeContainer();
    }

    void Context::iconButton(const Icon shownIcon, ButtonSpec spec)
    {
        const auto state = spec.state.value_or(ButtonState::Idle);

        const auto style =
            spec.state ? std::optional<StateColors>{}
                       : std::optional<StateColors>{StateColors{
                             .idleColor = themeValue.buttonIdleColor,
                             .hoveredColor = themeValue.buttonHoveredColor,
                             .pressedColor = themeValue.buttonPressedColor}};

        const detail::FocusRing ring{
            .color = themeValue.focusRingColor,
            .thickness = themeValue.focusRingThickness};

        tree->open(Node{ // GCOVR_EXCL_LINE
            .axis = Axis::Row,
            .widthSizing = spec.widthSizing,
            .heightSizing = kFitSizing,
            .crossAlignment = Alignment::Center,
            .padding = themeValue.buttonPadding,
            .backgroundColor = getButtonFill(themeValue, state),
            .widgetId = spec.widgetId,
            .styleColors = style,
            .focusStyle = ring});

        spacer(kGrowSizing);
        image(shownIcon, themeValue.buttonTextColor);
        spacer(kGrowSizing);

        closeContainer();
    }

    void Context::spacer(Sizing alongSizing)
    {
        const auto axis = tree->getNode(tree->getOpenIndex()).axis;

        Node node{.axis = axis}; // GCOVR_EXCL_LINE

        if (node.axis == Axis::Row)
        {
            node.widthSizing = alongSizing;
            node.heightSizing = kFitSizing;
        }
        else
        {
            node.widthSizing = kFitSizing;
            node.heightSizing = alongSizing;
        }

        tree->add(std::move(node));
    }

    Frame Context::build()
    {
        if (tree->getOpenIndex() != 0)
        {
            throw UiError{
                "antwika::ui::Context::build: a container is still open"};
        }

        requireTwoPanes(*tree);

        WidgetRects rects; // GCOVR_EXCL_LINE

        detail::layout(*tree, canvasSize, &rects);

        auto interactions = detail::resolve(
            *tree,
            pointerValue,
            keyboardValue,
            focusedWidget,
            pendingEdit,
            themeValue.sliderThumbWidth);

        interactions.edit = pendingEdit;

        HoverTargets hoverTargets; // GCOVR_EXCL_LINE

        auto commands = detail::createDrawList(*tree, &hoverTargets);

        return Frame{ // GCOVR_EXCL_LINE
            .drawList = std::move(commands),
            .interactions = std::move(interactions),
            .rects = std::move(rects),
            .hoverTargets = std::move(hoverTargets),
            .overlayRects = occludersOf(*tree)};
    }

    void Context::closeContainer()
    {
        if (!openScrolls.empty()
            && tree->getOpenIndex() == openScrolls.back().viewport)
        {
            finishScrollColumn();

            return;
        }

        tree->close();
    }

    ContainerScope Context::openContainer(Axis axis, const ContainerSpec &spec)
    {
        tree->open(Node{ // GCOVR_EXCL_LINE
            .axis = axis,
            .widthSizing = spec.widthSizing,
            .heightSizing = spec.heightSizing,
            .crossAlignment = spec.crossAlignment,
            .padding = spec.padding.value_or(0),
            .gap = spec.gap.value_or(themeValue.gap),
            .backgroundColor = spec.backgroundColor,
            .widgetId = spec.widgetId,
            .clips = spec.clips});

        return ContainerScope{*this};
    }

}
