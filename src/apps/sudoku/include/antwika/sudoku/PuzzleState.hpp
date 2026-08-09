#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include <antwika/wfc/SolverLimits.hpp>

#include "antwika/sudoku/Board.hpp"
#include "antwika/sudoku/Solve.hpp"
#include "antwika/sudoku/Status.hpp"

namespace antwika::sudoku
{

    class PuzzleState final
    {
    public:
        void start(const Board &puzzle);

        [[nodiscard]] const Board &board() const noexcept;

        [[nodiscard]] const Board &clues() const noexcept;

        void restore(
            const Board &puzzle,
            const Board &progress,
            const std::optional<Square> &pick,
            Status said);

        [[nodiscard]] bool isGiven(Square square) const;

        [[nodiscard]] const std::optional<Square> &selected()
            const noexcept;

        void select(Square square);

        void enter(int digit);

        void write(Square square, int digit);

        void solve(
            antwika::wfc::SolverLimits limits = {
                .maxSteps = kSolveStepBudget});

        [[nodiscard]] Status status() const noexcept;

        [[nodiscard]] std::uint32_t filled() const;

    private:
        void noteCompletion();

        Board givens{};
        Board cells{};
        std::optional<Square> chosen{};
        Status note = Status::Playing;
    };

}
