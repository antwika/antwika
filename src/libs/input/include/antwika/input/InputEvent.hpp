#pragma once

#include <cstdint>
#include <variant>

#include "antwika/input/Key.hpp"
#include "antwika/input/KeyModifiers.hpp"
#include "antwika/input/MouseButton.hpp"
#include "antwika/input/Position.hpp"

namespace antwika::input
{

    struct KeyPressed final
    {
        Key key = Key::A;
        KeyModifiers modifiers{};

        bool repeat = false;

        [[nodiscard]] bool operator==(const KeyPressed &other) const = default;
    };

    struct KeyReleased final
    {
        Key key = Key::A;
        KeyModifiers modifiers{};

        [[nodiscard]] bool operator==(const KeyReleased &other) const = default;
    };

    struct PointerMoved final
    {
        Position position{};

        [[nodiscard]] bool operator==(
            const PointerMoved &other) const = default;
    };

    struct PointerButtonPressed final
    {
        MouseButton button = MouseButton::Left;
        Position position{};
        KeyModifiers modifiers{};

        [[nodiscard]] bool operator==(
            const PointerButtonPressed &other) const = default;
    };

    struct PointerButtonReleased final
    {
        MouseButton button = MouseButton::Left;
        Position position{};
        KeyModifiers modifiers{};

        [[nodiscard]] bool operator==(
            const PointerButtonReleased &other) const = default;
    };

    struct PointerScrolled final
    {
        std::int32_t horizontal = 0;
        std::int32_t vertical = 0;

        [[nodiscard]] bool operator==(
            const PointerScrolled &other) const = default;
    };

    using InputEvent = std::variant<
        KeyPressed,
        KeyReleased,
        PointerMoved,
        PointerButtonPressed,
        PointerButtonReleased,
        PointerScrolled>;

}
