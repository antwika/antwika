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

    /**
     * @brief The desk between a player's bankroll and a seat at the
     * table.
     *
     * Chips are never created here, only moved: a buy-in withdraws from
     * the bankroll and puts exactly that much in front of a seat, and a
     * cash-out takes whatever is left in front of that seat and pays it
     * back. Because those are the only two doors, the total of every
     * bankroll plus every stack plus the pot is conserved over a whole
     * session, and a player can never risk money they do not have.
     */
    class CashGame final
    {
    public:
        /**
         * @brief Construct the desk over its table and ledger.
         * @param table The table players are seated at.
         * @param ledger Where buy-ins come from and cash-outs go.
         * @param minimumBuyIn Smallest stack a player may sit down with,
         * and also the smallest top-up.
         */
        CashGame(
            Table &table, BankrollLedger &ledger, Chips minimumBuyIn);

        CashGame(const CashGame &) = delete;
        CashGame(CashGame &&) = delete;

        CashGame &operator=(const CashGame &) = delete;
        CashGame &operator=(CashGame &&) = delete;

        /**
         * @brief Sit a player down, or top up the stack they already
         * have.
         * @param player The player's name.
         * @param amount Chips to move from bankroll to table.
         * @return The seat they now occupy.
         * @throws CashGameError If amount is under the table minimum, no
         * seat is free, or the player is already in a live hand.
         * @throws BankrollError If amount exceeds the player's bankroll.
         */
        SeatId buyIn(const std::string &player, Chips amount);

        /**
         * @brief Take a player's remaining chips back to their bankroll
         * and free their seat.
         * @param player The player's name.
         * @throws CashGameError If they are not seated, or are in a live
         * hand.
         */
        void cashOut(const std::string &player);

        /**
         * @brief Send home everyone whose stack has run out.
         *
         * Nothing is paid back -- an empty stack is worth nothing -- but
         * the seat is freed, so the player can buy in again from
         * whatever bankroll they have left.
         * @return The names sent home, in seat order.
         */
        std::vector<std::string> cashOutBustedPlayers();

        /**
         * @brief Send home every seated player.
         *
         * What a session ends with, so bankrolls reflect how the poker
         * actually went rather than leaving money on the table. A player
         * still in a live hand is left alone, since those chips are not
         * theirs to take back yet.
         * @return The names sent home, in seat order.
         */
        std::vector<std::string> cashOutEveryone();

        /**
         * @brief Find who is sitting in a seat.
         * @param seat The seat to look at.
         * @return That player's name, or nothing if the seat is empty.
         */
        [[nodiscard]] std::optional<std::string> playerAt(
            SeatId seat) const;

        /**
         * @brief Find where a player is sitting.
         * @param player The player's name.
         * @return Their seat, or nothing if they are not at the table.
         */
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

} // namespace antwika::poker
