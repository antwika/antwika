#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "antwika/holdem/Card.hpp"
#include "antwika/holdem/Chips.hpp"
#include "antwika/holdem/HandResult.hpp"
#include "antwika/holdem/Seat.hpp"
#include "antwika/holdem/SeatId.hpp"
#include "antwika/holdem/Stage.hpp"

namespace antwika::holdem
{

    struct DeckMemory final
    {
        std::array<Card, kCardCount> cards{};

        std::size_t dealt = 0;

        [[nodiscard]] bool operator==(
            const DeckMemory &other) const = default;
    };

    struct BettingMemory final
    {
        Chips currentBet = 0;

        Chips lastRaiseSize = 0;

        [[nodiscard]] bool operator==(
            const BettingMemory &other) const = default;
    };

    struct TableMemory final
    {
        std::vector<Seat> seats;

        std::optional<HandResult> result;

        std::optional<SeatId> toAct;

        Chips pot = 0;

        BettingMemory betting;

        Stage stage = Stage::PreFlop;

        std::vector<Card> board;

        std::uint64_t handCount = 0;

        SeatId button{};

        bool handInProgress = false;

        [[nodiscard]] bool operator==(
            const TableMemory &other) const = default;
    };

}
