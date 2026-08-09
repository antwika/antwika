#pragma once

#include <string>
#include <vector>

#include <cstdint>
#include <map>

#include <antwika/holdem/Chips.hpp>

namespace antwika::poker
{

    using antwika::holdem::Chips;

    struct RoomSummary final
    {
        std::uint64_t handsPlayed = 0;

        std::map<std::string, Chips> balances;

        Chips chipsLeftOnTable = 0;

        std::vector<std::string> console;

        bool operator==(const RoomSummary &other) const = default;
    };

}
