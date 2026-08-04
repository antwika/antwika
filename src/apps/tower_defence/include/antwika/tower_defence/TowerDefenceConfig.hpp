#pragma once

#include <array>
#include <cstdint>

#include "antwika/tower_defence/Campaign.hpp"
#include "antwika/tower_defence/MobKind.hpp"

namespace antwika::tower_defence
{

    /**
     * @brief The numbers this application reads off config.json
     * beside its assets at startup.
     *
     * Every field defaults to the value it externalizes, so a
     * default-constructed one is the shipped application and a
     * missing file changes nothing. The format's mechanics live in
     * antwika::config; what may not move here is anything a recorded
     * click's meaning depends on, for the reasons apps/game's page
     * gives.
     */
    struct TowerDefenceConfig
    {
        /** @brief Leaks the player can afford before the run is over. */
        std::uint32_t startingLives = kStartingLives;
        /** @brief Milliseconds one frame takes on the wall clock. */
        std::int32_t framePeriodMs = 80;

        /**
         * @brief What each mob kind is worth, costs and survives.
         *
         * A lookup keyed by kind rather than a constant only this
         * build can see -- kDefaultMobProfiles is what a run that says
         * nothing plays.
         */
        std::array<MobProfile, kMobKindCount> mobs = kDefaultMobProfiles;
    };

} // namespace antwika::tower_defence
