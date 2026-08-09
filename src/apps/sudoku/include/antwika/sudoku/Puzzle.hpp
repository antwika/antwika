#pragma once

#include <vector>

#include <antwika/wfc/AllDifferentConstraint.hpp>
#include <antwika/wfc/Domain.hpp>

#include "antwika/sudoku/Board.hpp"

namespace antwika::sudoku
{

    [[nodiscard]] std::vector<antwika::wfc::Domain> buildInitialWave(
        const Board &board);

    [[nodiscard]] std::vector<antwika::wfc::AllDifferentConstraint>
    buildConstraints();

    [[nodiscard]] bool obeysRules(const Board &board);

    [[nodiscard]] bool isComplete(const Board &board);

}
