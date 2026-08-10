#pragma once

#include <nlohmann/json_fwd.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include <antwika/replay/MigrationChain.hpp>
#include <antwika/ecs/World.hpp>

#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/CityGrid.hpp"
#include "antwika/game/Errand.hpp"
#include "antwika/game/GameState.hpp"
#include "antwika/game/GameSummary.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/Employment.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/Staff.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/Ruin.hpp"
#include "antwika/game/SceneSnapshot.hpp"
#include "antwika/game/Service.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    using antwika::replay::MigrationChain;

    inline constexpr std::string_view kSaveMagic = "antwika-game-save";

    inline constexpr std::uint32_t kSaveFormatVersion = 4;

    [[nodiscard]] MigrationChain standardSaveMigrations();

    struct SavedErrand final
    {
        std::optional<std::size_t> destination = std::nullopt;

        Resource carrying = Resource::Food;

        ErrandLeg leg = ErrandLeg::Outbound;

        [[nodiscard]] bool operator==(const SavedErrand &other) const
            = default;
    };

    struct SavedJourney final
    {
        Cell towards;

        std::optional<std::size_t> house = std::nullopt;

        [[nodiscard]] bool operator==(const SavedJourney &other) const
            = default;
    };

    struct SavedWalker final
    {
        Cell at;
        Direction facing = Direction::East;
        WalkerKind kind = WalkerKind::WaterCarrier;
        std::int32_t carried = 0;
        std::int32_t stepsUntilHome = kRoamingSteps;
        std::uint8_t ticksUntilStep = 0;

        std::optional<std::size_t> home = std::nullopt;

        std::optional<SavedErrand> errand = std::nullopt;

        std::optional<SavedJourney> journey = std::nullopt;

        std::optional<std::size_t> fireCall = std::nullopt;

        [[nodiscard]] bool operator==(const SavedWalker &other) const
            = default;
    };

    struct SavedBuilding final
    {
        Cell at;
        BuildingKind kind = BuildingKind::House;
        std::array<std::int32_t, kResourceCount> stock{};

        std::int32_t risk = 0;

        std::int32_t collapseRisk = 0;

        std::int32_t diseaseRisk = 0;
        std::int32_t ticksUntilSpawn = 0;
        std::int32_t ticksUntilDrain = 0;
        std::int32_t ticksUntilRisk = 0;

        Resource selling = Resource::Food;

        std::vector<std::size_t> walkers = {};

        std::array<std::int32_t, kServiceCount> coverage{};

        std::optional<std::int32_t> ticksUntilOutput = std::nullopt;

        std::optional<Household> household = std::nullopt;

        std::optional<StoredStaff> staff = std::nullopt;

        std::optional<StoredEmployment> employment = std::nullopt;

        [[nodiscard]] bool operator==(const SavedBuilding &other) const
            = default;
    };

    struct SavedRuin final
    {
        Cell at;
        BuildingKind kind = BuildingKind::House;
        RuinState state = RuinState::Burning;

        std::int32_t ticksUntilOut = 0;

        [[nodiscard]] bool operator==(const SavedRuin &other) const
            = default;
    };

    struct SaveGame final
    {
        GameState state;

        GridExtent extent;

        Camera camera;

        std::vector<Cell> paths;

        std::vector<SavedWalker> walkers;

        std::vector<SavedBuilding> buildings;

        std::vector<SavedRuin> ruins;

        std::uint64_t seed = 0;

        [[nodiscard]] bool operator==(const SaveGame &other) const = default;
    };

    [[nodiscard]] nlohmann::json saveGameToJson(const SaveGame &save);

    [[nodiscard]] SaveGame saveGameFromJson(const nlohmann::json &j);

    [[nodiscard]] SaveGame saveGameFrom(
        const CityGrid &grid,
        const PathIndex &paths,
        const Camera &camera,
        const GameState &state,
        GridExtent extent,
        std::uint64_t seed = 0);

    [[nodiscard]] SaveGame saveGameOf(
        const antwika::ecs::World &world,
        const PathIndex &paths,
        const Camera &camera,
        const GameState &state,
        GridExtent extent,
        std::uint64_t seed = 0);

    [[nodiscard]] PathIndex pathIndexOf(const SaveGame &save);

}
