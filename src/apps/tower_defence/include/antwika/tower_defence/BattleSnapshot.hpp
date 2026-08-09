#pragma once

#include <cstdint>
#include <vector>

#include "antwika/tower_defence/Campaign.hpp"
#include "antwika/tower_defence/Level.hpp"
#include "antwika/tower_defence/MobKind.hpp"

namespace antwika::tower_defence
{

    struct MobMarker final
    {
        Cell cell;
        MobKind kind = MobKind::Grunt;

        [[nodiscard]] bool operator==(const MobMarker &) const = default;
    };

    struct BattleSnapshot final
    {
        const Level &level;

        std::vector<MobMarker> mobs;

        std::vector<Cell> towers;

        std::uint32_t towerRangeSquared = 0;
    };

    [[nodiscard]] BattleSnapshot snapshotOf(const Campaign &campaign);

    [[nodiscard]] std::uint32_t rangeRadius(std::uint32_t rangeSquared);

}
