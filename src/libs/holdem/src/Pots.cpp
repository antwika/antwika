#include "Pots.hpp"

#include <algorithm>
#include <cstddef>
#include <span>
#include <utility>
#include <vector>

#include "antwika/holdem/Chips.hpp"
#include "antwika/holdem/HandValue.hpp"
#include "antwika/holdem/Payout.hpp"
#include "antwika/holdem/SeatId.hpp"
#include "antwika/holdem/SidePot.hpp"

namespace antwika::holdem
{

    namespace
    {

        [[nodiscard]] std::vector<bool> eligibilityBySeat(
            std::size_t seatCount, std::span<const SeatId> eligibleSeats)
        {
            std::vector<bool> eligible(seatCount, false);
            for (const auto seat : eligibleSeats)
            {
                eligible[indexOf(seat)] = true;
            }
            return eligible;
        }

        [[nodiscard]] std::vector<Chips> eligibleLevels(
            std::span<const Chips> committed,
            const std::vector<bool> &eligible)
        {
            std::vector<Chips> levels;
            for (std::size_t index = 0; index < committed.size(); ++index)
            {
                if (eligible[index] && committed[index] > 0)
                {
                    levels.push_back(committed[index]);
                }
            }
            std::sort(levels.begin(), levels.end());
            levels.erase(
                std::unique(levels.begin(), levels.end()), levels.end());
            return levels;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] bool isContender(
            const SidePot &pot, SeatId seat) noexcept
        {
            return std::find(
                       pot.contenders.begin(), pot.contenders.end(), seat)
                   != pot.contenders.end();
        }

        [[nodiscard]] HandValue bestAmongContenders(
            const SidePot &pot, std::span<const HandValue> valuesBySeat)
        {
            auto best = valuesBySeat[indexOf(pot.contenders.front())];
            for (const auto seat : pot.contenders)
            {
                if (valuesBySeat[indexOf(seat)] > best)
                {
                    best = valuesBySeat[indexOf(seat)];
                }
            }
            return best;
        }

    } // namespace

    std::vector<SidePot> buildSidePots(
        std::span<const Chips> committed,
        std::span<const SeatId> eligibleSeats)
    {
        const auto eligible =
            eligibilityBySeat(committed.size(), eligibleSeats);
        const auto levels = eligibleLevels(committed, eligible);
        if (levels.empty())
        {
            return {};
        }

        std::vector<SidePot> pots;
        Chips floorLevel = 0;
        for (const auto level : levels)
        {
            SidePot pot;
            for (std::size_t index = 0; index < committed.size(); ++index)
            {
                const auto capped = std::min(committed[index], level);
                if (capped > floorLevel)
                {
                    pot.amount += capped - floorLevel;
                }
                if (eligible[index] && committed[index] >= level)
                {
                    pot.contenders.push_back(makeSeatId(index));
                }
            }
            pots.push_back(std::move(pot));
            floorLevel = level;
        }

        // A folded player may have put in more than anyone could cover.
        // Nobody can be paid for covering that, so it has no layer.
        // It rides along with the top one instead.
        Chips surplus = 0;
        for (const auto amount : committed)
        {
            if (amount > floorLevel)
            {
                surplus += amount - floorLevel;
            }
        }
        pots.back().amount += surplus;

        return pots;
    }

    std::vector<Payout> distributePots(
        std::span<const SidePot> pots,
        std::span<const HandValue> valuesBySeat,
        std::span<const SeatId> payoutOrder)
    {
        std::vector<Chips> owed(valuesBySeat.size(), 0);
        for (const auto &pot : pots)
        {
            const auto best = bestAmongContenders(pot, valuesBySeat);

            // Walked in payoutOrder rather than seat order.
            // That puts the odd chip nearest the left of the button.
            std::vector<SeatId> winners;
            for (const auto seat : payoutOrder)
            {
                if (isContender(pot, seat)
                    && valuesBySeat[indexOf(seat)] == best)
                {
                    winners.push_back(seat);
                }
            }

            const auto share = pot.amount / winners.size();
            auto oddChips = pot.amount % winners.size();
            for (const auto seat : winners)
            {
                auto amount = share;
                if (oddChips > 0)
                {
                    ++amount;
                    --oddChips;
                }
                owed[indexOf(seat)] += amount;
            }
        }

        std::vector<Payout> payouts;
        for (std::size_t index = 0; index < owed.size(); ++index)
        {
            if (owed[index] > 0)
            {
                payouts.push_back(Payout{
                    .seat = makeSeatId(index),
                    .amount = owed[index],
                });
            }
        }
        return payouts;
    }

} // namespace antwika::holdem
