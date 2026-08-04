#include "antwika/holdem/Table.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "antwika/holdem/Card.hpp"
#include "antwika/holdem/Chips.hpp"
#include "antwika/holdem/HandResult.hpp"
#include "antwika/holdem/HandValue.hpp"
#include "antwika/holdem/IllegalActionError.hpp"
#include "antwika/holdem/Limits.hpp"
#include "antwika/holdem/Seat.hpp"
#include "antwika/holdem/SeatId.hpp"
#include "antwika/holdem/ShowdownEntry.hpp"
#include "antwika/holdem/TableStateError.hpp"

#include "Pots.hpp"
#include "Showdown.hpp"

namespace antwika::holdem
{

    namespace
    {

        [[nodiscard]] bool isDealtIn(const Seat &seat) noexcept
        {
            return seat.occupied && seat.stack > 0;
        }

        [[nodiscard]] bool isInHand(const Seat &seat) noexcept
        {
            return seat.inHand;
        }

        [[nodiscard]] bool canStillBet(const Seat &seat) noexcept
        {
            return seat.inHand && seat.stack > 0;
        }

        // Walks clockwise from the seat after `from`, wrapping once.
        // One function, rather than that loop at every place that asks.
        template <typename Predicate>
        [[nodiscard]] std::optional<SeatId> nextSeatFrom(
            const std::vector<Seat> &seats, SeatId from, Predicate predicate)
        {
            const auto count = seats.size();
            for (std::size_t step = 1; step <= count; ++step)
            {
                const auto index = (indexOf(from) + step) % count;
                if (predicate(seats[index]))
                {
                    return makeSeatId(index);
                }
            }
            return std::nullopt;
        }

    } // namespace

    Table::Table(std::size_t seatCount, Blinds blinds)
        : blindLevels(blinds)
    {
        if (seatCount < kMinSeats || seatCount > kMaxSeats)
        {
            throw TableStateError(
                "Table: a table seats between 2 and 9 players");
        }

        // Blinds is a plain aggregate with no invariant of its own.
        // This is the one door it reaches the betting rules through.
        // postBlinds() stakes the small blind against a live big one.
        // A small blind above it therefore owes a negative amount.
        // Chips is unsigned, so owedBy() wraps that figure instead.
        // commit() then hands the wrapped one to the next raise.
        // Refusing beats clamping the subtraction at the read.
        // A clamp leaves the pot just as wrong and harder to notice.
        if (blinds.small > blinds.big)
        {
            throw TableStateError(
                "Table: the small blind cannot exceed the big blind");
        }
        seats.resize(seatCount);
    }

    std::size_t Table::seatCount() const noexcept
    {
        return seats.size();
    }

    Blinds Table::blinds() const noexcept
    {
        return blindLevels;
    }

    const Seat &Table::seatAt(SeatId seat) const
    {
        requireSeatInRange(seat);
        return seats[indexOf(seat)];
    }

    std::optional<SeatId> Table::firstFreeSeat() const
    {
        for (std::size_t index = 0; index < seats.size(); ++index)
        {
            if (!seats[index].occupied)
            {
                return makeSeatId(index);
            }
        }
        return std::nullopt;
    }

    void Table::seatPlayer(SeatId seat, Chips stack)
    {
        requireSeatInRange(seat);
        auto &target = seats[indexOf(seat)];
        if (target.occupied)
        {
            throw TableStateError("Table: that seat is taken");
        }
        target = Seat{};
        target.occupied = true;
        target.stack = stack;
    }

    void Table::addChips(SeatId seat, Chips amount)
    {
        requireSeatInRange(seat);
        auto &target = seats[indexOf(seat)];
        if (!target.occupied)
        {
            throw TableStateError("Table: that seat is empty");
        }
        if (target.inHand)
        {
            throw TableStateError(
                "Table: chips cannot be added in the middle of a hand");
        }
        target.stack += amount;
    }

    void Table::removePlayer(SeatId seat)
    {
        requireSeatInRange(seat);
        auto &target = seats[indexOf(seat)];

        // Folding gives up the cards, not the chips already staked.
        // Those still belong to a pot this hand has to pay out.
        // Clearing the seat drops them from what finishHand() sees.
        // The payouts would fall short by exactly that stake.
        // Clearing the pot afterwards would discard the difference.
        // So leaving waits for the hand to settle, folded or not.
        // That is also what a card room makes a player do.
        // A seat that has staked nothing is free to go at any time.
        if (handInProgress && (target.inHand || target.committed > 0))
        {
            throw TableStateError(
                "Table: a player with chips in the pot cannot leave "
                "the table");
        }
        target = Seat{};
    }

