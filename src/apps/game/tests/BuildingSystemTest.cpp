#include "antwika/game/BuildingSystem.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Coverage.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/Service.hpp"
#include "antwika/game/Walker.hpp"

using antwika::ecs::Entity;
using antwika::ecs::World;
using antwika::game::Building;
using antwika::game::BuildingIndex;
using antwika::game::BuildingKind;
using antwika::game::BuildingSystem;
using antwika::game::Cell;
using antwika::game::Footprint;
using antwika::game::footprintOf;
using antwika::game::Coverage;
using antwika::game::kCoverageFull;
using antwika::game::kDrainPeriodTicks;
using antwika::game::kMaxRisk;
using antwika::game::kRiskPeriodTicks;
using antwika::game::kServiceCount;
using antwika::game::kStockCapacity;
using antwika::game::setCoverage;
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
            built.insert(at, footprintOf(building.kind));
            return entity;
        }

        void cover(
            Entity entity,
            std::array<std::int32_t, kServiceCount> ticksLeft)
        {
            setCoverage(world, entity, Coverage{.ticksLeft = ticksLeft});
            world.commit();
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
        Walker{.kind = WalkerKind::MarketSeller, .carried = 40});

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
        Walker{.kind = WalkerKind::MarketSeller, .carried = 40});

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
        Walker{.kind = WalkerKind::MarketSeller, .carried = kWalkerLoad});

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
        Walker{.kind = WalkerKind::MarketSeller, .carried = 10});
    sendWalker(
        Cell{.x = 2, .y = 1},
        Walker{.kind = WalkerKind::MarketSeller, .carried = 30});

    run(1);

    EXPECT_EQ(
        world.get<Building>(house).stock[resourceIndex(Resource::Food)], 50);
}

// A shelf already at capacity has no room for anything.
// So nothing changes hands and the walker keeps its load.
TEST_F(BuildingSystemTest, Update_HandsOverNothingToAShelfAlreadyFull)
{
    const auto house = build(
        Cell{.x = 1, .y = 0},
        Building{
            .kind = BuildingKind::House,
            .stock = {kStockCapacity, 0, 0}});

    const auto seller = sendWalker(
        Cell{.x = 0, .y = 0},
        Walker{.kind = WalkerKind::MarketSeller, .carried = 40});

    run(1);

    EXPECT_EQ(
        world.get<Building>(house).stock[resourceIndex(Resource::Food)],
        kStockCapacity);
    EXPECT_EQ(world.get<Walker>(seller).carried, 40);
}

// A water carrier hands nothing over: water is a service now.
// So what it leaves behind is a shelf exactly where it was.
TEST_F(BuildingSystemTest, Update_LeavesEveryShelfAloneForAServiceWalker)
{
    const auto house = build(
        Cell{.x = 1, .y = 0},
        Building{.kind = BuildingKind::House, .stock = {10, 20, 30}});

    sendWalker(
        Cell{.x = 0, .y = 0},
        Walker{.kind = WalkerKind::WaterCarrier, .carried = 20});

    run(1);

    const auto stock = world.get<Building>(house).stock;

    EXPECT_EQ(stock[resourceIndex(Resource::Food)], 10);
    EXPECT_EQ(stock[resourceIndex(Resource::Clay)], 20);
    EXPECT_EQ(stock[resourceIndex(Resource::Pottery)], 30);
}

// A fireman used to take a fixed amount of risk off here.
// He now refreshes coverage instead -- see CoverageSystem.
// So a delivery has nothing whatever to do with one.
TEST_F(BuildingSystemTest, Update_LeavesRiskAloneForAPassingFireman)
{
    const auto house = build(
        Cell{.x = 1, .y = 0},
        Building{.kind = BuildingKind::House, .risk = 60});

    const auto fireman = sendWalker(
        Cell{.x = 0, .y = 0}, Walker{.kind = WalkerKind::Fireman});

    run(1);

    EXPECT_EQ(world.get<Building>(house).risk, 60);
    EXPECT_EQ(world.get<Walker>(fireman).carried, 0);
}

TEST_F(BuildingSystemTest, Update_LeavesABuildingAWalkerIsNotBesideAlone)
{
    const auto house = build(
        Cell{.x = 5, .y = 5},
        Building{.kind = BuildingKind::House, .stock = {10, 10}});

    sendWalker(
        Cell{.x = 0, .y = 0},
        Walker{.kind = WalkerKind::MarketSeller, .carried = 40});

    run(1);

    EXPECT_EQ(
        world.get<Building>(house).stock[resourceIndex(Resource::Food)], 10);
}

TEST_F(BuildingSystemTest, Update_DrainsAHouseOnItsOwnPeriod)
{
    const auto house = build(
        Cell{.x = 0, .y = 0},
        Building{.kind = BuildingKind::House, .stock = {50, 50, 50}});

    run(static_cast<std::size_t>(kDrainPeriodTicks) + 1);

    const auto stock = world.get<Building>(house).stock;

    EXPECT_EQ(stock[resourceIndex(Resource::Food)], 49);
    EXPECT_EQ(stock[resourceIndex(Resource::Clay)], 49);
    EXPECT_EQ(stock[resourceIndex(Resource::Pottery)], 49);
}

