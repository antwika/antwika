#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cost.hpp"
#include "antwika/game/Employment.hpp"
#include "antwika/game/GameState.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/Production.hpp"
#include "antwika/game/Ruin.hpp"
#include "antwika/game/Staff.hpp"

namespace antwika::game
{

    /**
     * @brief How many walkers may exist at once, across the whole run.
     *
     * A run left going has an unbounded number of buildings on it and no
     * reason for any walker to leave, so without a cap the population and
     * the per-tick work behind it grow for ever. Sixty-four is well past
     * what a 24x24 grid reads as busy and far below what a tick costs
     * anything to walk.
     *
     * A building at the cap *holds* its countdown at zero rather than
     * resetting it, so the moment somebody wanders off the end of the
     * world -- or the cap is raised -- the next one leaves at once.
     *
     * Declared here rather than in SpawnSystem.hpp, where it used to
     * live, because every system that counts walkers now reads the cap
     * off a GameConfig and this constant is only its default.
     */
    inline constexpr std::size_t kWalkerLimit = 64;

    /**
     * @brief The numbers a session's rules run on, gathered as a value.
     *
     * Every field defaults to the constant it externalizes, so a
     * default-constructed GameConfig *is* the shipped game and a test that
     * says nothing about config exercises exactly what it always did.
     * The application reads config.json beside its assets at startup --
     * see ConfigFile.hpp -- and hands the result in through bootstrap(),
     * so a rebalance is an edit to a file rather than to this header.
     *
     * **A GameConfig is part of the game's definition, exactly as the source
     * and the art are.** Nothing here is recorded: a replay assumes the
     * config it was recorded under, as it assumes the build it was
     * recorded by, and replaying a session under an edited config is
     * running it on a different game. That is also why nothing here may
     * decide a layout or a pixel -- a widget moved by a config file
     * would re-aim every recorded click that hit it -- and why what a
     * save persists are the countdowns themselves, which restore as
     * written whatever the periods say now.
     *
     * A plain value with public fields rather than a class: it is data,
     * every combination of fields is a game somebody may want, and the
     * systems it is handed to copy it, so no lifetime rule is needed.
     * What a *file* may state is narrower than what the fields hold --
     * ConfigFile.cpp's schema is where a zero period or a negative cost
     * is refused, beside the parse that would admit it.
     */
    struct GameConfig
    {
        /** @brief What the bank opens with. */
        std::int64_t startingMoney = kStartingMoney;

        /** @brief What laying one cell of road takes out of the bank. */
        std::int64_t roadCost = kRoadCost;

        /** @brief What one press of the raze tool takes out of it. */
        std::int64_t razeCost = kRazeCost;

        /**
         * @brief What putting up each kind of building takes, keyed by
         * kind for kBuildingCosts' reason exactly.
         */
        std::array<std::int64_t, kBuildingKindCount> buildingCosts =
            kBuildingCosts;

        /** @brief Ticks between a building's risk steps. */
        std::int32_t riskPeriodTicks = kRiskPeriodTicks;

        /** @brief Ticks between a household's meals. */
        std::int32_t drainPeriodTicks = kDrainPeriodTicks;

        /** @brief Occupants one unit of every held resource serves. */
        std::int32_t mouthsPerServing = kMouthsPerServing;

        /** @brief Ticks between a building's walkers setting out. */
        std::int32_t spawnPeriodTicks = kSpawnPeriodTicks;

        /** @brief Ticks a fire burns before it is debris. */
        std::int32_t burnDurationTicks = kBurnDurationTicks;

        /** @brief Ticks between people moving into a qualifying house. */
        std::int32_t settlerPeriodTicks = kSettlerPeriodTicks;

        /** @brief Ticks a house must go on qualifying before it grows. */
        std::int32_t evolvePeriodTicks = kEvolvePeriodTicks;

        /** @brief Ticks a house must go on failing before it falls. */
        std::int32_t devolvePeriodTicks = kDevolvePeriodTicks;

        /** @brief Ticks between a workshop's or a farm's batches. */
        std::int32_t productionPeriodTicks = kProductionPeriodTicks;

        /** @brief Units one production period turns out. */
        std::int32_t productionBatch = kProductionBatch;

        /** @brief Ticks between a workplace's calls for labour. */
        std::int32_t labourPeriodTicks = kLabourPeriodTicks;

        /** @brief Ticks a workplace keeps a worker nobody replaced. */
        std::int32_t staffDecayPeriodTicks = kStaffDecayPeriodTicks;

        /** @brief How many walkers may exist at once. */
        std::size_t walkerLimit = kWalkerLimit;

        /**
         * @brief What putting up a building of this kind costs.
         * @param kind The kind being placed.
         * @return The configured cost.
         */
        [[nodiscard]] constexpr std::int64_t costOf(
            BuildingKind kind) const noexcept
        {
            return buildingCosts[
                buildingKindIndex(kind) % kBuildingKindCount];
        }

        /**
         * @brief Compare two tunings.
         * @param other The config to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const GameConfig &other) const = default;
    };

} // namespace antwika::game
