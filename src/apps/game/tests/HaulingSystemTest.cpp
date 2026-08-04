#include "antwika/game/HaulingSystem.hpp"

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
#include "antwika/game/BuildingSystem.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Errand.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/Store.hpp"
#include "antwika/game/Walker.hpp"

namespace
{
    using antwika::ecs::Entity;
    using antwika::ecs::kNullEntity;
    using antwika::ecs::World;
    using antwika::game::Building;
    using antwika::game::BuildingIndex;
    using antwika::game::BuildingKind;
    using antwika::game::BuildingSystem;
    using antwika::game::Cell;
    using antwika::game::Errand;
    using antwika::game::ErrandLeg;
    using antwika::game::footprintOf;
    using antwika::game::GridExtent;
    using antwika::game::HaulingSystem;
    using antwika::game::kStoreCapacity;
    using antwika::game::kWalkerLoad;
    using antwika::game::PathIndex;
    using antwika::game::Resource;
    using antwika::game::resourceIndex;
    using antwika::game::Walker;
    using antwika::game::WalkerKind;
    using antwika::log::mocks::MockLogger;

    constexpr GridExtent kExtent{.width = 12, .height = 12};

    class HaulingSystemTest : public ::testing::Test
    {
    protected:
        Entity build(Cell at, BuildingKind kind, std::int32_t stock)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, at);
            world.add<Building>(
                entity,
                Building{.kind = kind, .stock = {stock, stock, stock}});
            world.commit();
            built.insert(at, footprintOf(kind));
            return entity;
        }

        Entity sendCart(Cell at, Entity home)
        {
            const auto cart = world.create();
            world.add<Cell>(cart, at);
            world.add<Walker>(
                cart,
                Walker{.kind = WalkerKind::CartPusher, .home = home});
            world.commit();

            auto sender = world.get<Building>(home);
            sender.walkers[0] = cart;
            world.set<Building>(home, sender);
            world.commit();
            return cart;
        }

        void paveRow(std::int32_t y, std::int32_t from, std::int32_t to)
        {
            for (std::int32_t x = from; x <= to; ++x)
            {
                paths.insert(Cell{.x = x, .y = y});
            }
        }

        void haul()
        {
            hauling.update(world, 0);
            world.commit();
        }

        void deliver()
        {
            buildings.update(world, 0);
            world.commit();
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        PathIndex paths;
        BuildingIndex built;
        HaulingSystem hauling{paths, kExtent};
        BuildingSystem buildings{built, kExtent, antwika::game::GameConfig{}};
    };
} // namespace

TEST_F(HaulingSystemTest, Update_LoadsACartOutOfItsFarmsBarn)
{
    paveRow(5, 0, 11);
    const auto farm = build(Cell{.x = 0, .y = 3}, BuildingKind::Farm, 40);
    build(Cell{.x = 6, .y = 2}, BuildingKind::Storage, 0);
    const auto cart = sendCart(Cell{.x = 0, .y = 5}, farm);

    haul();

    EXPECT_EQ(world.get<Walker>(cart).carried, 40);
    EXPECT_EQ(
        world.get<Building>(farm).stock[resourceIndex(Resource::Food)],
        0);
}

TEST_F(HaulingSystemTest, Update_NeverLoadsMoreThanACartHolds)
{
    paveRow(5, 0, 11);
    const auto farm = build(
        Cell{.x = 0, .y = 3}, BuildingKind::Farm, kWalkerLoad * 2);
    build(Cell{.x = 6, .y = 2}, BuildingKind::Storage, 0);
    const auto cart = sendCart(Cell{.x = 0, .y = 5}, farm);

    haul();

    EXPECT_EQ(world.get<Walker>(cart).carried, kWalkerLoad);
}

TEST_F(HaulingSystemTest, Update_PointsTheCartAtTheStore)
{
    paveRow(5, 0, 11);
    const auto farm = build(Cell{.x = 0, .y = 3}, BuildingKind::Farm, 40);
    const auto store =
        build(Cell{.x = 6, .y = 2}, BuildingKind::Storage, 0);
    const auto cart = sendCart(Cell{.x = 0, .y = 5}, farm);

    haul();

    ASSERT_TRUE(world.has<Errand>(cart));
    EXPECT_EQ(world.get<Errand>(cart).destination, store);
    EXPECT_EQ(world.get<Errand>(cart).carrying, Resource::Food);
    EXPECT_EQ(world.get<Errand>(cart).leg, ErrandLeg::Outbound);
}

