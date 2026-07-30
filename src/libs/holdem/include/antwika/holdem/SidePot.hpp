#pragma once

#include <vector>

#include "antwika/holdem/Chips.hpp"
#include "antwika/holdem/SeatId.hpp"

namespace antwika::holdem
{

    /**
     * @brief One layer of the pot, together with the seats allowed to
     * win it.
     *
     * A player who is all-in for less than the others can only win what
     * they could have covered, so the pot splits into layers at every
     * all-in amount.
     */
    struct SidePot
    {
        /**
         * @brief Chips in this layer.
         */
        Chips amount{};

        /**
         * @brief Seats eligible to win it, in ascending seat order.
         */
        std::vector<SeatId> contenders;

        bool operator==(const SidePot &other) const = default;
    };

} // namespace antwika::holdem
