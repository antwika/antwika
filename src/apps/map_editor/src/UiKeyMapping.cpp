#include "antwika/map_editor/UiKeyMapping.hpp"

#include <cstdint>

namespace antwika::map_editor
{

    using antwika::input::Key;

    std::optional<ui::Key> uiKeyFor(
        const Key key, const bool shift) noexcept
    {
        switch (key)
        {
            case Key::Tab:
                return shift ? ui::Key::FocusPrevious
                             : ui::Key::FocusNext;
            case Key::Enter:
                return ui::Key::Activate;
            case Key::Backspace:
                return ui::Key::Backspace;
            case Key::Delete:
                return ui::Key::Delete;
            case Key::Escape:
                return ui::Key::Cancel;
            case Key::ArrowLeft:
                return shift ? ui::Key::SelectLeft : ui::Key::MoveLeft;
            case Key::ArrowRight:
                return shift ? ui::Key::SelectRight
                             : ui::Key::MoveRight;
            case Key::Home:
                return shift ? ui::Key::SelectLineStart
                             : ui::Key::MoveLineStart;
            case Key::End:
                return shift ? ui::Key::SelectLineEnd
                             : ui::Key::MoveLineEnd;
            default:
                return std::nullopt;
        }
    }

    char typedCharacterFor(const Key key, const bool shift) noexcept
    {
        const auto index = static_cast<std::uint8_t>(key);

        if (key >= Key::Digit0 && key <= Key::Digit9 && !shift)
        {
            return static_cast<char>(
                '0'
                + (index - static_cast<std::uint8_t>(Key::Digit0)));
        }

        if (key >= Key::A && key <= Key::Z)
        {
            const auto offset =
                index - static_cast<std::uint8_t>(Key::A);

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
            case Key::Comma:
                return shift ? '\0' : ',';
            default:
                return '\0';
        }
    }

}
