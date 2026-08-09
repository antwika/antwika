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

}
