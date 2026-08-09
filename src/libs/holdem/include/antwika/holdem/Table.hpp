#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "antwika/holdem/Action.hpp"
#include "antwika/holdem/BettingRound.hpp"
#include "antwika/holdem/Blinds.hpp"
#include "antwika/holdem/Card.hpp"
#include "antwika/holdem/Chips.hpp"
#include "antwika/holdem/HandFlow.hpp"
#include "antwika/holdem/HandResult.hpp"
#include "antwika/holdem/HandValue.hpp"
#include "antwika/holdem/IDeck.hpp"
#include "antwika/holdem/Seat.hpp"
#include "antwika/holdem/SeatId.hpp"
#include "antwika/holdem/ShowdownEntry.hpp"
#include "antwika/holdem/Stage.hpp"
#include "antwika/holdem/TableMemory.hpp"
#include "antwika/holdem/TableView.hpp"

namespace antwika::holdem
{

    class Table final
    {
    public:
        Table(std::size_t seatCount, Blinds blinds);

        Table(const Table &) = delete;
        Table(Table &&) = delete;

        Table &operator=(const Table &) = delete;
        Table &operator=(Table &&) = delete;

        [[nodiscard]] std::size_t seatCount() const noexcept;

        [[nodiscard]] Blinds blinds() const noexcept;

        [[nodiscard]] const Seat &seatAt(SeatId seat) const;

        [[nodiscard]] std::optional<SeatId> firstFreeSeat() const;

        void seatPlayer(SeatId seat, Chips stack);

        void addChips(SeatId seat, Chips amount);

        void removePlayer(SeatId seat);

        [[nodiscard]] bool canStartHand() const noexcept;

        void startHand(IDeck &deck);

        [[nodiscard]] bool isHandInProgress() const noexcept;

        [[nodiscard]] Stage stage() const noexcept;

        [[nodiscard]] SeatId button() const noexcept;

        [[nodiscard]] const std::vector<Card> &board() const noexcept;

        [[nodiscard]] Chips pot() const noexcept;

        [[nodiscard]] std::optional<SeatId> seatToAct() const noexcept;

        [[nodiscard]] TableView viewFor(SeatId seat) const;

        void apply(Action action);

        [[nodiscard]] const HandResult &lastResult() const;

        [[nodiscard]] std::uint64_t handsPlayed() const noexcept;

        [[nodiscard]] TableMemory remember() const;

        void restore(const TableMemory &memory, IDeck &deck);

    private:
        std::vector<Seat> seats;
        Blinds blindLevels;
        std::optional<HandResult> result;
        std::optional<SeatId> toAct;
        Chips potChips = 0;
        BettingRound betting;
        HandFlow flow;
        std::uint64_t handCount = 0;
        SeatId buttonSeat{};
        bool handInProgress = false;

        void requireSeatInRange(SeatId seat) const;
        [[nodiscard]] std::size_t countInHand() const noexcept;
        [[nodiscard]] std::size_t countAbleToAct() const noexcept;
        void commit(Seat &seat, Chips amount) noexcept;
        void applyCall(Seat &seat);
        void applyRaise(SeatId actor, Chips target);
        void dealHoleCards();
        void postBlinds();
        void openBetting(SeatId from);
        void advanceAfterAction(SeatId actor);
        void resetBettingRound() noexcept;
        void closeRound();
        void finishWithoutShowdown();
        void finishWithShowdown();
        void finishHand(
            const std::vector<HandValue> &values,
            std::vector<ShowdownEntry> entries);
    };

}
