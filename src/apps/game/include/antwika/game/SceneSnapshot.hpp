#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include <antwika/ecs/World.hpp>
#include <antwika/gfx/Point.hpp>

#include "antwika/game/BuildGhost.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/MapView.hpp"
#include "antwika/game/OverlayField.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/RoadPlan.hpp"
#include "antwika/game/Ruin.hpp"
#include "antwika/game/Service.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    using antwika::ecs::World;
    using antwika::gfx::Point;

    struct WalkerView final
    {
        Cell at;
        Direction facing = Direction::East;

        [[nodiscard]] bool operator==(const WalkerView &other) const = default;
    };

    struct WalkerSprite final
    {
        Cell at;
        Direction facing = Direction::East;

        std::optional<Cell> from{};

        std::uint8_t ticksIntoStep = 0;

        WalkerKind kind = WalkerKind::WaterCarrier;

        std::int32_t carried = 0;

        std::optional<Resource> carrying{};

        [[nodiscard]] bool operator==(
            const WalkerSprite &other) const = default;
    };

    struct BuildingView final
    {
        Cell at;
        BuildingKind kind = BuildingKind::House;

        std::array<std::int32_t, kServiceCount> coverage{};

        HousingLevel level = HousingLevel::Tent;

        std::int32_t population = 0;

        std::int32_t employed = 0;

        [[nodiscard]] bool operator==(
            const BuildingView &other) const = default;
    };

    struct BuildingSprite final
    {
        Cell at;
        BuildingKind kind = BuildingKind::House;

        std::array<std::int32_t, kResourceCount> stock{};

        std::array<std::int32_t, kServiceCount> coverage{};

        HousingLevel level = HousingLevel::Tent;

        std::int32_t population = 0;

        std::int32_t employed = 0;

        std::int32_t fireRisk = 0;

        std::int32_t collapseRisk = 0;

        std::int32_t diseaseRisk = 0;

        [[nodiscard]] bool operator==(
            const BuildingSprite &other) const = default;
    };

    struct RuinView final
    {
        Cell at;

        BuildingKind kind = BuildingKind::House;

        RuinState state = RuinState::Burning;

        [[nodiscard]] bool operator==(const RuinView &other) const
            = default;
    };

    struct HoverReadout final
    {
        Point anchor{};

        std::optional<BuildingSprite> building{};

        std::optional<WalkerSprite> walker{};

        std::optional<RuinView> ruin{};

        [[nodiscard]] bool operator==(
            const HoverReadout &other) const = default;
    };

    struct SceneSnapshot final
    {
        Camera camera;
        GridExtent extent;
        std::vector<Cell> paths;
        std::vector<WalkerSprite> walkers;
        std::vector<BuildingSprite> buildings;

        std::vector<RuinView> ruins;

        bool paused = false;

        RoadPlan plan;

        BuildGhost ghost;

        HoverReadout hover;
        MapView view = MapView::Normal;

        OverlayField overlay;

        [[nodiscard]] bool operator==(
            const SceneSnapshot &other) const = default;
    };

    [[nodiscard]] SceneSnapshot snapshotOf(
        const World &world,
        const PathIndex &paths,
        const Camera &camera,
        GridExtent extent,
        bool paused = false);

    [[nodiscard]] std::vector<WalkerView> walkerViewsOf(const World &world);

    [[nodiscard]] std::vector<BuildingView> buildingViewsOf(
        const World &world);

    [[nodiscard]] std::vector<RuinView> ruinViewsOf(const World &world);

}
