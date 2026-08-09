#include "antwika/console/Typing.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace antwika::console
{

    using antwika::input::Key;

    namespace
    {
        constexpr std::array<char, 10> kSwedishShiftedDigits{
            '=', '!', '"', '#', '\0', '%', '&', '/', '(', ')'};

        constexpr std::array<char, 10> kSwedishAltedDigits{
            '}', '\0', '@', '\0', '$', '\0', '\0', '{', '[', ']'};

        [[nodiscard]] char englishPunctuation(
            Key key, bool shift) noexcept
        {
            switch (key)
            {
            case Key::Minus:
                return shift ? '_' : '-';
            case Key::Period:
                return shift ? '\0' : '.';
            case Key::Comma:
                return shift ? '\0' : ',';
            case Key::Semicolon:
                return shift ? ':' : ';';
            case Key::Apostrophe:
                return shift ? '"' : '\'';
            case Key::LeftBracket:
                return shift ? '{' : '[';
            case Key::RightBracket:
                return shift ? '}' : ']';
            default:
                break;
            }

            return '\0';
        }

        [[nodiscard]] char swedishPunctuation(
            Key key, bool shift) noexcept
        {
            switch (key)
            {
            case Key::Minus:
                return shift ? '?' : '+';
            case Key::Slash:
                return shift ? '_' : '-';
            case Key::Period:
                return shift ? ':' : '.';
            case Key::Comma:
                return shift ? ';' : ',';
            case Key::Backslash:
                return shift ? '*' : '\'';
            default:
                break;
            }

            return '\0';
        }
    }

    char typedCharacterFor(
        Key key, bool shift, KeyboardLayout layout, bool alt) noexcept
    {
        const auto index = static_cast<std::uint8_t>(key);

        if (key >= Key::Keypad0 && key <= Key::Keypad9)
        {
            return static_cast<char>(
                '0' + index - static_cast<std::uint8_t>(Key::Keypad0));
        }

        if (key >= Key::Digit0 && key <= Key::Digit9)
        {
            const auto digit = static_cast<std::size_t>(
                index - static_cast<std::uint8_t>(Key::Digit0));

            if (layout == KeyboardLayout::Swedish && alt)
            {
                return kSwedishAltedDigits[digit];
            }

            if (!shift)
            {
                return static_cast<char>('0' + digit);
            }

            return layout == KeyboardLayout::Swedish
                       ? kSwedishShiftedDigits[digit]
                       : '\0';
        }

        if (key >= Key::A && key <= Key::Z)
        {
            const auto offset =
                index - static_cast<std::uint8_t>(Key::A);
            return static_cast<char>((shift ? 'A' : 'a') + offset);
        }

        if (key == Key::Space)
        {
            return ' ';
        }

        return layout == KeyboardLayout::Swedish
                   ? swedishPunctuation(key, shift)
                   : englishPunctuation(key, shift);
    }

    std::optional<antwika::ui::Key> consoleKeyFor(Key key) noexcept
    {
        switch (key)
        {
        case Key::Backspace:
            return antwika::ui::Key::Backspace;
        case Key::ArrowLeft:
            return antwika::ui::Key::MoveLeft;
        case Key::ArrowRight:
            return antwika::ui::Key::MoveRight;
        default:
            break;
        }

        return std::nullopt;
    }

}
