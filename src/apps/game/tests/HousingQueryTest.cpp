#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/HousingQuery.hpp"
#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/Store.hpp"

namespace
{
    using antwika::ecs::World;
    using antwika::game::Building;
    using antwika::game::BuildingKind;
    using antwika::game::Cell;
    using antwika::game::Household;
    using antwika::game::householdOf;
    using antwika::game::HousingLevel;
    using antwika::game::kHousingLevels;
    using antwika::game::levelOf;
    using antwika::game::populationAt;
    using antwika::game::populationCapacityOf;
    using antwika::game::requirementOf;
    using antwika::game::setHousehold;
    using antwika::game::stockCapacityAt;
    using antwika::log::mocks::MockLogger;

    class HousingQueryTest : public ::testing::Test
    {
    protected:
        antwika::ecs::Entity build()
        {
            const auto entity = world.create();
            world.add<Cell>(entity, Cell{.x = 1, .y = 1});
            world.add<Building>(
                entity, Building{.kind = BuildingKind::House});
            world.commit();
            return entity;
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
    };
}

TEST_F(HousingQueryTest, LevelOf_AnswersTheBottomLevelWithNoHousehold)
{
    const auto house = build();

    EXPECT_FALSE(world.has<Household>(house));
    EXPECT_EQ(levelOf(world, house), HousingLevel::Tent);
    EXPECT_EQ(populationAt(world, house), 0);
    EXPECT_EQ(householdOf(world, house), Household{});
}

TEST_F(HousingQueryTest, LevelOf_AnswersTheBottomLevelForADeadEntity)
{
    const auto house = build();
    world.destroy(house);
    world.commit();

    EXPECT_EQ(levelOf(world, house), HousingLevel::Tent);
    EXPECT_EQ(populationAt(world, house), 0);
}

TEST_F(HousingQueryTest, LevelOf_AnswersWhatWasWritten)
{
    const auto house = build();

    setHousehold(
        world,
        house,
        Household{.level = HousingLevel::Hovel, .population = 3});
    world.commit();

    EXPECT_EQ(levelOf(world, house), HousingLevel::Hovel);
    EXPECT_EQ(populationAt(world, house), 3);
}

TEST_F(HousingQueryTest, SetHousehold_OverwritesAnExistingComponent)
{
    const auto house = build();

    setHousehold(world, house, Household{.level = HousingLevel::Shack});
    world.commit();

    setHousehold(world, house, Household{.level = HousingLevel::Cottage});
    world.commit();

    EXPECT_EQ(levelOf(world, house), HousingLevel::Cottage);
}

TEST(HousingCapacityTest, PopulationCapacityOf_ReadsTheLevelsOwnRow)
{
    for (const auto level : kHousingLevels)
    {
        EXPECT_EQ(
            populationCapacityOf(level),
            requirementOf(level).populationCapacity);
    }
}

TEST_F(HousingQueryTest, StockCapacityAt_GrowsWithAHousesLevel)
{
    const auto house = world.create();
    world.add<Cell>(house, Cell{.x = 1, .y = 1});
    world.add<Building>(house, Building{.kind = BuildingKind::House});
    world.commit();

    EXPECT_EQ(
        stockCapacityAt(world, house, BuildingKind::House),
        antwika::game::kStockCapacity);

    setHousehold(
        world,
        house,
        Household{.level = HousingLevel::Cottage});
    world.commit();

    EXPECT_EQ(
        stockCapacityAt(world, house, BuildingKind::House),
        antwika::game::stockCapacityOf(HousingLevel::Cottage));
}

TEST_F(HousingQueryTest, StockCapacityAt_KeepsTheKindsAnswerElsewhere)
{
    const auto storehouse = world.create();
    world.add<Cell>(storehouse, Cell{.x = 1, .y = 1});
    world.add<Building>(
        storehouse, Building{.kind = BuildingKind::Storage});
    world.commit();

    EXPECT_EQ(
        stockCapacityAt(world, storehouse, BuildingKind::Storage),
        antwika::game::capacityOf(BuildingKind::Storage));
}
