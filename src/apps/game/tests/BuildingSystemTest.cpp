#include "antwika/game/BuildingSystem.hpp"

#include <cstddef>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/Walker.hpp"

using antwika::ecs::Entity;
using antwika::ecs::World;
using antwika::game::Building;
using antwika::game::BuildingIndex;
using antwika::game::BuildingKind;
using antwika::game::BuildingSystem;
using antwika::game::Cell;
using antwika::game::kDrainPeriodTicks;
using antwika::game::kMaxRisk;
using antwika::game::kRiskPeriodTicks;
using antwika::game::kRiskRelief;
using antwika::game::kStockCapacity;
using antwika::game::kWalkerLoad;
using antwika::game::Resource;
using antwika::game::resourceIndex;
using antwika::game::Walker;
using antwika::game::WalkerKind;
using antwika::log::mocks::MockLogger;

namespace
{
    class BuildingSystemTest : public ::testing::Test
    {
    protected:
        Entity build(Cell at, Building building)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, at);
            world.add<Building>(entity, building);
            world.commit();
            built.insert(at);
            return entity;
        }

        Entity sendWalker(Cell at, Walker walker)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, at);
            world.add<Walker>(entity, walker);
            world.commit();
            return entity;
        }

        void run(std::size_t ticks)
        {
            for (std::size_t tick = 0; tick < ticks; ++tick)
            {
                system.update(world, tick);
                world.commit();
            }
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        BuildingIndex built;
        BuildingSystem system{built};
    };
} // namespace

TEST_F(BuildingSystemTest, Update_HandsAWalkersLoadToTheBuildingBesideIt)
{
    const auto house = build(
        Cell{.x = 1, .y = 0},
        Building{.kind = BuildingKind::House, .stock = {10, 10}});

    sendWalker(
        Cell{.x = 0, .y = 0},
        Walker{.kind = WalkerKind::Food, .carried = 40});

    run(1);

    EXPECT_EQ(
        world.get<Building>(house).stock[resourceIndex(Resource::Food)], 50);
}

TEST_F(BuildingSystemTest, Update_TakesWhatItGaveOffTheWalker)
{
    build(
        Cell{.x = 1, .y = 0},
        Building{.kind = BuildingKind::House, .stock = {10, 10}});

    const auto walker = sendWalker(
        Cell{.x = 0, .y = 0},
        Walker{.kind = WalkerKind::Food, .carried = 40});

    run(1);

    EXPECT_EQ(world.get<Walker>(walker).carried, 0);
}

TEST_F(BuildingSystemTest, Update_DeliversOnlyWhatTheBuildingHasRoomFor)
{
    const auto house = build(
        Cell{.x = 1, .y = 0},
        Building{
            .kind = BuildingKind::House,
            .stock = {kStockCapacity - 5, 10}});

    const auto walker = sendWalker(
        Cell{.x = 0, .y = 0},
        Walker{.kind = WalkerKind::Food, .carried = kWalkerLoad});

    run(1);

    EXPECT_EQ(
        world.get<Building>(house).stock[resourceIndex(Resource::Food)],
        kStockCapacity);
    EXPECT_EQ(world.get<Walker>(walker).carried, kWalkerLoad - 5);
}

// The double-buffer trap.
// Both walkers read the same committed amount.
// So a plain write would let the second overwrite the first.
TEST_F(BuildingSystemTest, Update_AddsUpTwoDeliveriesInOneTick)
{
    const auto house = build(
        Cell{.x = 1, .y = 1},
        Building{.kind = BuildingKind::House, .stock = {10, 10}});

    sendWalker(
        Cell{.x = 0, .y = 1},
        Walker{.kind = WalkerKind::Food, .carried = 10});
    sendWalker(
        Cell{.x = 2, .y = 1},
        Walker{.kind = WalkerKind::Food, .carried = 30});

    run(1);

    EXPECT_EQ(
        world.get<Building>(house).stock[resourceIndex(Resource::Food)], 50);
}

TEST_F(BuildingSystemTest, Update_DeliversWaterToTheWaterShelf)
{
    const auto house = build(
        Cell{.x = 1, .y = 0},
        Building{.kind = BuildingKind::House, .stock = {10, 10}});

    sendWalker(
        Cell{.x = 0, .y = 0},
        Walker{.kind = WalkerKind::Water, .carried = 20});

    run(1);

    const auto stock = world.get<Building>(house).stock;

    EXPECT_EQ(stock[resourceIndex(Resource::Water)], 30);
    EXPECT_EQ(stock[resourceIndex(Resource::Food)], 10);
}

