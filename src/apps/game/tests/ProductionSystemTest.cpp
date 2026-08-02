#include "antwika/game/ProductionSystem.hpp"

#include <cstddef>
#include <cstdint>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Production.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/Store.hpp"

namespace
{
    using antwika::ecs::Entity;
    using antwika::ecs::World;
    using antwika::game::Building;
    using antwika::game::BuildingKind;
    using antwika::game::capacityOf;
    using antwika::game::Cell;
    using antwika::game::consumedToProduce;
    using antwika::game::kBuildingKindCount;
    using antwika::game::kProductionBatch;
    using antwika::game::kProductionPeriodTicks;
    using antwika::game::kStockCapacity;
    using antwika::game::producedBy;
    using antwika::game::Production;
    using antwika::game::ProductionSystem;
    using antwika::game::Resource;
    using antwika::game::resourceIndex;
    using antwika::log::mocks::MockLogger;

    class ProductionSystemTest : public ::testing::Test
    {
    protected:
        Entity build(BuildingKind kind, Building building)
        {
            building.kind = kind;
            const auto entity = world.create();
            world.add<Cell>(entity, Cell{.x = 1, .y = 1});
            world.add<Building>(entity, building);
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

        [[nodiscard]] std::int32_t held(Entity entity, Resource resource)
        {
            return world.get<Building>(entity)
                .stock[resourceIndex(resource)];
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        ProductionSystem system;
    };

    // One tick to be given a countdown, then the countdown itself.
    constexpr std::size_t kFirstBatch =
        static_cast<std::size_t>(kProductionPeriodTicks) + 2;
} // namespace

TEST(ProductionTableTest, ProducedBy_NamesOnlyTheThreeThatMakeAnything)
{
    for (std::size_t index = 0; index < kBuildingKindCount; ++index)
    {
        const auto kind = static_cast<BuildingKind>(index);

        EXPECT_EQ(
            producedBy(kind).has_value(),
            kind == BuildingKind::Farm || kind == BuildingKind::ClayPit
                || kind == BuildingKind::Workshop);
    }
}

TEST(ProductionTableTest, ConsumedToProduce_NamesOnlyTheWorkshop)
{
    for (std::size_t index = 0; index < kBuildingKindCount; ++index)
    {
        const auto kind = static_cast<BuildingKind>(index);

        EXPECT_EQ(
            consumedToProduce(kind).has_value(),
            kind == BuildingKind::Workshop);
    }
}

TEST_F(ProductionSystemTest, Update_GivesAProducerACountdownItLacks)
{
    const auto farm = build(BuildingKind::Farm, Building{});

    run(1);

    EXPECT_TRUE(world.has<Production>(farm));
    EXPECT_EQ(
        world.get<Production>(farm).ticksUntilOutput,
        kProductionPeriodTicks);
}

TEST_F(ProductionSystemTest, Update_LeavesAKindThatMakesNothingAlone)
{
    const auto house = build(BuildingKind::House, Building{});

    run(kFirstBatch);

    EXPECT_FALSE(world.has<Production>(house));
}

TEST_F(ProductionSystemTest, Update_FinishesABatchOnItsOwnPeriod)
{
    const auto farm =
        build(BuildingKind::Farm, Building{.stock = {0, 0, 0}});

    run(kFirstBatch - 1);
    EXPECT_EQ(held(farm, Resource::Food), 0);

    run(1);
    EXPECT_EQ(held(farm, Resource::Food), kProductionBatch);
}

TEST_F(ProductionSystemTest, Update_StartsTheNextBatchAfterOne)
{
    const auto farm =
        build(BuildingKind::Farm, Building{.stock = {0, 0, 0}});

    run(kFirstBatch);

    EXPECT_EQ(
        world.get<Production>(farm).ticksUntilOutput,
        kProductionPeriodTicks);
}

TEST_F(ProductionSystemTest, Update_HoldsAProducerAtItsCapacity)
{
    const auto farm = build(
        BuildingKind::Farm, Building{.stock = {kStockCapacity, 0, 0}});

    run(kFirstBatch);

    EXPECT_EQ(held(farm, Resource::Food), kStockCapacity);
}

// Held at zero rather than reset, exactly as a spawn countdown is.
// So a barn emptied after a long wait fills on the very next tick.
TEST_F(ProductionSystemTest, Update_FinishesAtOnceOnceThereIsRoom)
{
    const auto farm = build(
        BuildingKind::Farm, Building{.stock = {kStockCapacity, 0, 0}});

    run(kFirstBatch);

    auto emptied = world.get<Building>(farm);
    emptied.stock[resourceIndex(Resource::Food)] = 0;
    world.set<Building>(farm, emptied);
    world.commit();

    run(1);

    EXPECT_EQ(held(farm, Resource::Food), kProductionBatch);
}

TEST_F(ProductionSystemTest, Update_MakesNothingInAWorkshopWithNoClay)
{
    const auto workshop =
        build(BuildingKind::Workshop, Building{.stock = {0, 0, 0}});

    run(kFirstBatch);

    EXPECT_EQ(held(workshop, Resource::Pottery), 0);
}

TEST_F(ProductionSystemTest, Update_TurnsAWorkshopsClayIntoPottery)
{
    const auto workshop = build(
        BuildingKind::Workshop,
        Building{.stock = {0, kProductionBatch, 0}});

    run(kFirstBatch);

    EXPECT_EQ(held(workshop, Resource::Clay), 0);
    EXPECT_EQ(held(workshop, Resource::Pottery), kProductionBatch);
}

TEST_F(ProductionSystemTest, Update_NeverTakesAProducerPastItsCapacity)
{
    const auto farm = build(
        BuildingKind::Farm,
        Building{.stock = {capacityOf(BuildingKind::Farm) - 1, 0, 0}});

    run(kFirstBatch);

    EXPECT_EQ(
        held(farm, Resource::Food), capacityOf(BuildingKind::Farm));
}

TEST_F(ProductionSystemTest, Update_SweepsTheCountdownPoolWithItsBuilding)
{
    const auto farm = build(BuildingKind::Farm, Building{});
    const auto house = build(BuildingKind::House, Building{});

    run(1);

    world.destroy(farm);
    world.destroy(house);
    world.commit();

    EXPECT_FALSE(world.has<Production>(farm));
    EXPECT_FALSE(world.alive(house));
}

TEST_F(ProductionSystemTest, Update_DropsACountdownForABuildingAlreadyGone)
{
    const auto farm = build(BuildingKind::Farm, Building{});

    world.destroy(farm);
    system.update(world, 0);
    world.commit();

    EXPECT_FALSE(world.has<Production>(farm));
}
