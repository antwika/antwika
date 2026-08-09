#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include <antwika/enums/Enumeration.hpp>

#include "antwika/game/AtlasAssets.hpp"
#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cost.hpp"
#include "antwika/game/Employment.hpp"
#include "antwika/game/GameState.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/Production.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/Ruin.hpp"
#include "antwika/game/Staff.hpp"
#include "antwika/game/TileAtlas.hpp"

namespace antwika::game
{

    inline constexpr std::size_t kWalkerLimit = 64;

    [[nodiscard]] constexpr std::array<bool, kResourceCount>
    defaultSustaining() noexcept
    {
        std::array<bool, kResourceCount> sustaining{};

        for (const auto resource : kResources)
        {
            sustaining[resourceIndex(resource)] = sustains(resource);
        }

        return sustaining;
    }

    [[nodiscard]] inline AtlasAssets defaultAtlases()
    {
        AtlasAssets assets;
        assets.byKind = {"atlas_1x1.png", "atlas_2x2.png", "atlas_3x3.png"};
        assets.walker = "walker_1x1.png";

        return assets;
    }

    struct GameConfig final
    {
        std::int64_t startingMoney = kStartingMoney;

        std::int64_t roadCost = kRoadCost;

        std::int64_t razeCost = kRazeCost;

        std::array<std::int64_t, kBuildingKindCount> buildingCosts =
            kBuildingCosts;

        std::int32_t riskPeriodTicks = kRiskPeriodTicks;

        std::int32_t drainPeriodTicks = kDrainPeriodTicks;

        std::int32_t mouthsPerServing = kMouthsPerServing;

        std::int32_t spawnPeriodTicks = kSpawnPeriodTicks;

        std::int32_t burnDurationTicks = kBurnDurationTicks;

        std::int32_t spreadDelayTicks = kSpreadDelayTicks;

        std::int32_t migrantPeriodTicks = kMigrantPeriodTicks;

        std::int32_t evolvePeriodTicks = kEvolvePeriodTicks;

        std::int32_t devolvePeriodTicks = kDevolvePeriodTicks;

        std::int32_t productionPeriodTicks = kProductionPeriodTicks;

        std::int32_t productionBatch = kProductionBatch;

        std::int32_t labourPeriodTicks = kLabourPeriodTicks;

        std::int32_t staffDecayPeriodTicks = kStaffDecayPeriodTicks;

        std::size_t walkerLimit = kWalkerLimit;

        std::array<bool, kResourceCount> sustaining =
            defaultSustaining();

        AtlasAssets atlases = defaultAtlases();

        [[nodiscard]] constexpr std::int64_t costOf(
            BuildingKind kind) const noexcept
        {
            return antwika::enums::pick(buildingCosts, kind);
        }

        [[nodiscard]] bool operator==(const GameConfig &other) const = default;
    };

}
