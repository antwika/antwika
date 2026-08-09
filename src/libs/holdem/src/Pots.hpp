#pragma once

#include <span>
#include <vector>

#include "antwika/holdem/Chips.hpp"
#include "antwika/holdem/HandValue.hpp"
#include "antwika/holdem/Payout.hpp"
#include "antwika/holdem/SeatId.hpp"
#include "antwika/holdem/SidePot.hpp"

namespace antwika::holdem
{

    [[nodiscard]] std::vector<SidePot> buildSidePots(
        std::span<const Chips> committed,
        std::span<const SeatId> eligibleSeats);

    [[nodiscard]] std::vector<Payout> distributePots(
        std::span<const SidePot> pots,
        std::span<const HandValue> valuesBySeat,
        std::span<const SeatId> payoutOrder);

}