// Only what sustains() names is a larder.
// A house with no pottery is a house nobody has sold any to yet.
TEST_F(BuildingSystemTest, Update_KeepsAHouseThatHasRunOutOfAComfort)
{
    const auto house = build(
        Cell{.x = 0, .y = 0},
        Building{.kind = BuildingKind::House, .stock = {50, 0, 0}});

    run(1);

    EXPECT_TRUE(world.alive(house));
}

TEST_F(BuildingSystemTest, Update_LeavesASourcesStockWhereItIs)
{
    // A source is not a place anybody eats.
    const auto well = build(
        Cell{.x = 0, .y = 0},
        Building{.kind = BuildingKind::Well, .stock = {50, 50}});

    run(static_cast<std::size_t>(kDrainPeriodTicks) + 1);

    EXPECT_EQ(
        world.get<Building>(well).stock[resourceIndex(Resource::Food)], 50);
}

// A district nobody serves is a district that falls down.
TEST_F(BuildingSystemTest, Update_RaisesRiskWithNoSafetyCoverageAtAll)
{
    const auto house = build(
        Cell{.x = 0, .y = 0}, Building{.kind = BuildingKind::House});

    run(static_cast<std::size_t>(kRiskPeriodTicks) + 1);

    EXPECT_EQ(world.get<Building>(house).risk, 1);
}

// And one that both a fireman and an engineer reach works it back off.
TEST_F(BuildingSystemTest, Update_TakesRiskBackOffWhereBothServicesReach)
{
    const auto house = build(
        Cell{.x = 0, .y = 0},
        Building{.kind = BuildingKind::House, .risk = 40});
    cover(house, {0, 0, kCoverageFull, kCoverageFull});

    run(static_cast<std::size_t>(kRiskPeriodTicks) + 1);

    EXPECT_EQ(world.get<Building>(house).risk, 39);
}

// Safety alone is not enough: a building has to stay standing too.
TEST_F(BuildingSystemTest, Update_StillRaisesRiskWithOnlyOneOfTheTwo)
{
    const auto house = build(
        Cell{.x = 0, .y = 0}, Building{.kind = BuildingKind::House});
    cover(house, {0, 0, kCoverageFull, 0});

    run(static_cast<std::size_t>(kRiskPeriodTicks) + 1);

    EXPECT_EQ(world.get<Building>(house).risk, 1);
}

TEST_F(BuildingSystemTest, Update_NeverTakesRiskBelowNothing)
{
    const auto house = build(
        Cell{.x = 0, .y = 0}, Building{.kind = BuildingKind::House});
    cover(house, {0, 0, kCoverageFull, kCoverageFull});

    run(3 * static_cast<std::size_t>(kRiskPeriodTicks));

    EXPECT_EQ(world.get<Building>(house).risk, 0);
}

TEST_F(BuildingSystemTest, Update_NeverTakesRiskAboveTheMost)
{
    // A source, so an empty larder is not what ends it.
    const auto well = build(
        Cell{.x = 0, .y = 0},
        Building{.kind = BuildingKind::Well, .risk = kMaxRisk - 1});

    run(2 * static_cast<std::size_t>(kRiskPeriodTicks));

    EXPECT_FALSE(world.alive(well));
}

// A countdown that has not run out is a countdown and nothing else.
TEST_F(BuildingSystemTest, Update_LeavesRiskAloneBeforeItsPeriodIsUp)
{
    const auto house = build(
        Cell{.x = 0, .y = 0},
        Building{.kind = BuildingKind::House, .risk = 5});

    run(static_cast<std::size_t>(kRiskPeriodTicks) - 1);

    EXPECT_EQ(world.get<Building>(house).risk, 5);
    EXPECT_EQ(world.get<Building>(house).ticksUntilRisk, 1);
}

// What is still this system's is what a building at the most is for.
TEST_F(BuildingSystemTest, Update_DemolishesABuildingThatRanOutOfLuck)
{
    const auto house = build(
        Cell{.x = 0, .y = 0},
        Building{.kind = BuildingKind::House, .risk = kMaxRisk});

    run(1);

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
        Walker{.kind = WalkerKind::MarketSeller, .carried = kWalkerLoad});

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

TEST_F(BuildingSystemTest, Update_ReachesABuildingByAnyCellOfItsBlock)
{
    // Beside the block's far corner rather than its origin.
    // One cell's neighbours would never have matched it.
    const auto source = build(
        Cell{.x = 4, .y = 4},
        Building{
            .kind = BuildingKind::Farm, .stock = {10, 10}});

    sendWalker(
        Cell{.x = 6, .y = 5},
        Walker{.kind = WalkerKind::MarketSeller, .carried = 20});

    run(1);

    EXPECT_EQ(
        world.get<Building>(source).stock[resourceIndex(Resource::Food)],
        30);
}

TEST_F(BuildingSystemTest, Update_ClearsEveryCellOfADemolishedBlock)
{
    build(
        Cell{.x = 4, .y = 4},
        Building{.kind = BuildingKind::Farm, .risk = kMaxRisk});

    ASSERT_TRUE(built.has(Cell{.x = 5, .y = 5}));

    run(1);

    EXPECT_FALSE(built.has(Cell{.x = 4, .y = 4}));
    EXPECT_FALSE(built.has(Cell{.x = 5, .y = 5}));
}
