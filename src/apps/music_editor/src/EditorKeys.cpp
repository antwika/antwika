#include "antwika/music_editor/EditorKeys.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace antwika::music_editor
{

    using antwika::input::Key;

    namespace
    {
        // What each digit key types with shift held.
        // The layout is this application's, not a window system's.
        // Five of the ten are mini-notation, which is why.
        constexpr std::string_view kShiftedDigits{")!@#$%^&*("};

        constexpr std::string_view kDigits{"0123456789"};

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
    } // namespace

    std::optional<antwika::ui::Key> uiKeyFor(
        const Key key, const bool shift) noexcept
    {
        // Shift changes no meaning here.
        // It is read where the characters are.
        (void)shift;

        switch (key)
        {
        case Key::Enter:
            return antwika::ui::Key::Activate;
        case Key::Backspace:
            return antwika::ui::Key::Backspace;
        case Key::ArrowLeft:
            return antwika::ui::Key::MoveLeft;
        case Key::ArrowRight:
            return antwika::ui::Key::MoveRight;
        case Key::ArrowUp:
            return antwika::ui::Key::MoveUp;
        case Key::ArrowDown:
            return antwika::ui::Key::MoveDown;
        default:
            break;
        }

        return std::nullopt;
    }

    std::string_view typedTextFor(
        const Key key, const bool shift) noexcept
    {
        const auto index = static_cast<std::uint8_t>(key);

        // The digits are asked about first, and that is not arbitrary.
        // Key::A is the enumeration's zero, so "at least A" is folded.
        // Letters first would leave "at least Digit0" always true here.
        if (key >= Key::Digit0 && key <= Key::Digit9)
        {
            const auto offset = static_cast<std::size_t>(
                index - static_cast<std::uint8_t>(Key::Digit0));

            return oneOf(shift ? kShiftedDigits : kDigits, offset);
        }

        if (key >= Key::A && key <= Key::Z)
        {
            const auto offset = static_cast<std::size_t>(
                index - static_cast<std::uint8_t>(Key::A));

            return oneOf(shift ? kUppercase : kLowercase, offset);
        }

        switch (key)
        {
        case Key::Space:
            return " ";
        case Key::Tab:
            // Two, because one space does not read as an indent.
            return "  ";
        case Key::Minus:
            return shift ? "_" : "-";
        case Key::Period:
            return shift ? ">" : ".";
        case Key::Comma:
            return shift ? "<" : ",";
        case Key::Slash:
            return shift ? "?" : "/";
        case Key::LeftBracket:
            return shift ? "{" : "[";
        case Key::RightBracket:
            return shift ? "}" : "]";
        case Key::Semicolon:
            return shift ? ":" : ";";
        case Key::Apostrophe:
            return shift ? "\"" : "'";
        case Key::Grave:
            return shift ? "~" : "`";
        case Key::Equal:
            return shift ? "+" : "=";
        default:
            break;
        }

        return {};
    }

} // namespace antwika::music_editor
