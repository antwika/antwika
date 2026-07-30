#pragma once

#include <array>
#include <string>

#include <antwika/holdem/Card.hpp>
#include <antwika/holdem/Chips.hpp>
#include <antwika/holdem/Limits.hpp>

namespace antwika::poker
{

    using antwika::holdem::Card;
    using antwika::holdem::Chips;
    using antwika::holdem::kHoleCardCount;

    /**
     * @brief Everything a spectator may see about one seat, as a value.
     */
    struct SeatSnapshot
    {
        /**
         * @brief Who is sitting here, empty if nobody is.
         */
        std::string name{};

        /**
         * @brief Chips in front of this seat, not yet wagered.
         */
        Chips stack{};

        /**
         * @brief Chips this seat has put in across the whole hand.
         */
        Chips committed{};

        /**
         * @brief Chips this seat has put in during this betting round.
         */
        Chips roundCommitted{};

        /**
         * @brief This seat's cards, meaningful only while inHand.
         */
        std::array<Card, kHoleCardCount> holeCards{};

        /**
         * @brief Whether a player occupies this seat at all.
         */
        bool occupied = false;

        /**
         * @brief Whether this seat was dealt in and has not folded.
         */
        bool inHand = false;

        /**
         * @brief Whether the dealer button is on this seat.
         */
        bool isButton = false;

        /**
         * @brief Whether the hand is waiting on this seat to act.
         */
        bool isToAct = false;

        bool operator==(const SeatSnapshot &other) const = default;
    };

} // namespace antwika::poker
