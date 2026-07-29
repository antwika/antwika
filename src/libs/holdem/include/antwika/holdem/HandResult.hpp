#pragma once

#include <vector>

#include "antwika/holdem/Card.hpp"
#include "antwika/holdem/Chips.hpp"
#include "antwika/holdem/Payout.hpp"
#include "antwika/holdem/ShowdownEntry.hpp"
#include "antwika/holdem/Stage.hpp"

namespace antwika::holdem
{

    /**
     * @brief What happened in a finished hand.
     */
    struct HandResult
    {
        /**
         * @brief Chips that were in the middle, before being paid out.
         */
        Chips pot{};

        /**
         * @brief What each paid seat received, in ascending seat order.
         *
         * A lone survivor's own uncalled bet comes back to them through
         * here as well, because the top layer of the pot has nobody else
         * eligible for it.
         */
        std::vector<Payout> payouts;

        /**
         * @brief Every seat that revealed cards, strongest first.
         *
         * Empty when the hand ended with everyone but one player
         * folding, which is the case where no cards are shown.
         */
        std::vector<ShowdownEntry> showdown;

        /**
         * @brief The board as it stood when the hand ended.
         */
        std::vector<Card> board;

        /**
         * @brief The stage the hand ended on.
         */
        Stage stage{};

        bool operator==(const HandResult &other) const = default;
    };

} // namespace antwika::holdem