// The load still leaves the barn, and goes round on the cart instead.
// Which is what keeps a city with no storehouse in it fed.
TEST_F(HaulingSystemTest, Update_LoadsACartWithNowhereToTakeItAnyway)
{
    paveRow(5, 0, 11);
    const auto farm = build(Cell{.x = 0, .y = 3}, BuildingKind::Farm, 40);
    const auto cart = sendCart(Cell{.x = 0, .y = 5}, farm);

    haul();

    ASSERT_TRUE(world.has<Errand>(cart));
    EXPECT_EQ(world.get<Errand>(cart).destination, kNullEntity);
    EXPECT_EQ(world.get<Walker>(cart).carried, 40);
}

TEST_F(HaulingSystemTest, Update_LeavesACartWithNothingToHaulUnloaded)
{
    paveRow(5, 0, 11);
    const auto farm = build(Cell{.x = 0, .y = 3}, BuildingKind::Farm, 0);
    const auto cart = sendCart(Cell{.x = 0, .y = 5}, farm);

    haul();

    EXPECT_FALSE(world.has<Errand>(cart));
    EXPECT_EQ(world.get<Walker>(cart).carried, 0);
}

TEST_F(HaulingSystemTest, Update_LeavesAFarmWithNoRoadBesideItBoundNowhere)
{
    paveRow(5, 0, 11);
    const auto farm = build(Cell{.x = 0, .y = 0}, BuildingKind::Farm, 40);
    build(Cell{.x = 6, .y = 2}, BuildingKind::Storage, 0);
    const auto cart = sendCart(Cell{.x = 0, .y = 5}, farm);

    haul();

    ASSERT_TRUE(world.has<Errand>(cart));
    EXPECT_EQ(world.get<Errand>(cart).destination, kNullEntity);
}

TEST_F(HaulingSystemTest, Update_UnbindsAnEmptiedCart)
{
    paveRow(5, 0, 11);
    const auto farm = build(Cell{.x = 0, .y = 3}, BuildingKind::Farm, 40);
    build(Cell{.x = 6, .y = 2}, BuildingKind::Storage, 0);
    const auto cart = sendCart(Cell{.x = 0, .y = 5}, farm);

    haul();

    auto emptied = world.get<Walker>(cart);
    emptied.carried = 0;
    world.set<Walker>(cart, emptied);
    world.commit();

    haul();

    EXPECT_EQ(world.get<Errand>(cart).destination, kNullEntity);
}

// Otherwise a cart stands at a full door for ever.
// And the farm that sent it never gets its slot back.
TEST_F(HaulingSystemTest, Update_UnbindsACartWhoseStoreFilledUp)
{
    paveRow(5, 0, 11);
    const auto farm = build(Cell{.x = 0, .y = 3}, BuildingKind::Farm, 40);
    const auto store =
        build(Cell{.x = 6, .y = 2}, BuildingKind::Storage, 0);
    const auto cart = sendCart(Cell{.x = 0, .y = 5}, farm);

    haul();

    auto filled = world.get<Building>(store);
    filled.stock[resourceIndex(Resource::Food)] = kStoreCapacity;
    world.set<Building>(store, filled);
    world.commit();

    haul();

    EXPECT_EQ(world.get<Errand>(cart).destination, kNullEntity);
    EXPECT_EQ(world.get<Walker>(cart).carried, 40);
}

TEST_F(HaulingSystemTest, Update_LeavesALoadedCartOnItsWayAlone)
{
    paveRow(5, 0, 11);
    const auto farm = build(Cell{.x = 0, .y = 3}, BuildingKind::Farm, 40);
    build(Cell{.x = 6, .y = 2}, BuildingKind::Storage, 0);
    const auto cart = sendCart(Cell{.x = 0, .y = 5}, farm);

    haul();
    haul();

    EXPECT_NE(world.get<Errand>(cart).destination, kNullEntity);
    EXPECT_EQ(world.get<Walker>(cart).carried, 40);
}

