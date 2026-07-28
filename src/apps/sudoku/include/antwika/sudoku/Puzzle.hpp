#pragma once

#include <vector>

#include <antwika/wfc/AllDifferentConstraint.hpp>
#include <antwika/wfc/Domain.hpp>

#include "antwika/sudoku/Board.hpp"

namespace antwika::sudoku
{

    /**
     * @brief Build the initial antwika::wfc wave for a board.
     * @param board The board to convert; givens become singleton
     * domains, blanks become full domains.
     * @return 81 domains over alphabet indices 0-8, representing
     * digits 1-9, in row-major order matching Board's own layout.
     */
    [[nodiscard]] std::vector<antwika::wfc::Domain> buildInitialWave(
        const Board &board);

    /**
     * @brief Build Sudoku's row/column/box constraints.
     * @return 9 row + 9 column + 9 box AllDifferentConstraints, each
     * over the 9 flat cell indices it covers.
     */
    [[nodiscard]] std::vector<antwika::wfc::AllDifferentConstraint>
    buildConstraints();

} // namespace antwika::sudoku
