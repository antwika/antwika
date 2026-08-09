#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/BuildingSystem.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/HaulingSystem.hpp"
#include "antwika/game/Path.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/ProductionSystem.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/SpawnSystem.hpp"
#include "antwika/game/SupplySystem.hpp"
#include "antwika/game/Walker.hpp"
#include "antwika/game/WalkerSystem.hpp"

namespace
{
    using antwika::ecs::Entity;
    using antwika::ecs::SystemScheduler;
    using antwika::ecs::World;
    using antwika::game::Building;
    using antwika::game::BuildingIndex;
    using antwika::game::BuildingKind;
    using antwika::game::BuildingSystem;
    using antwika::game::Cell;
    using antwika::game::footprintOf;
    using antwika::game::GridExtent;
    using antwika::game::HaulingSystem;
    using antwika::game::kStockOnCompletion;
    using antwika::game::SupplySystem;
    using antwika::game::Path;
    using antwika::game::PathIndex;
    using antwika::game::ProductionSystem;
    using antwika::game::Resource;
    using antwika::game::resourceIndex;
    using antwika::game::SpawnSystem;
    using antwika::game::Walker;
    using antwika::game::WalkerSystem;
    using antwika::log::mocks::MockLogger;

    constexpr GridExtent kExtent{.width = 12, .height = 12};

    constexpr std::size_t kTicks = 600;

    constexpr std::size_t kPatientTicks = 1500;

    class SupplyChainTest : public ::testing::Test
    {
    protected:
        SupplyChainTest()
        {
            const auto walk = scheduler.createPhase("walk");
            scheduler.addSystem(walk, walkers);
            scheduler.addSystem(walk, buildings);
            scheduler.addSystem(walk, spawns);

            const auto produce = scheduler.createPhase("produce");
            scheduler.addSystem(produce, production);

            const auto haul = scheduler.createPhase("haul");
            scheduler.addSystem(haul, hauling);

            const auto supply = scheduler.createPhase("supply");
            scheduler.addSystem(supply, supplies);
        }

        Entity build(Cell at, BuildingKind kind, std::int32_t food)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, at);
            world.add<Building>(
                entity, Building{.kind = kind, .stock = {food, 0, 0}});
            world.commit();
            built.insert(at, footprintOf(kind));
            return entity;
        }

        [[nodiscard]] std::int32_t held(Entity entity, Resource resource)
        {
            return world.get<Building>(entity)
                .stock[resourceIndex(resource)];
        }

        void paveRow(std::int32_t y, std::int32_t from, std::int32_t to)
        {
            for (std::int32_t x = from; x <= to; ++x)
            {
                const Cell cell{.x = x, .y = y};
                paths.insert(cell);

                const auto entity = world.create();
                world.add<Cell>(entity, cell);
                world.add<Path>(entity, Path{});
            }

            world.commit();
        }

        [[nodiscard]] std::int32_t food(Entity entity)
        {
            return world.get<Building>(entity)
                .stock[resourceIndex(Resource::Food)];
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        PathIndex paths;
        BuildingIndex built;
        SystemScheduler scheduler;
        WalkerSystem walkers{paths, built, kExtent};
        BuildingSystem buildings{built, kExtent, antwika::game::GameConfig{}};
        SpawnSystem spawns{paths, antwika::game::GameConfig{}};
        ProductionSystem production{antwika::game::GameConfig{}};
        HaulingSystem hauling{paths, kExtent};
        SupplySystem supplies{paths, kExtent, antwika::game::GameConfig{}};
    };
}

TEST_F(SupplyChainTest, Run_FeedsAHouseThroughTheChain)
{
    paveRow(5, 0, 11);

    const auto farm = build(Cell{.x = 0, .y = 3}, BuildingKind::Farm, 0);
    const auto store =
        build(Cell{.x = 3, .y = 2}, BuildingKind::Storage, 0);
    const auto market =
        build(Cell{.x = 7, .y = 3}, BuildingKind::Market, 0);
    const auto house = build(
        Cell{.x = 10, .y = 4},
        BuildingKind::House,
        kStockOnCompletion);

    std::int32_t stored = 0;
    std::int32_t sold = 0;
    std::int32_t eaten = 0;

    for (std::size_t tick = 0; tick < kTicks; ++tick)
    {
        scheduler.run(world, tick);

        stored = std::max(stored, food(store));
        sold = std::max(sold, food(market));
        eaten = std::max(eaten, food(house));
    }

    EXPECT_TRUE(world.alive(farm));
    EXPECT_TRUE(world.alive(house));

    EXPECT_GT(stored, 0);
    EXPECT_GT(sold, 0);
    EXPECT_GT(eaten, kStockOnCompletion);
}