TEST_F(HaulingSystemTest, Update_LeavesAWalkerThatIsNoCartPusherAlone)
{
    paveRow(5, 0, 11);
    const auto farm = build(Cell{.x = 0, .y = 3}, BuildingKind::Farm, 40);

    const auto fireman = world.create();
    world.add<Cell>(fireman, Cell{.x = 0, .y = 5});
    world.add<Walker>(
        fireman, Walker{.kind = WalkerKind::Fireman, .home = farm});
    world.commit();

    auto sender = world.get<Building>(farm);
    sender.walkers[0] = fireman;
    world.set<Building>(farm, sender);
    world.commit();

    haul();

    EXPECT_FALSE(world.has<Errand>(fireman));
}

TEST_F(HaulingSystemTest, Update_LeavesAKindThatHaulsNothingAlone)
{
    paveRow(5, 0, 11);
    const auto well = build(Cell{.x = 0, .y = 4}, BuildingKind::Well, 40);
    const auto cart = sendCart(Cell{.x = 0, .y = 5}, well);

    haul();

    EXPECT_FALSE(world.has<Errand>(cart));
}

// The guard's whole purpose, and the reason an errand is bound at all.
TEST_F(HaulingSystemTest, Deliver_WalksACartPastAHouseWithoutUnloading)
{
    paveRow(5, 0, 11);
    const auto farm = build(Cell{.x = 0, .y = 3}, BuildingKind::Farm, 40);
    const auto store =
        build(Cell{.x = 6, .y = 2}, BuildingKind::Storage, 0);
    const auto house =
        build(Cell{.x = 3, .y = 4}, BuildingKind::House, 10);
    const auto cart = sendCart(Cell{.x = 0, .y = 5}, farm);

    haul();

    // Stood beside the house, on the road it would walk along.
    world.set<Cell>(cart, Cell{.x = 3, .y = 5});
    world.commit();

    deliver();

    EXPECT_EQ(
        world.get<Building>(house).stock[resourceIndex(Resource::Food)],
        10);
    EXPECT_EQ(world.get<Walker>(cart).carried, 40);
    EXPECT_EQ(
        world.get<Building>(store).stock[resourceIndex(Resource::Food)],
        0);
}

TEST_F(HaulingSystemTest, Deliver_UnloadsACartAtTheStoreItWasSentTo)
{
    paveRow(5, 0, 11);
    const auto farm = build(Cell{.x = 0, .y = 3}, BuildingKind::Farm, 40);
    const auto store =
        build(Cell{.x = 6, .y = 2}, BuildingKind::Storage, 0);
    const auto cart = sendCart(Cell{.x = 0, .y = 5}, farm);

    haul();

    world.set<Cell>(cart, Cell{.x = 6, .y = 5});
    world.commit();

    deliver();

    EXPECT_EQ(
        world.get<Building>(store).stock[resourceIndex(Resource::Food)],
        40);
    EXPECT_EQ(world.get<Walker>(cart).carried, 0);
}

// A load with nowhere to go is handed out as the cart passes.
// Except back to the barn it came from, which would move nothing.
TEST_F(HaulingSystemTest, Deliver_HandsARoamingLoadToAHouseButNotAFarm)
{
    paveRow(5, 0, 11);
    const auto farm = build(Cell{.x = 0, .y = 3}, BuildingKind::Farm, 40);
    const auto house =
        build(Cell{.x = 3, .y = 4}, BuildingKind::House, 0);
    const auto cart = sendCart(Cell{.x = 0, .y = 5}, farm);

    haul();

    EXPECT_EQ(
        world.get<Building>(farm).stock[resourceIndex(Resource::Food)],
        0);

    world.set<Cell>(cart, Cell{.x = 3, .y = 5});
    world.commit();

    deliver();

    EXPECT_EQ(
        world.get<Building>(house).stock[resourceIndex(Resource::Food)],
        40);
}

// A cart whose store was demolished under it keeps its errand here.
// WalkerSystem is what removes such a walker, and it runs earlier.
TEST_F(HaulingSystemTest, Update_LeavesACartWhoseStoreWasDemolishedBound)
{
    paveRow(5, 0, 11);
    const auto farm = build(Cell{.x = 0, .y = 3}, BuildingKind::Farm, 40);
    const auto store =
        build(Cell{.x = 6, .y = 2}, BuildingKind::Storage, 0);
    const auto cart = sendCart(Cell{.x = 0, .y = 5}, farm);

    haul();

    world.destroy(store);
    world.commit();

    haul();

    EXPECT_EQ(world.get<Errand>(cart).destination, store);
}