    bool Table::canStartHand() const noexcept
    {
        if (handInProgress)
        {
            return false;
        }
        const auto funded = static_cast<std::size_t>(
            std::count_if(seats.begin(), seats.end(), isDealtIn));
        return funded >= kMinSeats;
    }

    void Table::startHand(IDeck &newDeck)
    {
        if (handInProgress)
        {
            throw TableStateError("Table: a hand is already being played");
        }
        if (!canStartHand())
        {
            throw TableStateError(
                "Table: a hand needs two players with chips");
        }

        flow.begin(newDeck);

        for (auto &seat : seats)
        {
            seat.committed = 0;
            seat.roundCommitted = 0;
            seat.actedThisRound = false;
            seat.mayRaise = true;
            seat.holeCards = {};
            seat.inHand = isDealtIn(seat);
        }

        // canStartHand() found two funded seats.
        // So there is always a next one to move the button to.
        buttonSeat = *nextSeatFrom(seats, buttonSeat, isDealtIn);

        potChips = 0;
        betting.open(blindLevels.big);
        handInProgress = true;
        ++handCount;

        dealHoleCards();
        postBlinds();
    }

    bool Table::isHandInProgress() const noexcept
    {
        return handInProgress;
    }

    Stage Table::stage() const noexcept
    {
        return flow.stage();
    }

    SeatId Table::button() const noexcept
    {
        return buttonSeat;
    }

    const std::vector<Card> &Table::board() const noexcept
    {
        return flow.board();
    }

    Chips Table::pot() const noexcept
    {
        return potChips;
    }

    std::optional<SeatId> Table::seatToAct() const noexcept
    {
        return toAct;
    }

    TableView Table::viewFor(SeatId seat) const
    {
        requireSeatInRange(seat);
        const auto &target = seats[indexOf(seat)];
        const auto owed = betting.owedBy(target.roundCommitted);

        // Copying the board into the view can fail to allocate.
        // The edges gcov reports here are that unwind path only.
        const auto owedToCall = std::min(owed, target.stack);
        return TableView{ // GCOVR_EXCL_LINE
            .seat = seat,
            .stage = flow.stage(),
            .holeCards = target.holeCards,
            .board = flow.board(),
            .pot = potChips,
            .stack = target.stack,
            .currentBet = betting.bet(),
            .toCall = owedToCall,
            .minRaiseTo = betting.minimumRaiseTo(),
            .maxRaiseTo = target.roundCommitted + target.stack,
            .mayRaise = target.mayRaise,
            .playersInHand = countInHand(),
            .blinds = blindLevels,
        };
    }

    void Table::apply(Action action)
    {
        if (!toAct)
        {
            throw TableStateError("Table: nobody is waiting to act");
        }

        const auto actor = *toAct;
        auto &seat = seats[indexOf(actor)];
        switch (action.type)
        {
            case ActionType::Fold:
                seat.inHand = false;
                break;
            case ActionType::Check:
                if (!betting.isCovered(seat.roundCommitted))
                {
                    throw IllegalActionError(
                        "Table: cannot check with a bet to call");
                }
                break;
            case ActionType::Call:
                applyCall(seat);
                break;
            case ActionType::Bet:
                if (betting.isLive())
                {
                    throw IllegalActionError(
                        "Table: a bet is already live, so raise instead");
                }
                applyRaise(actor, action.amount);
                break;
            case ActionType::Raise:
                if (!betting.isLive())
                {
                    throw IllegalActionError(
                        "Table: no bet is live, so bet instead");
                }
                applyRaise(actor, action.amount);
                break;
            default:
                throw IllegalActionError(
                    "Table: that is not one of the five actions");
        }

        seat.actedThisRound = true;
        advanceAfterAction(actor);
    }

    const HandResult &Table::lastResult() const
    {
        if (!result)
        {
            throw TableStateError("Table: no hand has finished yet");
        }
        return *result;
    }

    std::uint64_t Table::handsPlayed() const noexcept
    {
        return handCount;
    }

    void Table::requireSeatInRange(SeatId seat) const
    {
        if (indexOf(seat) >= seats.size())
        {
            throw TableStateError("Table: no such seat at this table");
        }
    }

    std::size_t Table::countInHand() const noexcept
    {
        return static_cast<std::size_t>(
            std::count_if(seats.begin(), seats.end(), isInHand));
    }

