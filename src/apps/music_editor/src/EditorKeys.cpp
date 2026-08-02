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
        /**
         * @brief What one key types, in one layout.
         *
         * Three columns because the Swedish board really does keep the
         * brackets, the braces and the dollar on its right-hand alt
         * key, and a table with two could not say so.
         */
        struct Chord
        {
            Key key;
            std::string_view plain;
            std::string_view shifted;
            std::string_view alted;
        };

        // The American board.
        // What the score language was written on, and what this was.
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

        // The Swedish board, key by key, as its keys are printed.
        // The empty cells are the ones this window cannot draw.
        // They are its own letters and its dead accents.
        // No score is written in those.
        // Everything the grammar needs is here.
        // Four of them are on alt, the dollar and the brackets among them.
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

        // One character out of a table that outlives every caller.
        // So a view of it is safe to hand back.
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

        // Alt first, and only where the layout puts something there.
        // Alt on a key with a bare third level types what it types.
        // Which is what a board with no third level does anyway.
        [[nodiscard]] std::string_view pick(
            const Chord &chord, const KeyModifiers modifiers) noexcept
        {
            if (modifiers.alt && !chord.alted.empty())
            {
                return chord.alted;
            }

            return modifiers.shift ? chord.shifted : chord.plain;
        }
    } // namespace

    std::optional<antwika::ui::Key> uiKeyFor(
        const Key key, const KeyModifiers modifiers) noexcept
    {
        // Control is a keyboard of its own, so it is read first.
        // Its V is deliberately absent.
        // A paste is characters, out of a clipboard the sink holds.
        if (modifiers.control)
        {
            switch (key)
            {
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
        // A copy that also typed a c would be no use to anybody.
        if (modifiers.control)
        {
            return {};
        }

        // The letters are the one run both boards agree about.
        if (key >= Key::A && key <= Key::Z)
        {
            const auto offset = static_cast<std::size_t>(
                static_cast<std::uint8_t>(key)
                - static_cast<std::uint8_t>(Key::A));

            return oneOf(
                modifiers.shift ? kUppercase : kLowercase, offset);
        }

        if (key == Key::Space)
        {
            return " ";
        }

        if (key == Key::Tab)
        {
            // Two, because one space does not read as an indent.
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

} // namespace antwika::music_editor
