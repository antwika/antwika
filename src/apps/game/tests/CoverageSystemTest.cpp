#include "antwika/game/CoverageSystem.hpp"

#include <cstddef>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Coverage.hpp"
#include "antwika/game/Service.hpp"
#include "antwika/game/Walker.hpp"

using antwika::ecs::Entity;
using antwika::ecs::World;
using antwika::game::Building;
using antwika::game::BuildingKind;
using antwika::game::Cell;
using antwika::game::Coverage;
using antwika::game::coverageOf;
using antwika::game::CoverageSystem;
using antwika::game::kCoverageFull;
using antwika::game::kServices;
using antwika::game::Service;
using antwika::game::Walker;
using antwika::game::WalkerKind;
using antwika::log::mocks::MockLogger;

namespace
{
    class CoverageSystemTest : public ::testing::Test
    {
    protected:
        Entity build(Cell at, Building building)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, at);
            world.add<Building>(entity, building);
            world.commit();
            return entity;
        }

        Entity sendWalker(Cell at, WalkerKind kind)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, at);
            world.add<Walker>(entity, Walker{.kind = kind});
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
        CoverageSystem system;
    };
} // namespace

TEST_F(CoverageSystemTest, Update_TopsUpTheServiceAWalkerBesideItConfers)
{
    const auto house = build(
        Cell{.x = 1, .y = 0}, Building{.kind = BuildingKind::House});

    sendWalker(Cell{.x = 0, .y = 0}, WalkerKind::WaterCarrier);

    run(1);

    EXPECT_EQ(coverageOf(world, house, Service::Water), kCoverageFull);
}

TEST_F(CoverageSystemTest, Update_LeavesEveryOtherServiceAtNothing)
{
    const auto house = build(
        Cell{.x = 1, .y = 0}, Building{.kind = BuildingKind::House});

    sendWalker(Cell{.x = 0, .y = 0}, WalkerKind::Doctor);

    run(1);

    EXPECT_EQ(coverageOf(world, house, Service::Health), kCoverageFull);
    EXPECT_EQ(coverageOf(world, house, Service::Water), 0);
}

// A fireman refreshes nothing: he relieves a risk directly instead.
// See BuildingSystem's relief pass, and ServiceWalk's table.
TEST_F(CoverageSystemTest, Update_TakesNothingOffAFiremanPassingBy)
{
    const auto house = build(
        Cell{.x = 1, .y = 0}, Building{.kind = BuildingKind::House});

    sendWalker(Cell{.x = 0, .y = 0}, WalkerKind::Fireman);

    run(1);

    EXPECT_EQ(coverageOf(world, house), antwika::game::Coverage{});
}

// The aggregation is std::max against one constant.
// So it is idempotent, which is the whole ordering argument.
// Two walkers in one tick have to leave what one leaves.
TEST_F(CoverageSystemTest, Update_LeavesWhatOneWalkerWouldForTwoOfThem)
{
    const auto oneCarrier = build(
        Cell{.x = 1, .y = 0}, Building{.kind = BuildingKind::House});
    sendWalker(Cell{.x = 0, .y = 0}, WalkerKind::WaterCarrier);

    const auto twoCarriers = build(
        Cell{.x = 1, .y = 5}, Building{.kind = BuildingKind::House});
    sendWalker(Cell{.x = 0, .y = 5}, WalkerKind::WaterCarrier);
    sendWalker(Cell{.x = 2, .y = 5}, WalkerKind::WaterCarrier);

    run(1);

    EXPECT_EQ(
        coverageOf(world, twoCarriers), coverageOf(world, oneCarrier));
}

// A block is reached by any of its cells, exactly as a delivery is.
TEST_F(CoverageSystemTest, Update_ReachesABuildingByAnyCellOfItsBlock)
{
    const auto farm = build(
        Cell{.x = 4, .y = 4}, Building{.kind = BuildingKind::Farm});

    sendWalker(Cell{.x = 6, .y = 5}, WalkerKind::Doctor);

    run(1);

    EXPECT_EQ(coverageOf(world, farm, Service::Health), kCoverageFull);
}

TEST_F(CoverageSystemTest, Update_ReachesNothingFromAWalkerBesideNothing)
{
    const auto house = build(
        Cell{.x = 5, .y = 5}, Building{.kind = BuildingKind::House});

    sendWalker(Cell{.x = 0, .y = 0}, WalkerKind::WaterCarrier);

    run(1);

    EXPECT_FALSE(world.has<Coverage>(house));
}

