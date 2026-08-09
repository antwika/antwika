#pragma once

#include <array>
#include <cstdint>

#include "antwika/tower_defence/Campaign.hpp"
#include "antwika/tower_defence/MobKind.hpp"

namespace antwika::tower_defence
{

    struct TowerDefenceConfig final
    {
        std::uint32_t startingLives = kStartingLives;
        std::int32_t framePeriodMs = 80;

        std::array<MobProfile, kMobKindCount> mobs = kDefaultMobProfiles;
    };

}
