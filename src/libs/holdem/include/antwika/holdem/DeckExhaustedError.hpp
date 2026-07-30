#pragma once

#include <stdexcept>

namespace antwika::holdem
{

    /**
     * @brief Thrown when a card is asked of a deck that has none left.
     *
     * Unreachable from a hold'em table -- nine players and a board come
     * to 23 of 52 cards -- but a deck is worth being honest about its
     * own emptiness rather than dealing a card twice.
     */
    class DeckExhaustedError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::holdem
