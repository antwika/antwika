#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/BuildingSystem.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Walker.hpp"

using antwika::ecs::Entity;
using antwika::ecs::World;
using antwika::game::Building;
using antwika::game::BuildingIndex;
using antwika::game::BuildingKind;
using antwika::game::BuildingSystem;
using antwika::game::Cell;
using antwika::game::Direction;
using antwika::game::kDrainPeriodTicks;
using antwika::game::kMaxRisk;
using antwika::game::kRiskPeriodTicks;
using antwika::game::kSpawnPeriodTicks;
using antwika::game::kWalkerCarryCapacity;
using antwika::game::newlyBuilt;
using antwika::game::PathIndex;
using antwika::game::Walker;
using antwika::game::walkerFor;
using antwika::game::WalkerKind;
using antwika::log::mocks::MockLogger;

namespace
{
    constexpr Cell kAt{.x = 5, .y = 5};

    class BuildingSystemTest : public ::testing::Test
    {
    protected:
        Entity put(Cell cell, const Building &building)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, cell);
            world.add<Building>(entity, building);
            world.commit();
            buildings.insert(cell);

            return entity;
        }

        void run(std::int32_t ticks)
        {
            for (std::int32_t tick = 0; tick < ticks; ++tick)
            {
                system.update(
                    world, static_cast<antwika::time::Tick>(tick));
                world.commit();
            }
        }

        [[nodiscard]] std::size_t buildingCount() const
        {
            return world.view<Building, Cell>().size();
        }

        [[nodiscard]] std::size_t walkerCount() const
        {
            return world.view<Walker, Cell>().size();
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        PathIndex paths;
        BuildingIndex buildings;
        BuildingSystem system{paths, buildings};
    };
} // namespace

TEST(WalkerForTest, EverySourceSendsOutItsOwnKindOfWalker)
{
    EXPECT_EQ(walkerFor(BuildingKind::FoodSource), WalkerKind::Food);
    EXPECT_EQ(walkerFor(BuildingKind::WaterSource), WalkerKind::Water);
    EXPECT_EQ(walkerFor(BuildingKind::FireStation), WalkerKind::Fireman);
    EXPECT_EQ(
        walkerFor(BuildingKind::ArchitectPost), WalkerKind::Architect);
}

TEST(WalkerForTest, AHouseSendsOutNothing)
{
    EXPECT_FALSE(walkerFor(BuildingKind::House).has_value());
}

TEST_F(BuildingSystemTest, Update_DrainsOneUnitEveryDrainPeriod)
{
    const auto entity = put(kAt, newlyBuilt(BuildingKind::House));

    run(kDrainPeriodTicks - 1);
    EXPECT_EQ(world.get<Building>(entity).stock.held, 10);

    run(1);
    EXPECT_EQ(world.get<Building>(entity).stock.held, 9);
}

TEST_F(BuildingSystemTest, Update_RaisesBothRisksEveryRiskPeriod)
{
    const auto entity = put(kAt, newlyBuilt(BuildingKind::House));

    run(kRiskPeriodTicks);

    EXPECT_EQ(world.get<Building>(entity).fireRisk, 1);
    EXPECT_EQ(world.get<Building>(entity).collapseRisk, 1);
}

TEST_F(BuildingSystemTest, Update_LeavesAHealthyBuildingStanding)
{
    put(kAt, newlyBuilt(BuildingKind::House));

    run(1);

    EXPECT_EQ(buildingCount(), 1U);
    EXPECT_TRUE(buildings.has(kAt));
}

// Out of stock is the end of it, and the ground is free again.
TEST_F(BuildingSystemTest, Update_RemovesABuildingThatRunsOut)
{
    auto starving = newlyBuilt(BuildingKind::House);
    starving.stock.held = 1;
    starving.drainIn = 1;
    put(kAt, starving);

    run(1);

    EXPECT_EQ(buildingCount(), 0U);
    EXPECT_FALSE(buildings.has(kAt));
}

TEST_F(BuildingSystemTest, Update_BurnsDownABuildingAtMaximumFireRisk)
{
    auto doomed = newlyBuilt(BuildingKind::House);
    doomed.fireRisk = kMaxRisk - 1;
    doomed.riskIn = 1;
    put(kAt, doomed);

    run(1);

    EXPECT_EQ(buildingCount(), 0U);
    EXPECT_FALSE(buildings.has(kAt));
}

