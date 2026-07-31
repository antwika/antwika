#include "antwika/game/KeyText.hpp"

#include <cstdint>

namespace antwika::game
{

    using antwika::input::Key;

    std::optional<antwika::ui::Key> uiKeyFor(Key key, bool shift) noexcept
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

    char typedCharacterFor(Key key, bool shift) noexcept
    {
        const auto index = static_cast<std::uint8_t>(key);

        if (key >= Key::A && key <= Key::Z)
        {
            const auto offset =
                index - static_cast<std::uint8_t>(Key::A);
            return static_cast<char>((shift ? 'A' : 'a') + offset);
        }

        if (key >= Key::Digit0 && key <= Key::Digit9 && !shift)
        {
            return static_cast<char>(
                '0' + (index - static_cast<std::uint8_t>(Key::Digit0)));
        }

        switch (key)
        {
        case Key::Space:
            return ' ';
        case Key::Minus:
            // The shifted hyphen, since a file name often wants one.
            return shift ? '_' : '-';
        case Key::Period:
            return shift ? '\0' : '.';
        default:
            break;
        }

        return '\0';
    }

} // namespace antwika::game
