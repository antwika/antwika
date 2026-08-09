#pragma once

#include <optional>
#include <string>
#include <vector>

#include <antwika/holdem/Chips.hpp>
#include <antwika/holdem/SeatId.hpp>
#include <antwika/holdem/Table.hpp>

#include "antwika/poker/BankrollLedger.hpp"

namespace antwika::poker
{

    using antwika::holdem::Chips;
    using antwika::holdem::SeatId;
    using antwika::holdem::Table;

    class CashGame final
    {
    public:
        CashGame(
            Table &table, BankrollLedger &ledger, Chips minimumBuyIn);

        CashGame(const CashGame &) = delete;
        CashGame(CashGame &&) = delete;

        CashGame &operator=(const CashGame &) = delete;
        CashGame &operator=(CashGame &&) = delete;

        SeatId buyIn(const std::string &player, Chips amount);

        void cashOut(const std::string &player);

        std::vector<std::string> cashOutBustedPlayers();

        std::vector<std::string> cashOutEveryone();

        [[nodiscard]] std::optional<std::string> playerAt(
            SeatId seat) const;

        [[nodiscard]] const std::vector<std::string> &
        names() const noexcept;

        void restoreNames(std::vector<std::string> held);

        [[nodiscard]] std::optional<SeatId> seatOf(
            const std::string &player) const;

    private:
        Table &table;
        BankrollLedger &ledger;
        Chips minimumBuyIn;
        std::vector<std::string> namesBySeat;

        [[nodiscard]] std::vector<std::string> cashOutSeats(
            bool onlyBusted);
    };

}
