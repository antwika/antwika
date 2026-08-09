#pragma once

#include <optional>
#include <variant>

#include <antwika/gfx/Point.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputState.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/PointerHint.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/ui/HoverPointer.hpp>
#include <antwika/ui/Pointer.hpp>

namespace antwika::app
{

    using antwika::gfx::Point;
    using antwika::input::InputEvent;
    using antwika::input::InputState;
    using antwika::input::MouseButton;
    using antwika::input::PointerHint;
    using antwika::input::Position;
    using antwika::ui::HoverPointer;
    using antwika::ui::Pointer;

    [[nodiscard]] constexpr Point asPoint(Position position) noexcept
    {
        return Point{.x = position.x, .y = position.y};
    }

    [[nodiscard]] constexpr bool locates(const InputEvent &event) noexcept
    {
        using antwika::input::PointerButtonPressed;
        using antwika::input::PointerButtonReleased;
        using antwika::input::PointerMoved;

        return std::holds_alternative<PointerMoved>(event)
               || std::holds_alternative<PointerButtonPressed>(event)
               || std::holds_alternative<PointerButtonReleased>(event);
    }

    [[nodiscard]] constexpr const antwika::input::PointerButtonPressed *
    pressOf(const InputEvent &event, const MouseButton button) noexcept
    {
        const auto *pressed =
            std::get_if<antwika::input::PointerButtonPressed>(&event);

        return pressed != nullptr && pressed->button == button ? pressed
                                                               : nullptr;
    }

    [[nodiscard]] constexpr bool isPressOf(
        const InputEvent &event, const MouseButton button) noexcept
    {
        return pressOf(event, button) != nullptr;
    }

    [[nodiscard]] constexpr const antwika::input::PointerButtonPressed *
    leftPress(const InputEvent &event) noexcept
    {
        return pressOf(event, MouseButton::Left);
    }

    [[nodiscard]] constexpr bool isLeftPress(
        const InputEvent &event) noexcept
    {
        return isPressOf(event, MouseButton::Left);
    }

    [[nodiscard]] constexpr bool isReleaseOf(
        const InputEvent &event, const MouseButton button) noexcept
    {
        const auto *released =
            std::get_if<antwika::input::PointerButtonReleased>(&event);

        return released != nullptr && released->button == button;
    }

    [[nodiscard]] constexpr bool isLeftRelease(
        const InputEvent &event) noexcept
    {
        return isReleaseOf(event, MouseButton::Left);
    }

    [[nodiscard]] Pointer pointerFrom(
        const InputState &state,
        bool located,
        MouseButton button = MouseButton::Left) noexcept;

    [[nodiscard]] constexpr HoverPointer hoverFrom(
        const std::optional<PointerHint> &hint) noexcept
    {
        if (!hint)
        {
            return HoverPointer{};
        }

        return HoverPointer{.position = asPoint(hint->position)};
    }

}
