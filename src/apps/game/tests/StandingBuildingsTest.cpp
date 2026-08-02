#include "antwika/game/StandingBuildings.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"

using antwika::ecs::Entity;
using antwika::ecs::World;
using antwika::game::Building;
using antwika::game::BuildingKind;
using antwika::game::Cell;
using antwika::game::standingBuildings;
using antwika::log::mocks::MockLogger;

namespace
{
    class StandingBuildingsTest : public ::testing::Test
    {
    protected:
        Entity build(Cell at, BuildingKind kind)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, at);
            world.add<Building>(entity, Building{.kind = kind});
            world.commit();
            return entity;
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
    };
} // namespace

TEST_F(StandingBuildingsTest, StandingBuildings_IsEmptyWithNothingBuilt)
{
    EXPECT_TRUE(standingBuildings(world).empty());
}

TEST_F(StandingBuildingsTest, StandingBuildings_KeysEveryCellOfABlock)
{
    // A farm is two cells square, so it holds four keys.
    const auto farm = build(Cell{.x = 4, .y = 4}, BuildingKind::Farm);

    const auto standing = standingBuildings(world);

    ASSERT_EQ(standing.size(), 4U);
    EXPECT_EQ(standing.at(Cell{.x = 4, .y = 4}), farm);
    EXPECT_EQ(standing.at(Cell{.x = 5, .y = 4}), farm);
    EXPECT_EQ(standing.at(Cell{.x = 4, .y = 5}), farm);
    EXPECT_EQ(standing.at(Cell{.x = 5, .y = 5}), farm);
}

TEST_F(StandingBuildingsTest, StandingBuildings_KeepsEveryBuildingApart)
{
    const auto house = build(Cell{.x = 0, .y = 0}, BuildingKind::House);
    const auto well = build(Cell{.x = 9, .y = 9}, BuildingKind::Well);

    const auto standing = standingBuildings(world);

    EXPECT_EQ(standing.at(Cell{.x = 0, .y = 0}), house);
    EXPECT_EQ(standing.at(Cell{.x = 9, .y = 9}), well);
    EXPECT_EQ(standing.size(), 2U);
}
