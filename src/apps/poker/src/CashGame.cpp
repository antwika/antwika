#include "antwika/poker/CashGame.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <antwika/holdem/Seat.hpp>
#include <antwika/holdem/SeatId.hpp>

#include "antwika/poker/CashGameError.hpp"

namespace antwika::poker
{

    using antwika::holdem::indexOf;
    using antwika::holdem::makeSeatId;

    namespace
    {

        // The same rule Table::removePlayer() enforces.
        // A folded player's stake is in the pot until it is paid out.
        // So the seat is not theirs to vacate yet.
        [[nodiscard]] bool hasChipsInThePot(
            const Table &table, SeatId seat)
        {
            const auto &state = table.seatAt(seat);
            return table.isHandInProgress()
                   && (state.inHand || state.committed > 0);
        }

    } // namespace

    CashGame::CashGame(
        Table &table, BankrollLedger &ledger, Chips minimumBuyIn)
        : table(table),
          ledger(ledger),
          minimumBuyIn(minimumBuyIn),
          namesBySeat(table.seatCount())
    {
    }

    SeatId CashGame::buyIn(const std::string &player, Chips amount)
    {
        if (amount < minimumBuyIn)
        {
            throw CashGameError(
                "CashGame: " + player + " must buy in for at least "
                + std::to_string(minimumBuyIn));
        }

        // Everything that could refuse is checked before the bankroll.
        // A refusal then never strands chips between ledger and table.
        const auto seated = seatOf(player);
        if (seated && table.seatAt(*seated).inHand)
        {
            throw CashGameError(
                "CashGame: " + player
                + " cannot top up in the middle of a hand");
        }

        const auto free = table.firstFreeSeat();
        if (!seated && !free)
        {
            throw CashGameError("CashGame: every seat is taken");
        }

        ledger.withdraw(player, amount);
        if (seated)
        {
            table.addChips(*seated, amount);
            return *seated;
        }

        table.seatPlayer(*free, amount);
        namesBySeat[indexOf(*free)] = player;
        return *free;
    }

    void CashGame::cashOut(const std::string &player)
    {
        const auto seated = seatOf(player);
        if (!seated)
        {
            throw CashGameError(
                "CashGame: " + player + " is not at the table");
        }
        if (hasChipsInThePot(table, *seated))
        {
            throw CashGameError(
                "CashGame: " + player
                + " cannot leave in the middle of a hand");
        }

        const auto stack = table.seatAt(*seated).stack;
        table.removePlayer(*seated);
        namesBySeat[indexOf(*seated)].clear();
        ledger.deposit(player, stack);
    }

    std::vector<std::string> CashGame::cashOutBustedPlayers()
    {
        return cashOutSeats(true);
    }

    std::vector<std::string> CashGame::cashOutEveryone()
    {
        return cashOutSeats(false);
    }

    std::optional<std::string> CashGame::playerAt(SeatId seat) const
    {
        const auto &name = namesBySeat[indexOf(seat)];
        if (name.empty())
        {
            return std::nullopt;
        }
        return name;
    }

    std::optional<SeatId> CashGame::seatOf(const std::string &player) const
    {
        for (std::size_t index = 0; index < namesBySeat.size(); ++index)
        {
            if (namesBySeat[index] == player)
            {
                return makeSeatId(index);
            }
        }
        return std::nullopt;
    }

    std::vector<std::string> CashGame::cashOutSeats(bool onlyBusted)
    {
        std::vector<std::string> sentHome;
        for (std::size_t index = 0; index < namesBySeat.size(); ++index)
        {
            const auto seat = makeSeatId(index);
            const auto player = playerAt(seat);
            if (!player)
            {
                continue;
            }
            if (onlyBusted && table.seatAt(seat).stack > 0)
            {
                continue;
            }

            // A hand still running owns those chips.
            // They stay put rather than being pulled out from under it.
            if (hasChipsInThePot(table, seat))
            {
                continue;
            }

            cashOut(*player);
            sentHome.push_back(*player);
        }
        return sentHome;
    } // GCOVR_EXCL_LINE

} // namespace antwika::poker
