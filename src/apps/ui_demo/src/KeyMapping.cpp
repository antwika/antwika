#include "antwika/ui_demo/KeyMapping.hpp"

#include <cstdint>

namespace antwika::ui_demo
{

    using antwika::input::Key;

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
        case Key::Escape:
            return antwika::ui::Key::Cancel;
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
        // That is a branch no test could take.
        if (key >= Key::Digit0 && key <= Key::Digit9 && !shift)
        {
            return static_cast<char>(
                '0' + (index - static_cast<std::uint8_t>(Key::Digit0)));
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
            return shift ? '\0' : '.';
        default:
            break;
        }

        return '\0';
    }

} // namespace antwika::ui_demo
