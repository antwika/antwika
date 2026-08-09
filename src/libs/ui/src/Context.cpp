#include "antwika/ui/Context.hpp"

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
#include "antwika/ui/Overlays.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/UiError.hpp"
#include "antwika/ui/WidgetRects.hpp"

#include "Flatten.hpp"
#include "FocusRing.hpp"
#include "Interactive.hpp"
#include "Layout.hpp"
#include "LayoutTree.hpp"
#include "Node.hpp"
#include "NodeKind.hpp"
#include "Resolve.hpp"

namespace antwika::ui
{

    namespace
    {
        using detail::Interactive;
        using detail::Node;

        Color buttonFill(const Theme &theme, ButtonState state) noexcept
        {
            if (state == ButtonState::Hovered)
            {
                return theme.buttonHovered;
            }

            if (state == ButtonState::Pressed)
            {
                return theme.buttonPressed;
            }

            return theme.buttonIdle;
        }

        [[nodiscard]] std::size_t panesUnder(
            const detail::LayoutTree &tree,
            const std::size_t index,
            const std::size_t divider)
        {
            std::size_t panes = 0;

            for (auto child = tree.node(index).firstChild;
                 child != detail::kNoNode;
                 child = tree.node(child).nextSibling)
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
            for (std::size_t index = 0; index < tree.size(); ++index)
            {
                const auto &node = tree.node(index);

                if (!node.splitInfo)
                {
                    continue;
                }

                if (panesUnder(tree, index, node.splitInfo->divider) != 2)
                {
                    throw UiError{
                        "antwika::ui::Context::finish: a split needs "
                        "exactly two panes"};
                }
            }
        }

        [[nodiscard]] Overlays overlaysOf(const detail::LayoutTree &tree)
        {
            Overlays found;

            for (std::size_t index = 0; index < tree.size(); ++index)
            {
                const auto &node = tree.node(index);

                if (node.overlay)
                {
                    found.push_back(node.arranged);
                }
            }

            return found;
        } // GCOVR_EXCL_LINE
    }

    Context::Context(
        Size canvas,
        Theme theme,
        Pointer pointer,
        Keyboard keyboard,
        WidgetId focus)
        : canvasSize{canvas},
          themeValue{theme},
          pointerValue{std::move(pointer)},
          keyboardValue{std::move(keyboard)},
          focusValue{focus},
          tree{std::make_unique<detail::LayoutTree>(Node{ // GCOVR_EXCL_LINE
              .axis = Axis::Column,
              .width = kGrow,
              .height = kGrow,
              .gap = theme.gap})}
    {
    }

    Context::~Context() = default;

    const Theme &Context::theme() const noexcept
    {
        return themeValue;
    }

    Scope Context::row(ContainerSpec spec)
    {
        return openContainer(Axis::Row, spec);
    }

    Scope Context::column(ContainerSpec spec)
    {
        return openContainer(Axis::Column, spec);
    }

    Scope Context::panel(ContainerSpec spec)
    {
        if (!spec.background)
        {
            spec.background = themeValue.panel;
        }

        if (!spec.padding)
        {
            spec.padding = themeValue.padding;
        }

        return column(spec);
    }

    void Context::label(std::string_view text)
    {
        label(text, themeValue.text);
    }

    void Context::label(std::string_view text, Color color)
    {
        tree->add(Node{ // GCOVR_EXCL_LINE
            .kind = detail::NodeKind::Text,
            .width = kFit,
            .height = kFit,
            .text = std::string{text}, // GCOVR_EXCL_LINE
            .textScale = themeValue.textScale,
            .textColor = color});
    }

    void Context::button(std::string_view text, ButtonSpec spec)
    {
        const auto state = spec.state.value_or(ButtonState::Idle);

        const auto style =
            spec.state ? std::optional<Interactive>{}
                       : std::optional<Interactive>{Interactive{
                             .idle = themeValue.buttonIdle,
                             .hovered = themeValue.buttonHovered,
                             .pressed = themeValue.buttonPressed}};

        const detail::FocusRing ring{
            .color = themeValue.focusRing,
            .thickness = themeValue.focusRingThickness};

        tree->open(Node{ // GCOVR_EXCL_LINE
            .axis = Axis::Row,
            .width = spec.width,
            .height = kFit,
            .cross = Alignment::Center,
            .padding = themeValue.buttonPadding,
            .background = buttonFill(themeValue, state),
            .id = spec.id,
            .style = style,
            .focusStyle = ring});

        spacer(kGrow);
        label(text, themeValue.buttonText);
        spacer(kGrow);

        closeContainer();
    }

    void Context::spacer(Sizing along)
    {
        const auto axis = tree->node(tree->openIndex()).axis;

        Node node{.axis = axis}; // GCOVR_EXCL_LINE

        if (node.axis == Axis::Row)
        {
            node.width = along;
            node.height = kFit;
        }
        else
        {
            node.width = kFit;
            node.height = along;
        }

        tree->add(std::move(node));
    }

    Frame Context::finish()
    {
        if (tree->openIndex() != 0)
        {
            throw UiError{
                "antwika::ui::Context::finish: a container is still open"};
        }

        requireTwoPanes(*tree);

        WidgetRects rects; // GCOVR_EXCL_LINE

        detail::layout(*tree, canvasSize, &rects);

        auto interactions = detail::resolve(
            *tree,
            pointerValue,
            keyboardValue,
            focusValue,
            pendingEdit,
            themeValue.sliderThumbWidth);

        interactions.edit = pendingEdit;

        HoverTargets hoverTargets; // GCOVR_EXCL_LINE

        auto commands = detail::flatten(*tree, &hoverTargets);

        return Frame{ // GCOVR_EXCL_LINE
            .commands = std::move(commands),
            .interactions = std::move(interactions),
            .rects = std::move(rects),
            .hoverTargets = std::move(hoverTargets),
            .overlays = overlaysOf(*tree)};
    }

    void Context::closeContainer() noexcept
    {
        tree->close();
    }

    Scope Context::openContainer(Axis axis, const ContainerSpec &spec)
    {
        tree->open(Node{ // GCOVR_EXCL_LINE
            .axis = axis,
            .width = spec.width,
            .height = spec.height,
            .cross = spec.cross,
            .padding = spec.padding.value_or(0),
            .gap = spec.gap.value_or(themeValue.gap),
            .background = spec.background,
            .id = spec.id});

        return Scope{*this};
    }

}
