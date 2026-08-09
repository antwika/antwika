#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include <antwika/enums/Enumeration.hpp>

namespace antwika::input
{

    enum class Key : std::uint8_t
    {
        A = 0,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,

        Digit0,
        Digit1,
        Digit2,
        Digit3,
        Digit4,
        Digit5,
        Digit6,
        Digit7,
        Digit8,
        Digit9,

        Keypad0,
        Keypad1,
        Keypad2,
        Keypad3,
        Keypad4,
        Keypad5,
        Keypad6,
        Keypad7,
        Keypad8,
        Keypad9,

        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,

        ArrowLeft,
        ArrowRight,
        ArrowUp,
        ArrowDown,

        Escape,
        Enter,
        Space,
        Tab,
        Backspace,
        Delete,
        Insert,
        Home,
        End,
        PageUp,
        PageDown,

        Minus,
        Equal,
        LeftBracket,
        RightBracket,
        Backslash,
        Semicolon,
        Apostrophe,
        Grave,
        Comma,
        Period,
        Slash,

        IntlBackslash,

        CapsLock,
        LeftShift,
        RightShift,
        LeftControl,
        RightControl,
        LeftAlt,
        RightAlt,
        LeftSuper,
        RightSuper,
    };

    [[nodiscard]] constexpr Key enumBound(Key) noexcept
    {
        return Key::RightSuper;
    }

    inline constexpr std::size_t kKeyCount =
        antwika::enums::kCount<Key>;

    [[nodiscard]] constexpr std::size_t keyIndex(Key key) noexcept
    {
        return static_cast<std::size_t>(key);
    }

    [[nodiscard]] std::string_view toString(Key key);

    [[nodiscard]] Key keyFromString(std::string_view name);

}
