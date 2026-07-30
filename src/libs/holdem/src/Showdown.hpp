#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "antwika/holdem/Card.hpp"
#include "antwika/holdem/HandValue.hpp"
#include "antwika/holdem/Seat.hpp"
#include "antwika/holdem/ShowdownEntry.hpp"

/**
 * @file
 * @brief Scoring the hands still standing at a showdown.
 *
 * Private to the library, and beside Pots.hpp for the same reason: this
 * is what the pot is split by, and the split is what it feeds. Reads
 * seats and the board and writes neither, so it is a function rather
 * than a piece of Table.
 */
namespace antwika::holdem
{

    /**
     * @brief What a showdown worked out, in the two shapes it is
     * wanted in.
     */
    struct ShowdownScores final
    {
        /**
         * @brief Each seat's hand strength, indexed by seat.
         *
         * A seat that is not in the hand keeps a default HandValue, so
         * the vector can be indexed by any seat the pots name.
         */
        std::vector<HandValue> values;

        /**
         * @brief The seats that showed down, strongest hand first.
         *
         * Ties keep their seat order, so equal hands read around the
         * table rather than in whichever order a sort left them.
         */
        std::vector<ShowdownEntry> entries;
    };

    /**
     * @brief Score every hand still in at the showdown.
     *
     * @param seats Every seat at the table; those not in the hand are
     * skipped, not scored.
     * @param board The community cards, which every hand is made with.
     * @return The scores, one entry per seat still in the hand.
     */
    [[nodiscard]] ShowdownScores scoreShowdown(
        std::span<const Seat> seats, std::span<const Card> board);

    /**
     * @brief Score a hand that never reached a showdown.
     *
     * Nothing is compared, because the last player standing wins every
     * layer they are eligible for whatever they hold -- which is also
     * how their own uncalled bet comes back to them.
     *
     * @param seatCount How many seats the table has.
     * @return Default values for every seat and no entries at all.
     */
    [[nodiscard]] ShowdownScores scoreWithoutShowdown(std::size_t seatCount);

} // namespace antwika::holdem
