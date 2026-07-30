#include "antwika/holdem/BettingRound.hpp"

#include "antwika/holdem/IllegalActionError.hpp"

namespace antwika::holdem
{

    void BettingRound::open(Chips bigBlind) noexcept
    {
        currentBet = bigBlind;
        lastRaiseSize = bigBlind;
    }

    void BettingRound::reset(Chips bigBlind) noexcept
    {
        currentBet = 0;
        lastRaiseSize = bigBlind;
    }

    void BettingRound::close() noexcept
    {
        currentBet = 0;
    }

    Chips BettingRound::bet() const noexcept
    {
        return currentBet;
    }

    bool BettingRound::isLive() const noexcept
    {
        return currentBet != 0;
    }

    Chips BettingRound::minimumRaiseTo() const noexcept
    {
        return currentBet + lastRaiseSize;
    }

    Chips BettingRound::owedBy(Chips roundCommitted) const noexcept
    {
        return currentBet - roundCommitted;
    }

    bool BettingRound::isCovered(Chips roundCommitted) const noexcept
    {
        return roundCommitted >= currentBet;
    }

    bool BettingRound::raiseTo(Chips target, Chips allInTo)
    {
        if (target <= currentBet)
        {
            throw IllegalActionError(
                "Table: a raise has to beat the current bet");
        }

        const auto minimumTo = minimumRaiseTo();
        if (target < minimumTo && target != allInTo)
        {
            throw IllegalActionError(
                "Table: below the minimum raise while holding chips back");
        }

        const auto previousBet = currentBet;
        currentBet = target;

        if (target < minimumTo)
        {
            // An all-in short of a full raise reopens nothing.
            return false;
        }

        lastRaiseSize = target - previousBet;

        return true;
    }

} // namespace antwika::holdem
