#include "antwika/music_editor/EditorKeys.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace antwika::music_editor
{

    using antwika::input::Key;
    using antwika::input::KeyModifiers;

    namespace
    {
        struct Chord final
        {
            Key key;
            std::string_view plain;
            std::string_view shifted;
            std::string_view alted;
        };

        constexpr auto kEnglish = std::to_array<Chord>({
            {Key::Digit0, "0", ")", ""},
            {Key::Digit1, "1", "!", ""},
            {Key::Digit2, "2", "@", ""},
            {Key::Digit3, "3", "#", ""},
            {Key::Digit4, "4", "$", ""},
            {Key::Digit5, "5", "%", ""},
            {Key::Digit6, "6", "^", ""},
            {Key::Digit7, "7", "&", ""},
            {Key::Digit8, "8", "*", ""},
            {Key::Digit9, "9", "(", ""},
            {Key::Minus, "-", "_", ""},
            {Key::Equal, "=", "+", ""},
            {Key::LeftBracket, "[", "{", ""},
            {Key::RightBracket, "]", "}", ""},
            {Key::Backslash, "\\", "|", ""},
            {Key::Semicolon, ";", ":", ""},
            {Key::Apostrophe, "'", "\"", ""},
            {Key::Grave, "`", "~", ""},
            {Key::Comma, ",", "<", ""},
            {Key::Period, ".", ">", ""},
            {Key::Slash, "/", "?", ""},
            {Key::IntlBackslash, "\\", "|", ""},
        });

        constexpr auto kSwedish = std::to_array<Chord>({
            {Key::Digit0, "0", "=", "}"},
            {Key::Digit1, "1", "!", ""},
            {Key::Digit2, "2", "\"", "@"},
            {Key::Digit3, "3", "#", ""},
            {Key::Digit4, "4", "", "$"},
            {Key::Digit5, "5", "%", ""},
            {Key::Digit6, "6", "&", ""},
            {Key::Digit7, "7", "/", "{"},
            {Key::Digit8, "8", "(", "["},
            {Key::Digit9, "9", ")", "]"},
            {Key::Minus, "+", "?", "\\"},
            {Key::Equal, "", "", ""},
            {Key::LeftBracket, "", "", ""},
            {Key::RightBracket, "", "^", "~"},
            {Key::Backslash, "'", "*", ""},
            {Key::Semicolon, "", "", ""},
            {Key::Apostrophe, "", "", ""},
            {Key::Grave, "", "", ""},
            {Key::Comma, ",", ";", ""},
            {Key::Period, ".", ":", ""},
            {Key::Slash, "-", "_", ""},
            {Key::IntlBackslash, "<", ">", "|"},
        });

        constexpr std::string_view kLowercase{
            "abcdefghijklmnopqrstuvwxyz"};

        constexpr std::string_view kUppercase{
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"};

        constexpr std::string_view kDigits{"0123456789"};

        [[nodiscard]] std::string_view oneOf(
            const std::string_view table, const std::size_t at) noexcept
        {
            return table.substr(at, 1);
        }

        [[nodiscard]] std::span<const Chord> chordsOf(
            const KeyLayout layout) noexcept
        {
            return layout == KeyLayout::Swedish
                       ? std::span<const Chord>{kSwedish}
                       : std::span<const Chord>{kEnglish};
        }

        [[nodiscard]] std::string_view pick(
            const Chord &chord, const KeyModifiers modifiers) noexcept
        {
            if (modifiers.alt && !chord.alted.empty())
            {
                return chord.alted;
            }

            return modifiers.shift ? chord.shifted : chord.plain;
        }
    }

    std::optional<antwika::ui::Key> uiKeyFor(
        const Key key, const KeyModifiers modifiers) noexcept
    {
        if (modifiers.control)
        {
            switch (key)
            {
            case Key::A:
                return antwika::ui::Key::SelectAll;
            case Key::C:
                return antwika::ui::Key::Copy;
            case Key::X:
                return antwika::ui::Key::Cut;
            default:
                return std::nullopt;
            }
        }

        switch (key)
        {
        case Key::Enter:
            return antwika::ui::Key::Activate;
        case Key::Backspace:
            return antwika::ui::Key::Backspace;
        case Key::Delete:
            return antwika::ui::Key::Delete;
        case Key::ArrowLeft:
            return modifiers.shift ? antwika::ui::Key::SelectLeft
                                   : antwika::ui::Key::MoveLeft;
        case Key::ArrowRight:
            return modifiers.shift ? antwika::ui::Key::SelectRight
                                   : antwika::ui::Key::MoveRight;
        case Key::ArrowUp:
            return modifiers.shift ? antwika::ui::Key::SelectUp
                                   : antwika::ui::Key::MoveUp;
        case Key::ArrowDown:
            return modifiers.shift ? antwika::ui::Key::SelectDown
                                   : antwika::ui::Key::MoveDown;
        case Key::Home:
            return modifiers.shift
                       ? antwika::ui::Key::SelectLineStart
                       : antwika::ui::Key::MoveLineStart;
        case Key::End:
            return modifiers.shift ? antwika::ui::Key::SelectLineEnd
                                   : antwika::ui::Key::MoveLineEnd;
        default:
            break;
        }

        return std::nullopt;
    }

    std::string_view typedTextFor(
        const Key key,
        const KeyModifiers modifiers,
        const KeyLayout layout) noexcept
    {
        if (modifiers.control)
        {
            return {};
        }

        if (key >= Key::A && key <= Key::Z)
        {
            const auto offset = static_cast<std::size_t>(
                static_cast<std::uint8_t>(key)
                - static_cast<std::uint8_t>(Key::A));

            return oneOf(
                modifiers.shift ? kUppercase : kLowercase, offset);
        }

        if (key >= Key::Keypad0 && key <= Key::Keypad9)
        {
            const auto offset = static_cast<std::size_t>(
                static_cast<std::uint8_t>(key)
                - static_cast<std::uint8_t>(Key::Keypad0));

            return oneOf(kDigits, offset);
        }

        if (key == Key::Space)
        {
            return " ";
        }

        if (key == Key::Tab)
        {
            return "  ";
        }

        for (const auto &chord : chordsOf(layout))
        {
            if (chord.key == key)
            {
                return pick(chord, modifiers);
            }
        }

        return {};
    }

    std::string_view nameOf(const KeyLayout layout) noexcept
    {
        return layout == KeyLayout::Swedish ? "swedish" : "english";
    }

}