// An errand walker confers nothing, and gives a building no component.
TEST_F(CoverageSystemTest, Update_ConfersNothingForAWalkerThatCarriesGoods)
{
    const auto house = build(
        Cell{.x = 1, .y = 0}, Building{.kind = BuildingKind::House});

    sendWalker(Cell{.x = 0, .y = 0}, WalkerKind::MarketSeller);

    run(1);

    EXPECT_EQ(coverageOf(world, house), Coverage{});
    EXPECT_FALSE(world.has<Coverage>(house));
}

TEST_F(CoverageSystemTest, Update_DecaysOneTickOfCoverageEachTick)
{
    const auto house = build(
        Cell{.x = 1, .y = 0}, Building{.kind = BuildingKind::House});

    const auto carrier =
        sendWalker(Cell{.x = 0, .y = 0}, WalkerKind::WaterCarrier);

    run(1);
    world.destroy(carrier);
    world.commit();

    run(5);

    EXPECT_EQ(
        coverageOf(world, house, Service::Water), kCoverageFull - 5);
}

TEST_F(CoverageSystemTest, Update_DecaysToNothingAndNoFurther)
{
    Coverage nearlyGone;
    nearlyGone.ticksLeft[antwika::game::serviceIndex(Service::Water)] = 2;

    const auto house = build(
        Cell{.x = 1, .y = 0}, Building{.kind = BuildingKind::House});
    antwika::game::setCoverage(world, house, nearlyGone);
    world.commit();

    run(10);

    EXPECT_EQ(coverageOf(world, house), Coverage{});
}

TEST_F(CoverageSystemTest, Update_DoesNothingWithNothingBuiltAtAll)
{
    EXPECT_NO_THROW(run(1));
}

// Coverage tops up while it is being decayed, and lands full.
// Which is only true because the decay runs first.
TEST_F(CoverageSystemTest, Update_HoldsCoverageFullWhileAWalkerIsBesideIt)
{
    const auto house = build(
        Cell{.x = 1, .y = 0}, Building{.kind = BuildingKind::House});

    sendWalker(Cell{.x = 0, .y = 0}, WalkerKind::WaterCarrier);

    run(20);

    EXPECT_EQ(coverageOf(world, house, Service::Water), kCoverageFull);
}

// The field is a function of the building set, and so is the risk.
// Two worlds built in opposite orders have to end up identical.
TEST_F(CoverageSystemTest, Update_ReachesTheSameStateUnderEitherOrder)
{
    ::testing::NiceMock<MockLogger> otherLogger;
    World other{otherLogger};
    CoverageSystem otherSystem;

    const auto put =
        [](World &into, Cell at, BuildingKind kind)
    {
        const auto entity = into.create();
        into.add<Cell>(entity, at);
        into.add<Building>(entity, Building{.kind = kind});
        return entity;
    };

    const auto walk = [](World &into, Cell at, WalkerKind kind)
    {
        const auto entity = into.create();
        into.add<Cell>(entity, at);
        into.add<Walker>(entity, Walker{.kind = kind});
    };

    const auto first = put(world, Cell{.x = 1, .y = 0}, BuildingKind::House);
    const auto second = put(world, Cell{.x = 3, .y = 0}, BuildingKind::House);
    walk(world, Cell{.x = 2, .y = 0}, WalkerKind::Fireman);
    world.commit();

    walk(other, Cell{.x = 2, .y = 0}, WalkerKind::Fireman);
    const auto otherSecond =
        put(other, Cell{.x = 3, .y = 0}, BuildingKind::House);
    const auto otherFirst =
        put(other, Cell{.x = 1, .y = 0}, BuildingKind::House);
    other.commit();

    for (std::size_t tick = 0; tick < 5; ++tick)
    {
        system.update(world, tick);
        world.commit();
        otherSystem.update(other, tick);
        other.commit();
    }

    EXPECT_EQ(coverageOf(other, otherFirst), coverageOf(world, first));
    EXPECT_EQ(coverageOf(other, otherSecond), coverageOf(world, second));

    for (const auto service : kServices)
    {
        EXPECT_EQ(
            coverageOf(other, otherFirst, service),
            coverageOf(world, first, service));
    }
}
