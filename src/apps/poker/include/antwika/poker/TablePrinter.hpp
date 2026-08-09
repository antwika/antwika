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

    class TablePrinter final
    {
    public:
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

        void printStep(const StepOutcome &outcome);

        [[nodiscard]] PrinterMemory remember() const;

        void restore(const PrinterMemory &memory);

    private:
        struct Returned final
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

}
