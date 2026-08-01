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

    /**
     * @brief The whole of what one session holds: a puzzle, what has
     * been written into it, which square is picked, and what the
     * session last said.
     *
     * Every one of those is regenerated on a replay from the recorded
     * puzzle, clicks and keystrokes, so none of them is ever persisted.
     * The clues are kept beside the working grid rather than inferred
     * from it, because "this square was given" stops being visible the
     * moment somebody types the same digit into an empty one -- and a
     * clue that could be overwritten is a puzzle that can be edited
     * into an easier one.
     */
    class PuzzleState final
    {
    public:
        /**
         * @brief Start on a puzzle, discarding whatever came before.
         * @param puzzle The grid as given; its filled squares become
         * the clues.
         */
        void start(const Board &puzzle);

        /**
         * @brief Get the grid as it stands.
         * @return The clues and everything written since.
         */
        [[nodiscard]] const Board &board() const noexcept;

        /**
         * @brief Check whether a square is one of the puzzle's clues.
         * @param square The square to ask about.
         * @return True when the puzzle arrived with a digit there.
         * @throws BoardFormatError If the square is outside the grid.
         */
        [[nodiscard]] bool isGiven(Square square) const;

        /**
         * @brief Get which square is picked.
         * @return The square, or nothing until one is.
         */
        [[nodiscard]] const std::optional<Square> &selected()
            const noexcept;

        /**
         * @brief Pick a square.
         *
         * Picking a clue is allowed and says so: refusing the click
         * outright would leave somebody clicking a square and getting
         * no answer at all.
         *
         * @param square The square to pick.
         * @throws BoardFormatError If the square is outside the grid.
         */
        void select(Square square);

        /**
         * @brief Write a digit into the picked square.
         *
         * Nothing happens while no square is picked, since there is
         * nowhere for the digit to go.
         *
         * @param digit 0 to empty the square, or a digit 1-9.
         * @throws BoardFormatError If the digit is outside 0-9.
         */
        void enter(int digit);

        /**
         * @brief Write a digit into a named square.
         *
         * What a script asks for, and what typing goes through once a
         * square is picked: one rule about clues, one place the grid
         * changes, and one place completion is noticed.
         *
         * @param square The square to write into.
         * @param digit 0 to empty the square, or a digit 1-9.
         * @throws BoardFormatError If the square is outside the grid or
         * the digit is outside 0-9.
         */
        void write(Square square, int digit);

        /**
         * @brief Finish the grid from wherever it has got to.
         *
         * The answer is always reported rather than acted on blindly:
         * a grid somebody has made contradictory comes back
         * Unsolvable and is left exactly as it was, which is what
         * "a solve that cannot succeed says so" means here.
         *
         * @param limits How much search to allow.
         */
        void solve(
            antwika::wfc::SolverLimits limits = {
                .maxSteps = kSolveStepBudget});

        /**
         * @brief Get what the session last said.
         * @return The status; Playing until something happens.
         */
        [[nodiscard]] Status status() const noexcept;

        /**
         * @brief Count the squares holding a digit.
         * @return How many of the 81 are filled, clues included.
         */
        [[nodiscard]] std::uint32_t filled() const;

    private:
        void noteCompletion();

        Board givens{};
        Board cells{};
        std::optional<Square> chosen{};
        Status note = Status::Playing;
    };

} // namespace antwika::sudoku
