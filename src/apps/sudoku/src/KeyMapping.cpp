#include "antwika/sudoku/KeyMapping.hpp"

#include <cstdint>

namespace antwika::sudoku
{

    using antwika::input::Key;

    std::optional<int> digitFor(const Key key) noexcept
    {
        // The digits are asked about first, and that is not arbitrary.
        // Key::A is the enumeration's zero, so "at least A" is folded.
        // Letters first would leave "at least Digit0" always true here.
        // That is a branch no test could take.
        // ui_demo::typedCharacterFor() says the same at length.
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

} // namespace antwika::sudoku