    std::size_t Table::countAbleToAct() const noexcept
    {
        return static_cast<std::size_t>(
            std::count_if(seats.begin(), seats.end(), canStillBet));
    }

    void Table::commit(Seat &seat, Chips amount) noexcept
    {
        seat.stack -= amount;
        seat.roundCommitted += amount;
        seat.committed += amount;
        potChips += amount;
    }

    void Table::applyCall(Seat &seat)
    {
        const auto owed = betting.owedBy(seat.roundCommitted);
        if (owed == 0)
        {
            throw IllegalActionError(
                "Table: nothing to call, so check instead");
        }
        commit(seat, std::min(owed, seat.stack));
    }

    void Table::applyRaise(SeatId actor, Chips target)
    {
        auto &seat = seats[indexOf(actor)];
        if (!seat.mayRaise)
        {
            throw IllegalActionError(
                "Table: an all-in short of a full raise reopened nothing, "
                "so calling is all that is left");
        }

        const auto allInTo = seat.roundCommitted + seat.stack;
        if (target > allInTo)
        {
            throw IllegalActionError(
                "Table: cannot stake more than the stack holds");
        }

        // The round owns what beats the live bet; this owns who may.
        const auto reopened = betting.raiseTo(target, allInTo);
        commit(seat, target - seat.roundCommitted);

        if (reopened)
        {
            for (auto &other : seats)
            {
                other.actedThisRound = false;
                other.mayRaise = true;
            }
            return;
        }

        // An all-in short of a full raise reopens nothing.
        // Everyone who already acted keeps their turn spent.
        // They owe the difference to stay, but may not raise again.
        // Players yet to speak still have a full turn.
        for (auto &other : seats)
        {
            if (other.actedThisRound)
            {
                other.mayRaise = false;
            }
        }
    }

    void Table::dealHoleCards()
    {
        const auto players = countInHand();
        for (std::size_t round = 0; round < kHoleCardCount; ++round)
        {
            auto seat = buttonSeat;
            for (std::size_t dealt = 0; dealt < players; ++dealt)
            {
                seat = *nextSeatFrom(seats, seat, isInHand);
                seats[indexOf(seat)].holeCards[round] = flow.dealCard();
            }
        }
    }

    void Table::postBlinds()
    {
        // Heads-up the button posts the small blind.
        // That makes it first pre-flop and last on every street after.
        // So the rule below covers two players with no second case.
        const auto headsUp = countInHand() == kMinSeats;
        const auto smallSeat = headsUp
                                   ? buttonSeat
                                   : *nextSeatFrom(
                                         seats, buttonSeat, isInHand);
        const auto bigSeat = *nextSeatFrom(seats, smallSeat, isInHand);

        auto &small = seats[indexOf(smallSeat)];
        commit(small, std::min(blindLevels.small, small.stack));
        auto &big = seats[indexOf(bigSeat)];
        commit(big, std::min(blindLevels.big, big.stack));

        openBetting(bigSeat);
    }

    void Table::openBetting(SeatId from)
    {
        const auto next = nextSeatFrom(seats, from, canStillBet);

        // Two players holding chips have an ordinary round between them.
        // One on its own still has a decision when it owes the blind.
        // No-limit gives that player call-or-fold.
        // Skipping it treats them as all-in for what they posted.
        // That caps them out of a layer they never paid for.
        // On a later street resetBettingRound() has zeroed every debt.
        // So the second test only ever fires pre-flop.
        const auto owes =
            next
            && !betting.isCovered(seats[indexOf(*next)].roundCommitted);
        if (countAbleToAct() >= kMinSeats || owes)
        {
            toAct = next;
            return;
        }

        // Nobody is left owing chips they could still wager with.
        // So there is nothing to decide on this street or any later one.
        toAct = std::nullopt;
        closeRound();
    }

    void Table::advanceAfterAction(SeatId actor)
    {
        if (countInHand() <= 1)
        {
            finishWithoutShowdown();
            return;
        }

        const auto owesOrOwed = [this](const Seat &seat)
        {
            return canStillBet(seat)
                   && (!seat.actedThisRound
                       || !betting.isCovered(seat.roundCommitted));
        };
        if (const auto next = nextSeatFrom(seats, actor, owesOrOwed))
        {
            toAct = *next;
            return;
        }

        toAct = std::nullopt;
        closeRound();
    }

