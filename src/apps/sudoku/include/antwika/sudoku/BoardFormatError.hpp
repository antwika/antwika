#pragma once

#include <stdexcept>

namespace antwika::sudoku
{

    /**
     * @brief Thrown when a Sudoku board is asked for something it
     * cannot be.
     *
     * A board string of the wrong length once whitespace is stripped,
     * or holding a character that isn't a digit 1-9 or a blank marker
     * (. or 0).
     * A row or column outside the grid.
     * A digit outside 0-9 written into a cell.
     *
     * Deliberately a single, specific, catchable type, mirroring
     * antwika::wfc::WfcError and antwika::ecs::EcsError.
     */
    class BoardFormatError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::sudoku
