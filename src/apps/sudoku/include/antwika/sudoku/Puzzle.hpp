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

    /**
     * @brief Check a board against the rules, blanks and all.
     *
     * Asked of a board somebody is part-way through, so a blank is not
     * a violation: what it refuses is two of the same digit inside one
     * row, column or box.
     *
     * The groups come from buildConstraints() rather than from a second
     * copy of the geometry, so the squares this checks are by
     * construction the squares the solver constrains.
     *
     * @param board The board to check.
     * @return True when no group holds one digit twice.
     */
    [[nodiscard]] bool obeysRules(const Board &board);

    /**
     * @brief Check whether a board is a finished Sudoku.
     * @param board The board to check.
     * @return True when every square holds a digit and obeysRules().
     */
    [[nodiscard]] bool isComplete(const Board &board);

} // namespace antwika::sudoku
