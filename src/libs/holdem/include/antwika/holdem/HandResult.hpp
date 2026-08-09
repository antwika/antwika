#pragma once

#include <vector>

#include "antwika/holdem/Card.hpp"
#include "antwika/holdem/Chips.hpp"
#include "antwika/holdem/Payout.hpp"
#include "antwika/holdem/ShowdownEntry.hpp"
#include "antwika/holdem/Stage.hpp"

namespace antwika::holdem
{

    struct HandResult final
    {
        Chips pot{};

        std::vector<Payout> payouts;

        std::vector<ShowdownEntry> showdown;

        std::vector<Card> board;

        Stage stage{};

        bool operator==(const HandResult &other) const = default;
    };

}