    void Table::resetBettingRound() noexcept
    {
        for (auto &seat : seats)
        {
            seat.roundCommitted = 0;
            seat.actedThisRound = false;
            seat.mayRaise = true;
        }
        betting.reset(blindLevels.big);
    }

    void Table::closeRound()
    {
        while (flow.hasStreetToDeal())
        {
            flow.dealStreet();
            resetBettingRound();

            if (countAbleToAct() >= kMinSeats)
            {
                // Post-flop, action opens left of the button.
                // Heads-up included.
                toAct = nextSeatFrom(seats, buttonSeat, canStillBet);
                return;
            }
        }

        finishWithShowdown();
    }

    void Table::finishWithoutShowdown()
    {
        // The survivor is the only seat eligible for any layer.
        // So the split hands them their own uncalled bet back.
        // No separate step is needed for it.
        auto scores = scoreWithoutShowdown(seats.size());
        finishHand(scores.values, std::move(scores.entries));
    }

    void Table::finishWithShowdown()
    {
        flow.toShowdown();

        auto scores = scoreShowdown(seats, flow.board());
        finishHand(scores.values, std::move(scores.entries));
    }

    void Table::finishHand(
        const std::vector<HandValue> &values,
        std::vector<ShowdownEntry> entries)
    {
        std::vector<Chips> committed;
        std::vector<SeatId> eligibleSeats;
        committed.reserve(seats.size());
        for (std::size_t index = 0; index < seats.size(); ++index)
        {
            committed.push_back(seats[index].committed);
            if (seats[index].inHand)
            {
                eligibleSeats.push_back(makeSeatId(index));
            }
        }

        std::vector<SeatId> payoutOrder;
        payoutOrder.reserve(seats.size());
        for (std::size_t step = 1; step <= seats.size(); ++step)
        {
            payoutOrder.push_back(
                makeSeatId((indexOf(buttonSeat) + step) % seats.size()));
        }

        const auto pots = buildSidePots(committed, eligibleSeats);
        const auto payouts = distributePots(pots, values, payoutOrder);
        for (const auto &payout : payouts)
        {
            seats[indexOf(payout.seat)].stack += payout.amount;
        }

        // Handing the result to the optional allocates three vectors.
        // The edges gcov reports here are those unwind paths only.
        result = HandResult{ // GCOVR_EXCL_LINE
            .pot = potChips,
            .payouts = payouts,
            .showdown = std::move(entries),
            .board = flow.board(),
            .stage = flow.stage(),
        };

        potChips = 0;
        betting.close();
        toAct = std::nullopt;
        handInProgress = false;
        flow.end();
        // Every per-round figure goes back to what a fresh seat holds.
        // betting.close() has just zeroed the live bet above them.
        // A seat still holding roundCommitted would owe a negative.
        // viewFor() reported that as owing its whole stack to call.
        // committed is the one exception and stays where it is.
        // TableRunner reads it either side of apply() for the stake.
        // A hand-ending action is paid out before it looks.
        for (auto &seat : seats)
        {
            seat.inHand = false;
            seat.actedThisRound = false;
            seat.roundCommitted = 0;
            seat.mayRaise = true;
        }
    }

    TableMemory Table::remember() const
    {
        // Standing the memory up copies two vectors and an optional.
        // The edges gcov reports here are those unwind paths only.
        return TableMemory{ // GCOVR_EXCL_LINE
            .seats = seats,
            .result = result,
            .toAct = toAct,
            .pot = potChips,
            .betting = betting.remember(),
            .stage = flow.stage(),
            .board = flow.board(),
            .handCount = handCount,
            .button = buttonSeat,
            .handInProgress = handInProgress};

        // gcov puts the returned value's unwind block here.
        // No input reaches it.
    } // GCOVR_EXCL_LINE

    void Table::restore(const TableMemory &memory, IDeck &deck)
    {
        if (memory.seats.size() != seats.size())
        {
            throw TableStateError(
                "antwika::holdem: a memory of another table's seat "
                "count cannot stand here");
        }

        if (memory.handInProgress && !memory.toAct.has_value())
        {
            throw TableStateError(
                "antwika::holdem: a hand in progress always has a "
                "seat to act");
        }

        seats = memory.seats;
        result = memory.result;
        toAct = memory.toAct;
        potChips = memory.pot;
        betting.restore(memory.betting);
        flow.resume(memory.stage, memory.board);
        handCount = memory.handCount;
        buttonSeat = memory.button;
        handInProgress = memory.handInProgress;

        if (handInProgress)
        {
            flow.adopt(deck);
        }
    }

} // namespace antwika::holdem
