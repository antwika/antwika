#include "antwika/sudoku/KeyMapping.hpp"

#include <cstdint>

namespace antwika::sudoku
{

    using antwika::input::Key;

    std::optional<int> digitFor(const Key key) noexcept
    {
        if (key >= Key::Digit0 && key <= Key::Digit9)
        {
            return static_cast<int>(
                static_cast<std::uint8_t>(key)
                - static_cast<std::uint8_t>(Key::Digit0));
        }

        if (key == Key::Backspace || key == Key::Delete)
        {
            return 0;
        }

        return std::nullopt;
    }

}