TEST_F(BuildingSystemTest, Update_LetsAFiremanTakeRiskOffInstead)
{
    const auto house = build(
        Cell{.x = 1, .y = 0},
        Building{.kind = BuildingKind::House, .risk = 60});

    const auto fireman = sendWalker(
        Cell{.x = 0, .y = 0}, Walker{.kind = WalkerKind::Fireman});

    run(1);

    EXPECT_EQ(world.get<Building>(house).risk, 60 - kRiskRelief);
    EXPECT_EQ(world.get<Walker>(fireman).carried, 0);
}

TEST_F(BuildingSystemTest, Update_NeverTakesRiskBelowNothing)
{
    const auto house = build(
        Cell{.x = 1, .y = 0},
        Building{.kind = BuildingKind::House, .risk = 1});

    sendWalker(Cell{.x = 0, .y = 0}, Walker{.kind = WalkerKind::Architect});

    run(1);

    EXPECT_EQ(world.get<Building>(house).risk, 0);
}

TEST_F(BuildingSystemTest, Update_LeavesABuildingAWalkerIsNotBesideAlone)
{
    const auto house = build(
        Cell{.x = 5, .y = 5},
        Building{.kind = BuildingKind::House, .stock = {10, 10}});

    sendWalker(
        Cell{.x = 0, .y = 0},
        Walker{.kind = WalkerKind::Food, .carried = 40});

    run(1);

    EXPECT_EQ(
        world.get<Building>(house).stock[resourceIndex(Resource::Food)], 10);
}

TEST_F(BuildingSystemTest, Update_DrainsAHouseOnItsOwnPeriod)
{
    const auto house = build(
        Cell{.x = 0, .y = 0},
        Building{.kind = BuildingKind::House, .stock = {50, 50}});

    run(static_cast<std::size_t>(kDrainPeriodTicks) + 1);

    const auto stock = world.get<Building>(house).stock;

    EXPECT_EQ(stock[resourceIndex(Resource::Food)], 49);
    EXPECT_EQ(stock[resourceIndex(Resource::Water)], 49);
}

TEST_F(BuildingSystemTest, Update_LeavesASourcesStockWhereItIs)
{
    // A source is not a place anybody eats.
    const auto well = build(
        Cell{.x = 0, .y = 0},
        Building{.kind = BuildingKind::WaterSource, .stock = {50, 50}});

    run(static_cast<std::size_t>(kDrainPeriodTicks) + 1);

    EXPECT_EQ(
        world.get<Building>(well).stock[resourceIndex(Resource::Food)], 50);
}

TEST_F(BuildingSystemTest, Update_RaisesRiskOnItsOwnPeriod)
{
    const auto house = build(
        Cell{.x = 0, .y = 0}, Building{.kind = BuildingKind::House});

    run(static_cast<std::size_t>(kRiskPeriodTicks) + 1);

    EXPECT_EQ(world.get<Building>(house).risk, 1);
}

TEST_F(BuildingSystemTest, Update_DemolishesABuildingThatRanOutOfLuck)
{
    const auto house = build(
        Cell{.x = 0, .y = 0},
        Building{.kind = BuildingKind::House, .risk = kMaxRisk - 1});

    run(static_cast<std::size_t>(kRiskPeriodTicks) + 1);

    EXPECT_FALSE(world.alive(house));
}

TEST_F(BuildingSystemTest, Update_DemolishesAHouseThatRanOutOfFood)
{
    const auto house = build(
        Cell{.x = 0, .y = 0},
        Building{.kind = BuildingKind::House, .stock = {1, 50}});

    run(static_cast<std::size_t>(kDrainPeriodTicks) + 1);

    EXPECT_FALSE(world.alive(house));
}

// The cell has to become buildable again.
// Otherwise a burned-down house holds its ground for ever.
TEST_F(BuildingSystemTest, Update_ClearsTheCellOfWhatItDemolishes)
{
    build(
        Cell{.x = 4, .y = 4},
        Building{.kind = BuildingKind::House, .risk = kMaxRisk});

    ASSERT_TRUE(built.has(Cell{.x = 4, .y = 4}));

    run(1);

    EXPECT_FALSE(built.has(Cell{.x = 4, .y = 4}));
}

TEST_F(BuildingSystemTest, Update_LeavesTheWalkerOfADemolishedBuilding)
{
    // The walker outlives its home.
    // WalkerSystem removes it once its own budget runs out.
    const auto walker = sendWalker(
        Cell{.x = 0, .y = 0},
        Walker{.kind = WalkerKind::Food, .carried = kWalkerLoad});

    build(
        Cell{.x = 5, .y = 5},
        Building{.kind = BuildingKind::House, .risk = kMaxRisk});

    run(1);

    EXPECT_TRUE(world.alive(walker));
}

TEST_F(BuildingSystemTest, Update_DoesNothingWithNoBuildingsAtAll)
{
    EXPECT_NO_THROW(run(1));
}
