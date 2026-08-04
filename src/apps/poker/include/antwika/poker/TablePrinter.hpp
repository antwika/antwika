#pragma once

#include <cstddef>
#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/holdem/Card.hpp>
#include <antwika/holdem/Chips.hpp>
#include <antwika/holdem/HandResult.hpp>
#include <antwika/holdem/SeatId.hpp>
#include <antwika/holdem/Stage.hpp>
#include <antwika/holdem/StepOutcome.hpp>
#include <antwika/holdem/Table.hpp>
#include <antwika/time/IClock.hpp>

#include "antwika/poker/CashGame.hpp"
#include "antwika/poker/PrinterMemory.hpp"

namespace antwika::poker
{

    using antwika::holdem::Card;
    using antwika::holdem::Chips;
    using antwika::holdem::HandResult;
    using antwika::holdem::SeatId;
    using antwika::holdem::Stage;
    using antwika::holdem::StepOutcome;
    using antwika::holdem::Table;
    using antwika::time::IClock;

    /**
     * @brief Writes a table's steps out as a hand history.
     *
     * Follows the layout hand-history readers and trackers already
     * understand: a header naming the hand and the stakes, the seats and
     * their stacks, one line per posted blind and per decision, a
     * `*** STREET ***` marker as each card comes out, and a summary
     * closing the hand off.
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
         * @param clock Read for the timestamp each hand is headed with.
         * @param tableName The name the table is announced under.
         */
        TablePrinter(
            std::ostream &out,
            const CashGame &game,
            const Table &table,
            IClock &clock,
            std::string tableName);

        TablePrinter(const TablePrinter &) = delete;
        TablePrinter(TablePrinter &&) = delete;

        TablePrinter &operator=(const TablePrinter &) = delete;
        TablePrinter &operator=(TablePrinter &&) = delete;

        /**
         * @brief Describe one step of the table's loop.
         *
         * Prints the deal, each action as it happens, a line whenever a
         * new street comes out, and the showdown, the payouts and the
         * summary once the hand is over. An idle table prints nothing,
         * since nothing happened.
         * @param outcome What the step did.
         */
        void printStep(const StepOutcome &outcome);

        /**
         * @brief Take the narration's mid-hand standing, as a value.
         * @return Everything restore() needs.
         */
        [[nodiscard]] PrinterMemory remember() const;

        /**
         * @brief Stand the narration at a remembered instant.
         * @param memory The instant to stand at.
         */
        void restore(const PrinterMemory &memory);

    private:
        /**
         * @brief A bet nobody covered, on its way back to its owner.
         */
        struct Returned
        {
            SeatId seat{};
            Chips amount{};
        };

        std::ostream &out;
        const CashGame &game;
        const Table &table;
        IClock &clock;
        std::string tableName;
        std::vector<PrinterNote> notes;
        std::optional<SeatId> smallBlindSeat;
        std::optional<SeatId> bigBlindSeat;
        Stage stage{};
        std::size_t boardShown = 0;

        void printHandStart();
        void printBlinds();
        void printPost(std::string_view blind, SeatId seat);
        void printAction(const StepOutcome &outcome);
        void printStreets(std::span<const Card> board);
        void printResult();
        void printSummary(const HandResult &result, Returned returned);

        [[nodiscard]] Returned uncalledBet() const;
        [[nodiscard]] Chips collectedBy(
            SeatId seat, Returned returned) const;
        [[nodiscard]] std::string outcomeOf(
            SeatId seat, Returned returned) const;
        [[nodiscard]] std::string timestamp() const;
        [[nodiscard]] std::string nameOf(SeatId seat) const;
        [[nodiscard]] std::string seatLabel(SeatId seat) const;
        [[nodiscard]] std::string positionsOf(SeatId seat) const;
        [[nodiscard]] std::optional<SeatId> nextInHand(SeatId from) const;
        [[nodiscard]] bool handJustEnded() const;
        [[nodiscard]] bool wasDealtIn(SeatId seat) const;
        [[nodiscard]] Chips stackBeforeTheHand(SeatId seat) const;
    };

} // namespace antwika::poker
