#include "antwika/music_editor/EditorKeys.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace antwika::music_editor
{

    using antwika::input::Key;

    namespace
    {
        // What each digit key types with shift held.
        // The layout is this application's, not a window system's.
        // Five of the ten are mini-notation, which is why.
        constexpr std::array<char, 10> kShiftedDigits{
            ')', '!', '@', '#', '$', '%', '^', '&', '*', '('};
    } // namespace

    std::optional<antwika::ui::Key> uiKeyFor(
        const Key key, const bool shift) noexcept
    {
        switch (key)
        {
        case Key::Tab:
            return shift ? antwika::ui::Key::FocusPrevious
                         : antwika::ui::Key::FocusNext;
        case Key::Enter:
            return antwika::ui::Key::Activate;
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

    char typedCharacterFor(const Key key, const bool shift) noexcept
    {
        const auto index = static_cast<std::uint8_t>(key);

        // The digits are asked about first, and that is not arbitrary.
        // Key::A is the enumeration's zero, so "at least A" is folded.
        // Letters first would leave "at least Digit0" always true here.
        if (key >= Key::Digit0 && key <= Key::Digit9)
        {
            const auto offset =
                index - static_cast<std::uint8_t>(Key::Digit0);

            if (shift)
            {
                return kShiftedDigits[static_cast<std::size_t>(offset)];
            }

            return static_cast<char>('0' + offset);
        }

        if (key >= Key::A && key <= Key::Z)
        {
            const auto offset = index - static_cast<std::uint8_t>(Key::A);

            return static_cast<char>((shift ? 'A' : 'a') + offset);
        }

        switch (key)
        {
        case Key::Space:
            return ' ';
        case Key::Minus:
            return shift ? '_' : '-';
        case Key::Period:
            return shift ? '>' : '.';
        case Key::Comma:
            return shift ? '<' : ',';
        case Key::Slash:
            return shift ? '?' : '/';
        case Key::LeftBracket:
            return shift ? '{' : '[';
        case Key::RightBracket:
            return shift ? '}' : ']';
        case Key::Grave:
            return shift ? '~' : '`';
        case Key::Equal:
            return shift ? '+' : '=';
        default:
            break;
        }

        return '\0';
    }

} // namespace antwika::music_editor
