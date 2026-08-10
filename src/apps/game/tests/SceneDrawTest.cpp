#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "SceneDrawer.hpp"
#include "Translators.hpp"
#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/BuildingSystem.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Coverage.hpp"
#include "antwika/game/CoverageSystem.hpp"
#include "antwika/game/Desirability.hpp"
#include "antwika/game/DesirabilitySystem.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/GameConfig.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/HaulingSystem.hpp"
#include "antwika/game/HousingSystem.hpp"
#include "antwika/game/MapView.hpp"
#include "antwika/game/OverlayField.hpp"
#include "antwika/game/Path.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/ProductionSystem.hpp"
#include "antwika/game/SceneSnapshot.hpp"
#include "antwika/game/SpawnSystem.hpp"
#include "antwika/game/SupplySystem.hpp"
#include "antwika/game/WalkerSystem.hpp"

namespace
{
    using antwika::ecs::SystemScheduler;
    using antwika::ecs::World;
    using antwika::game::Building;
    using antwika::game::BuildingIndex;
    using antwika::game::BuildingKind;
    using antwika::game::BuildingSystem;
    using antwika::game::Camera;
    using antwika::game::Cell;
    using antwika::game::CoverageSystem;
    using antwika::game::DesirabilityField;
    using antwika::game::DesirabilitySystem;
    using antwika::game::footprintOf;
    using antwika::game::GameConfig;
    using antwika::game::GridExtent;
    using antwika::game::HaulingSystem;
    using antwika::game::HousingSystem;
    using antwika::game::MapView;
    using antwika::game::overlayFieldOf;
    using antwika::game::Path;
    using antwika::game::PathIndex;
    using antwika::game::Point;
    using antwika::game::ProductionSystem;
    using antwika::game::SceneSnapshot;
    using antwika::game::snapshotOf;
    using antwika::game::SpawnSystem;
    using antwika::game::SupplySystem;
    using antwika::game::WalkerSystem;
    using antwika::gfx::Bitmap;
    using antwika::gfx::Size;
    using antwika::gfx::mocks::MockRenderer;
    using antwika::gfx::mocks::MockTexture;
    using antwika::game::tests::kTranslator;
    using antwika::log::mocks::MockLogger;
    using ::testing::_;
    using ::testing::AtLeast;
    using ::testing::NiceMock;

    constexpr Size kCanvas{.width = 960, .height = 600};

    constexpr GridExtent kExtent{.width = 12, .height = 12};

    constexpr std::size_t kTicks = 400;

    class SceneDrawTest : public ::testing::Test
    {
    protected:
        SceneDrawTest()
        {
            const auto walk = scheduler.createPhase("walk");
            scheduler.addSystem(walk, walkers);
            scheduler.addSystem(walk, buildings);
            scheduler.addSystem(walk, spawns);

            const auto serve = scheduler.createPhase("serve");
            scheduler.addSystem(serve, coverage);
            scheduler.addSystem(serve, desirability);

            const auto make = scheduler.createPhase("make");
            scheduler.addSystem(make, production);
            scheduler.addSystem(make, hauling);
            scheduler.addSystem(make, supplies);

            const auto settle = scheduler.createPhase("settle");
            scheduler.addSystem(settle, housing);
        }

        void pave(Cell cell)
        {
            paths.insert(cell);

            const auto entity = world.create();
            world.add<Cell>(entity, cell);
            world.add<Path>(entity, Path{});
        }

        void build(Cell at, BuildingKind kind)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, at);
            world.add<Building>(entity, Building{.kind = kind});
            world.commit();
            built.insert(at, footprintOf(kind));
        }

        [[nodiscard]] SceneSnapshot city(MapView view)
        {
            auto snapshot = snapshotOf(world, paths, camera, kExtent);
            snapshot.view = view;
            snapshot.overlay =
                overlayFieldOf(world, view, field, kExtent);

            return snapshot;
        }

        void run()
        {
            for (std::int32_t x = 0; x < kExtent.width; ++x)
            {
                pave(Cell{.x = x, .y = 5});
            }

            world.commit();

            build(Cell{.x = 0, .y = 3}, BuildingKind::ClayPit);
            build(Cell{.x = 3, .y = 2}, BuildingKind::Storage);
            build(Cell{.x = 6, .y = 3}, BuildingKind::Workshop);
            build(Cell{.x = 8, .y = 3}, BuildingKind::Market);
            build(Cell{.x = 2, .y = 6}, BuildingKind::Well);
            build(Cell{.x = 3, .y = 6}, BuildingKind::Doctor);
            build(Cell{.x = 4, .y = 6}, BuildingKind::House);
            build(Cell{.x = 5, .y = 6}, BuildingKind::House);
            build(Cell{.x = 10, .y = 4}, BuildingKind::House);

            for (std::size_t tick = 0; tick < kTicks; ++tick)
            {
                scheduler.run(world, tick);
            }
        }

        [[nodiscard]] static std::unique_ptr<NiceMock<MockRenderer>>
            stubbedRenderer()
        {
            auto renderer = std::make_unique<NiceMock<MockRenderer>>();

            ON_CALL(*renderer, createTexture(_))
                .WillByDefault(
                    [](const Bitmap &)
                    { return std::make_unique<NiceMock<MockTexture>>(); });

            return renderer;
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        PathIndex paths;
        BuildingIndex built;
        DesirabilityField field;
        Camera camera{Point{.x = 470, .y = 70}, 3};
        SystemScheduler scheduler;
        WalkerSystem walkers{paths, built, kExtent};
        BuildingSystem buildings{built, kExtent, GameConfig{}};
        SpawnSystem spawns{paths, GameConfig{}};
        CoverageSystem coverage{};
        DesirabilitySystem desirability{field, kExtent};
        ProductionSystem production{GameConfig{}};
        HaulingSystem hauling{paths, kExtent};
        SupplySystem supplies{paths, kExtent, GameConfig{}};
        HousingSystem housing{field, GameConfig{}};
    };
}

TEST_F(SceneDrawTest, DrawScene_DrawsTheCityAsItStands)
{
    run();

    const auto renderer = stubbedRenderer();

    EXPECT_CALL(*renderer, drawTexture(_, _, _, _)).Times(AtLeast(1));

    antwika::game::preview::drawScene(
        *renderer, city(MapView::Normal), kCanvas);
}

TEST_F(SceneDrawTest, DrawScene_DrawsEveryHeatMapWithItsNumbers)
{
    run();

    for (std::size_t index = 1;
         index < antwika::game::kMapViewCount;
         ++index)
    {
        const auto view = static_cast<MapView>(index);
        const auto snapshot = city(view);

        ASSERT_FALSE(snapshot.overlay.empty()) << index;

        const auto renderer = stubbedRenderer();

        EXPECT_CALL(*renderer, drawText(_, _, _, _)).Times(AtLeast(1));

        antwika::game::preview::drawScene(*renderer, snapshot, kCanvas);
    }
}
