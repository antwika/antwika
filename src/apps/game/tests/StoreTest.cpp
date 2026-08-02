#include "antwika/game/Store.hpp"

#include <cstddef>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Resource.hpp"

namespace
{
    using antwika::ecs::World;
    using antwika::game::acceptsAt;
    using antwika::game::Building;
    using antwika::game::BuildingKind;
    using antwika::game::capacityOf;
    using antwika::game::Cell;
    using antwika::game::kBuildingKindCount;
    using antwika::game::kResources;
    using antwika::game::kStockCapacity;
    using antwika::game::kStoreCapacity;
    using antwika::game::Resource;
    using antwika::game::resourceIndex;
    using antwika::game::stockOf;
    using antwika::game::suppliesMarkets;
    using antwika::log::mocks::MockLogger;

    class StoreTest : public ::testing::Test
    {
    protected:
        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
    };
} // namespace

// The table is the feature, so it is walked in full.
TEST(StoreTableTest, AcceptsAt_AnswersForEveryKindAndResource)
{
    for (std::size_t index = 0; index < kBuildingKindCount; ++index)
    {
        const auto kind = static_cast<BuildingKind>(index);

        for (const auto resource : kResources)
        {
            const auto accepted = acceptsAt(kind, resource);

            EXPECT_EQ(accepted, kind == BuildingKind::Storage);
        }
    }
}

TEST(StoreTableTest, CapacityOf_GivesAStorehouseMoreRoomThanAnybody)
{
    for (std::size_t index = 0; index < kBuildingKindCount; ++index)
    {
        const auto kind = static_cast<BuildingKind>(index);

        EXPECT_EQ(
            capacityOf(kind),
            kind == BuildingKind::Storage ? kStoreCapacity
                                          : kStockCapacity);
    }
}

TEST(StoreTableTest, SuppliesMarkets_NamesTheStorehouseAndNothingElse)
{
    for (std::size_t index = 0; index < kBuildingKindCount; ++index)
    {
        const auto kind = static_cast<BuildingKind>(index);

        EXPECT_EQ(suppliesMarkets(kind), kind == BuildingKind::Storage);
    }
}

TEST_F(StoreTest, StockOf_ReadsWhatABuildingHolds)
{
    const auto entity = world.create();
    world.add<Cell>(entity, Cell{});
    world.add<Building>(
        entity,
        Building{.kind = BuildingKind::Storage, .stock = {5, 7, 9}});
    world.commit();

    EXPECT_EQ(stockOf(world, entity, Resource::Food), 5);
    EXPECT_EQ(stockOf(world, entity, Resource::Clay), 7);
    EXPECT_EQ(stockOf(world, entity, Resource::Pottery), 9);
}

// The shape every later reader of this header depends on.
// An absent component is an answer rather than an exception.
TEST_F(StoreTest, StockOf_AnswersNothingForAnEntityThatIsNoBuilding)
{
    const auto entity = world.create();
    world.add<Cell>(entity, Cell{});
    world.commit();

    EXPECT_EQ(stockOf(world, entity, Resource::Food), 0);
}

TEST_F(StoreTest, StockOf_AnswersNothingForAnEntityThatIsNotThere)
{
    EXPECT_EQ(
        stockOf(world, antwika::ecs::kNullEntity, Resource::Food), 0);
}

TEST_F(StoreTest, StockOf_AndTheComponentAgreeOnEveryResource)
{
    const auto entity = world.create();
    world.add<Cell>(entity, Cell{});
    world.add<Building>(
        entity,
        Building{.kind = BuildingKind::House, .stock = {1, 2, 3}});
    world.commit();

    for (const auto resource : kResources)
    {
        EXPECT_EQ(
            stockOf(world, entity, resource),
            world.get<Building>(entity).stock[resourceIndex(resource)]);
    }
}
