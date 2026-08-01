#pragma once

#include <optional>

#include <antwika/input/Key.hpp>

namespace antwika::sudoku
{

    /**
     * @brief Read a key as the digit it writes into a square.
     *
     * The application's half of a seam both libraries leave open:
     * antwika::input names the key that went down and has no opinion
     * about what it means, and which key does what is an application's
     * decision.
     *
     * A layout written down rather than asked of a window system, so
     * what a recording holds is the symbolic key and the digit comes
     * back identically under any backend on any keyboard -- the same
     * reason InputEventCodec persists key names rather than scancodes.
     *
     * Backspace, Delete and `0` all empty a square, since all three are
     * things somebody reaches for meaning "take that back", and 0 is
     * already what Board calls a blank.
     *
     * @param key The key that went down.
     * @return The digit 1-9, 0 to empty the square, or nothing for a
     * key this application has no meaning for.
     */
    [[nodiscard]] std::optional<int> digitFor(
        antwika::input::Key key) noexcept;

} // namespace antwika::sudoku
