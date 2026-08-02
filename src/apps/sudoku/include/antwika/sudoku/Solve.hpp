#pragma once

#include <antwika/wfc/SolveResult.hpp>
#include <antwika/wfc/SolverLimits.hpp>

#include "antwika/sudoku/Board.hpp"

namespace antwika::sudoku
{

    /**
     * @brief How many candidate values a solve inside a tick may try.
     *
     * A solve runs on the thread that is stepping the simulation, so it
     * has to be bounded by something: an unbounded one on a grid
     * somebody had filled adversarially would stop the loop rather than
     * merely take a while, and a tick that never returns draws no
     * frames and reads no input.
     * Steps rather than milliseconds, because a wall-clock bound would
     * make the answer depend on the machine and a replay would diverge
     * from the run it replays -- which is the same reason
     * antwika::wfc counts steps in the first place.
     *
     * Generous enough that no ordinary Sudoku reaches it: the demo
     * puzzle propagates to a solution with no backtracking at all.
     */
    inline constexpr std::uint64_t kSolveStepBudget = 200000;

    /**
     * @brief What one attempt at finishing a grid came to.
     */
    struct SolveAttempt
    {
        /**
         * @brief How the solve ended.
         */
        antwika::wfc::SolveOutcome outcome =
            antwika::wfc::SolveOutcome::Unsatisfiable;

        /**
         * @brief The finished grid, meaningful only when Solved.
         *
         * Blank otherwise, rather than a partly filled guess: the whole
         * point of antwika::wfc reporting an outcome is that a caller
         * is never handed a plausible-looking board it cannot trust.
         */
        Board board{};
    };

    /**
     * @brief Finish a grid from wherever it has got to.
     *
     * The console showcase this application used to be, kept as a
     * function: the 81 cells become domains, the rules become 27
     * antwika::wfc::AllDifferentConstraints, and the solver reports
     * rather than guesses.
     * Filled squares -- the puzzle's clues and whatever a player has
     * typed alike -- go in as singleton domains, so "solve from here"
     * and "solve the puzzle" are the same call.
     *
     * Deterministic on its input and drawing no random bits at all:
     * antwika::wfc breaks every tie by ascending cell index, so the
     * same grid gives the same solution on every toolchain, and there
     * is no seed for a recording to have to carry.
     *
     * @param board The grid to finish.
     * @param limits How much search to allow; the default is
     * kSolveStepBudget.
     * @return The outcome, and the finished grid when there is one.
     */
    [[nodiscard]] SolveAttempt solvePuzzle(
        const Board &board,
        antwika::wfc::SolverLimits limits = {
            .maxSteps = kSolveStepBudget});

} // namespace antwika::sudoku