TEST_F(BuildingSystemTest, Update_CollapsesABuildingAtMaximumCollapseRisk)
{
    auto doomed = newlyBuilt(BuildingKind::House);
    doomed.collapseRisk = kMaxRisk - 1;
    doomed.riskIn = 1;
    put(kAt, doomed);

    run(1);

    EXPECT_EQ(buildingCount(), 0U);
    EXPECT_FALSE(buildings.has(kAt));
}

TEST_F(BuildingSystemTest, Update_SpawnsAWalkerOntoAnAdjacentRoad)
{
    paths.insert(Cell{.x = kAt.x + 1, .y = kAt.y});
    put(kAt, newlyBuilt(BuildingKind::FoodSource));

    run(kSpawnPeriodTicks - 1);
    EXPECT_EQ(walkerCount(), 0U);

    run(1);
    ASSERT_EQ(walkerCount(), 1U);

    const auto entity = *world.view<Walker, Cell>().begin();
    EXPECT_EQ(
        world.get<Cell>(entity), (Cell{.x = kAt.x + 1, .y = kAt.y}));
    EXPECT_EQ(world.get<Walker>(entity).kind, WalkerKind::Food);
    EXPECT_EQ(world.get<Walker>(entity).facing, Direction::East);
    EXPECT_EQ(world.get<Walker>(entity).carried, kWalkerCarryCapacity);
}

// North first, so which road a walker steps onto is never a toss-up.
TEST_F(BuildingSystemTest, Update_PrefersTheRoadToTheNorth)
{
    paths.insert(Cell{.x = kAt.x, .y = kAt.y - 1});
    paths.insert(Cell{.x = kAt.x, .y = kAt.y + 1});
    put(kAt, newlyBuilt(BuildingKind::WaterSource));

    run(kSpawnPeriodTicks);

    ASSERT_EQ(walkerCount(), 1U);
    const auto entity = *world.view<Walker, Cell>().begin();
    EXPECT_EQ(
        world.get<Cell>(entity), (Cell{.x = kAt.x, .y = kAt.y - 1}));
    EXPECT_EQ(world.get<Walker>(entity).facing, Direction::North);
}

// No road yet, so it tries again next tick rather than in a minute.
TEST_F(BuildingSystemTest, Update_RetriesTheSpawnOnceARoadArrives)
{
    put(kAt, newlyBuilt(BuildingKind::FireStation));

    run(kSpawnPeriodTicks);
    EXPECT_EQ(walkerCount(), 0U);

    paths.insert(Cell{.x = kAt.x - 1, .y = kAt.y});
    run(1);

    ASSERT_EQ(walkerCount(), 1U);
    const auto entity = *world.view<Walker, Cell>().begin();
    EXPECT_EQ(world.get<Walker>(entity).kind, WalkerKind::Fireman);
    EXPECT_EQ(world.get<Walker>(entity).facing, Direction::West);
    EXPECT_EQ(world.get<Walker>(entity).carried, 0);
}

TEST_F(BuildingSystemTest, Update_SpawnsNothingFromAHouse)
{
    paths.insert(Cell{.x = kAt.x + 1, .y = kAt.y});
    put(kAt, newlyBuilt(BuildingKind::House));

    run(kSpawnPeriodTicks);

    EXPECT_EQ(walkerCount(), 0U);
}

// An architect post is the fourth spawner, and the one to the south.
TEST_F(BuildingSystemTest, Update_SpawnsAnArchitectSouthwards)
{
    paths.insert(Cell{.x = kAt.x, .y = kAt.y + 1});
    put(kAt, newlyBuilt(BuildingKind::ArchitectPost));

    run(kSpawnPeriodTicks);

    ASSERT_EQ(walkerCount(), 1U);
    const auto entity = *world.view<Walker, Cell>().begin();
    EXPECT_EQ(world.get<Walker>(entity).kind, WalkerKind::Architect);
    EXPECT_EQ(world.get<Walker>(entity).facing, Direction::South);
}
