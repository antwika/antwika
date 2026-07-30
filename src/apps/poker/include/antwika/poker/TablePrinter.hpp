#pragma once

#include <ostream>

#include <antwika/holdem/StepOutcome.hpp>
#include <antwika/holdem/Table.hpp>

#include "antwika/poker/CashGame.hpp"

namespace antwika::poker
{

    using antwika::holdem::StepOutcome;
    using antwika::holdem::Table;

    /**
     * @brief Narrates a table's steps to a stream.
     *
     * Reads the table and the cash game rather than being told what to
     * say, so nothing in the game has to carry presentation concerns
     * around with it -- and a test can point it at a std::ostringstream
     * and read back exactly what a player would have seen.
     */
    class TablePrinter final
    {
    public:
        /**
         * @brief Construct the printer over its output and its subjects.
         * @param out Stream every line is written to.
         * @param game Resolves seats to player names.
         * @param table Read for the board, the pot and hand results.
         */
        TablePrinter(
            std::ostream &out, const CashGame &game, const Table &table);

        TablePrinter(const TablePrinter &) = delete;
        TablePrinter(TablePrinter &&) = delete;

        TablePrinter &operator=(const TablePrinter &) = delete;
        TablePrinter &operator=(TablePrinter &&) = delete;

        /**
         * @brief Describe one step of the table's loop.
         *
         * Prints the deal, each action as it happens, a line whenever a
         * new street comes out, and the payouts once the hand is over.
         * An idle table prints nothing, since nothing happened.
         * @param outcome What the step did.
         */
        void printStep(const StepOutcome &outcome);

    private:
        std::ostream &out;
        const CashGame &game;
        const Table &table;

        void printHandStart();
        void printAction(const StepOutcome &outcome);
        void printResult();
        [[nodiscard]] std::string nameOf(
            antwika::holdem::SeatId seat) const;
    };

} // namespace antwika::poker
