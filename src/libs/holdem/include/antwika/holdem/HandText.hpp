#pragma once

#include <string>

#include "antwika/holdem/HandValue.hpp"

namespace antwika::holdem
{

    /**
     * @brief Describe a hand's strength the way a player would say it.
     *
     * Reads the ranks back out of the value itself, so a description
     * needs neither the cards it came from nor the board it was made
     * with.
     * @param value The hand value to describe, as evaluate() composed
     * it; a value composed any other way describes an unknown hand.
     * @return The description, e.g. "a pair of Sevens".
     */
    [[nodiscard]] std::string describe(HandValue value);

} // namespace antwika::holdem