TEST_F(SupplyChainTest, Run_FeedsWithAFarmAlone)
{
    paveRow(5, 0, 11);

    build(Cell{.x = 0, .y = 3}, BuildingKind::Farm, 0);
    const auto house = build(
        Cell{.x = 4, .y = 4}, BuildingKind::House, kStockOnCompletion);

    std::int32_t eaten = 0;

    for (std::size_t tick = 0; tick < kTicks; ++tick)
    {
        scheduler.run(world, tick);
        eaten = std::max(eaten, food(house));
    }

    EXPECT_TRUE(world.alive(house));
    EXPECT_GT(eaten, kStockOnCompletion);
}

TEST_F(SupplyChainTest, Run_TurnsClayIntoPotteryInTheStore)
{
    paveRow(5, 0, 11);

    const auto pit =
        build(Cell{.x = 0, .y = 3}, BuildingKind::ClayPit, 0);
    const auto store =
        build(Cell{.x = 3, .y = 2}, BuildingKind::Storage, 0);
    const auto workshop =
        build(Cell{.x = 7, .y = 3}, BuildingKind::Workshop, 0);

    std::int32_t stored = 0;
    std::int32_t fetched = 0;
    std::int32_t fired = 0;
    std::int32_t shipped = 0;

    for (std::size_t tick = 0; tick < kTicks; ++tick)
    {
        scheduler.run(world, tick);

        stored = std::max(stored, held(store, Resource::Clay));
        fetched = std::max(fetched, held(workshop, Resource::Clay));
        fired = std::max(fired, held(workshop, Resource::Pottery));
        shipped = std::max(shipped, held(store, Resource::Pottery));
    }

    EXPECT_TRUE(world.alive(pit));
    EXPECT_TRUE(world.alive(workshop));

    EXPECT_GT(stored, 0);
    EXPECT_GT(fetched, 0);
    EXPECT_GT(fired, 0);
    EXPECT_GT(shipped, 0);
}

TEST_F(SupplyChainTest, Run_CarriesPotteryFromTheClayPitToAHouse)
{
    paveRow(5, 0, 11);

    build(Cell{.x = 0, .y = 3}, BuildingKind::ClayPit, 0);
    build(Cell{.x = 3, .y = 2}, BuildingKind::Storage, 0);
    build(Cell{.x = 6, .y = 3}, BuildingKind::Workshop, 0);
    build(Cell{.x = 8, .y = 3}, BuildingKind::Market, 0);
    const auto house = build(
        Cell{.x = 10, .y = 4},
        BuildingKind::House,
        kStockOnCompletion);

    std::int32_t shelved = 0;

    for (std::size_t tick = 0; tick < kPatientTicks; ++tick)
    {
        scheduler.run(world, tick);

        shelved = std::max(shelved, held(house, Resource::Pottery));
    }

    EXPECT_GT(shelved, 0);
}

TEST_F(SupplyChainTest, Run_KeepsFeedingTheHouseThatAlsoTakesPottery)
{
    paveRow(5, 0, 11);

    build(Cell{.x = 0, .y = 3}, BuildingKind::ClayPit, 0);
    build(Cell{.x = 1, .y = 6}, BuildingKind::Farm, 0);
    build(Cell{.x = 3, .y = 2}, BuildingKind::Storage, 0);
    build(Cell{.x = 6, .y = 3}, BuildingKind::Workshop, 0);
    build(Cell{.x = 8, .y = 3}, BuildingKind::Market, 0);
    const auto house = build(
        Cell{.x = 10, .y = 4},
        BuildingKind::House,
        kStockOnCompletion);

    std::int32_t fed = 0;
    std::int32_t shelved = 0;

    for (std::size_t tick = 0; tick < kPatientTicks; ++tick)
    {
        scheduler.run(world, tick);

        fed = std::max(fed, held(house, Resource::Food));
        shelved = std::max(shelved, held(house, Resource::Pottery));
    }

    EXPECT_GT(fed, kStockOnCompletion);
    EXPECT_GT(shelved, 0);
}

TEST_F(SupplyChainTest, Run_FiresNothingWithNoClay)
{
    paveRow(5, 0, 11);

    const auto workshop =
        build(Cell{.x = 7, .y = 3}, BuildingKind::Workshop, 0);

    for (std::size_t tick = 0; tick < kTicks; ++tick)
    {
        scheduler.run(world, tick);
    }

    EXPECT_EQ(held(workshop, Resource::Pottery), 0);
}
