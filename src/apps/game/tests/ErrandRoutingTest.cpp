#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/ErrandRouting.hpp"
#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/Store.hpp"

namespace
{
    using antwika::ecs::Entity;
    using antwika::ecs::kNullEntity;
    using antwika::ecs::World;
    using antwika::game::Building;
    using antwika::game::BuildingKind;
    using antwika::game::Cell;
    using antwika::game::GridExtent;
    using antwika::game::kStoreCapacity;
    using antwika::game::nearestAccepting;
    using antwika::game::nearestHolding;
    using antwika::game::PathIndex;
    using antwika::game::Resource;
    using antwika::log::mocks::MockLogger;

    constexpr GridExtent kExtent{.width = 12, .height = 12};

    class ErrandRoutingTest : public ::testing::Test
    {
    protected:
        Entity build(Cell at, BuildingKind kind, std::int32_t food)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, at);
            world.add<Building>(
                entity, Building{.kind = kind, .stock = {food, 0, 0}});
            world.commit();
            return entity;
        }

        void paveRow(std::int32_t y, std::int32_t from, std::int32_t to)
        {
            for (std::int32_t x = from; x <= to; ++x)
            {
                paths.insert(Cell{.x = x, .y = y});
            }
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        PathIndex paths;
    };
}

TEST_F(ErrandRoutingTest, NearestAccepting_FindsNobodyInAnEmptyCity)
{
    paveRow(0, 0, 5);

    EXPECT_EQ(
        nearestAccepting(
            world, Cell{}, Resource::Food, paths, kExtent),
        kNullEntity);
}

TEST_F(ErrandRoutingTest, NearestAccepting_TakesTheShorterRoute)
{
    paveRow(0, 0, 11);

    const auto near = build(Cell{.x = 3, .y = 1}, BuildingKind::Storage, 0);
    build(Cell{.x = 8, .y = 1}, BuildingKind::Storage, 0);

    EXPECT_EQ(
        nearestAccepting(
            world, Cell{}, Resource::Food, paths, kExtent),
        near);
}

TEST_F(ErrandRoutingTest, NearestAccepting_BreaksATieOnTheLowerCell)
{
    paveRow(5, 0, 11);

    build(Cell{.x = 8, .y = 2}, BuildingKind::Storage, 0);
    const auto lower =
        build(Cell{.x = 2, .y = 2}, BuildingKind::Storage, 0);

    EXPECT_EQ(
        nearestAccepting(
            world, Cell{.x = 5, .y = 5}, Resource::Food, paths, kExtent),
        lower);
}

TEST_F(ErrandRoutingTest, NearestAccepting_PassesOverAStoreWithNoRoom)
{
    paveRow(0, 0, 11);

    build(Cell{.x = 3, .y = 1}, BuildingKind::Storage, kStoreCapacity);
    const auto roomy =
        build(Cell{.x = 8, .y = 1}, BuildingKind::Storage, 0);

    EXPECT_EQ(
        nearestAccepting(
            world, Cell{}, Resource::Food, paths, kExtent),
        roomy);
}

TEST_F(ErrandRoutingTest, NearestAccepting_PassesOverAHouse)
{
    paveRow(0, 0, 11);

    build(Cell{.x = 3, .y = 1}, BuildingKind::House, 0);

    EXPECT_EQ(
        nearestAccepting(
            world, Cell{}, Resource::Food, paths, kExtent),
        kNullEntity);
}

TEST_F(ErrandRoutingTest, NearestAccepting_PassesOverAStoreOffTheRoads)
{
    paveRow(0, 0, 3);

    build(Cell{.x = 9, .y = 5}, BuildingKind::Storage, 0);

    EXPECT_EQ(
        nearestAccepting(
            world, Cell{}, Resource::Food, paths, kExtent),
        kNullEntity);
}

TEST_F(ErrandRoutingTest, NearestAccepting_PassesOverAMarket)
{
    paveRow(0, 0, 11);

    build(Cell{.x = 3, .y = 1}, BuildingKind::Market, 0);

    EXPECT_EQ(
        nearestAccepting(
            world, Cell{}, Resource::Food, paths, kExtent),
        kNullEntity);
}

TEST_F(ErrandRoutingTest, NearestHolding_FindsTheStoreWithSomethingInIt)
{
    paveRow(0, 0, 11);

    build(Cell{.x = 3, .y = 1}, BuildingKind::Storage, 0);
    const auto stocked =
        build(Cell{.x = 8, .y = 1}, BuildingKind::Storage, 40);

    EXPECT_EQ(
        nearestHolding(world, Cell{}, Resource::Food, paths, kExtent),
        stocked);
}

TEST_F(ErrandRoutingTest, NearestHolding_NeverBuysFromAnotherMarket)
{
    paveRow(0, 0, 11);

    build(Cell{.x = 3, .y = 1}, BuildingKind::Market, 40);

    EXPECT_EQ(
        nearestHolding(world, Cell{}, Resource::Food, paths, kExtent),
        kNullEntity);
}

TEST_F(ErrandRoutingTest, NearestAccepting_TakesALaterCandidateThatIsNearer)
{
    paveRow(0, 0, 11);

    build(Cell{.x = 3, .y = 1}, BuildingKind::Storage, 0);
    const auto near = build(Cell{.x = 8, .y = 1}, BuildingKind::Storage, 0);

    EXPECT_EQ(
        nearestAccepting(
            world, Cell{.x = 11, .y = 0}, Resource::Food, paths, kExtent),
        near);
}
