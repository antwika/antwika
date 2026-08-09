#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

#include <antwika/ecs/World.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Coverage.hpp"
#include "antwika/game/Errand.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/Journey.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Production.hpp"
#include "antwika/game/Ruin.hpp"
#include "antwika/game/Walker.hpp"
#include "antwika/game/Employment.hpp"
#include "antwika/game/Staff.hpp"

namespace antwika::game
{

    using antwika::ecs::World;

    struct StoredWalker final
    {
        Cell at;

        Walker walker;

        std::optional<std::size_t> home = std::nullopt;

        std::optional<Errand> errand = std::nullopt;

        std::optional<std::size_t> destination = std::nullopt;

        std::optional<Journey> journey = std::nullopt;

        std::optional<std::size_t> joining = std::nullopt;

        std::optional<std::size_t> attending = std::nullopt;

        [[nodiscard]] bool operator==(const StoredWalker &other) const
            = default;
    };

    struct StoredBuilding final
    {
        Cell at;

        Building building;

        std::array<std::optional<std::size_t>, kMaxWalkersOut> walkers{};

        Coverage coverage{};

        std::optional<Production> production = std::nullopt;

        std::optional<Household> household = std::nullopt;

        std::optional<StoredStaff> staff = std::nullopt;

        std::optional<StoredEmployment> employment = std::nullopt;

        [[nodiscard]] bool operator==(const StoredBuilding &other) const
            = default;
    };

    struct StoredRuin final
    {
        Cell at;

        Ruin ruin;

        [[nodiscard]] bool operator==(const StoredRuin &other) const
            = default;
    };

    struct CityGrid final
    {
        std::vector<StoredWalker> walkers;

        std::vector<StoredBuilding> buildings;

        std::vector<StoredRuin> ruins;

        [[nodiscard]] bool operator==(const CityGrid &other) const
            = default;
    };

    [[nodiscard]] CityGrid cityGridOf(const World &world);

    void restoreCityGrid(
        World &world,
        BuildingIndex &built,
        const PathIndex &paths,
        const CityGrid &grid);

}
