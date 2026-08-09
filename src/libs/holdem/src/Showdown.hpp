#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "antwika/holdem/Card.hpp"
#include "antwika/holdem/HandValue.hpp"
#include "antwika/holdem/Seat.hpp"
#include "antwika/holdem/ShowdownEntry.hpp"

namespace antwika::holdem
{

    struct ShowdownScores final
    {
        std::vector<HandValue> values;

        std::vector<ShowdownEntry> entries;
    };

    [[nodiscard]] ShowdownScores scoreShowdown(
        std::span<const Seat> seats, std::span<const Card> board);

    [[nodiscard]] ShowdownScores scoreWithoutShowdown(std::size_t seatCount);

}
