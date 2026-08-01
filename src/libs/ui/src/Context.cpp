#include "antwika/ui/Context.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "antwika/ui/Alignment.hpp"
#include "antwika/ui/Axis.hpp"
#include "antwika/ui/ButtonState.hpp"
#include "antwika/ui/HoverTargets.hpp"
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

    // Every Node carries a std::string.
    // So each one built here has an unwind path that frees it.
    // Nothing between building a Node and handing it over throws.
    // That is all the GCOVR_EXCL_LINE markers below cover.

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
    } // namespace

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
        // A button told how to look is dressed here and for good.
        // One left to work it out carries the colours instead.
        // resolve() picks between them once there is a layout.
        const auto state = spec.state.value_or(ButtonState::Idle);

        const auto style =
            spec.state ? std::optional<Interactive>{}
                       : std::optional<Interactive>{Interactive{
                             .idle = themeValue.buttonIdle,
                             .hovered = themeValue.buttonHovered,
                             .pressed = themeValue.buttonPressed}};

        // Every button carries one, named or not.
        // resolve() is the one place that skips the unnamed ones.
        // An id is what focus crosses back into application state as.
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

        // Growing room on both sides is what centres the label.
        // It comes out of the distribution the layout already does.
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
        // The root is what is open once every Scope has gone.
        // Anything else means one is still alive around this call.
        // What it holds open has widgets still to be put in it.
        // So the frame this would lay out is not the one written.
        if (tree->openIndex() != 0)
        {
            throw UiError{
                "antwika::ui::Context::finish: a container is still open"};
        }

        // Filled by the arranging pass rather than by a pass of its own.
        // So a reported rectangle is one the picture was drawn from.
        WidgetRects rects; // GCOVR_EXCL_LINE

        detail::layout(*tree, canvasSize, &rects);

        auto interactions = detail::resolve(
            *tree, pointerValue, keyboardValue, focusValue);

        // The edit was worked out where the field was declared.
        // It needs no layout, unlike everything the pointer decides.
        interactions.edit = pendingEdit;

        // Filled by the flattening pass, since a target names a command.
        // Reading them changes nothing; only applyHover() acts on them.
        HoverTargets hoverTargets; // GCOVR_EXCL_LINE

        auto commands = detail::flatten(*tree, &hoverTargets);

        return Frame{ // GCOVR_EXCL_LINE
            .commands = std::move(commands),
            .interactions = std::move(interactions),
            .rects = std::move(rects),
            .hoverTargets = std::move(hoverTargets)};
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

} // namespace antwika::ui
