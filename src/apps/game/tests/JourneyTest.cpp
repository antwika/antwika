#include <gtest/gtest.h>

#include <cstdint>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/HousingQuery.hpp"
#include "antwika/game/Journey.hpp"

namespace
{
    using antwika::ecs::Entity;
    using antwika::ecs::kNullEntity;
    using antwika::ecs::World;
    using antwika::game::Building;
    using antwika::game::BuildingKind;
    using antwika::game::Cell;
    using antwika::game::GridExtent;
    using antwika::game::Household;
    using antwika::game::HousingLevel;
    using antwika::game::beside;
using antwika::game::Journey;
    using antwika::game::nearestGate;
    using antwika::game::nearestVacancy;
        using antwika::game::populationCapacityOf;
    using antwika::game::setHousehold;
    using antwika::log::mocks::MockLogger;

    constexpr GridExtent kExtent{.width = 8, .height = 8};

    class JourneyTest : public ::testing::Test
    {
    protected:
        Entity house(Cell at, std::int32_t people, HousingLevel level)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, at);
            world.add<Building>(
                entity, Building{.kind = BuildingKind::House});
            world.commit();

            (void)built.insert(
                at, antwika::game::footprintOf(BuildingKind::House));

            setHousehold(
                world,
                entity,
                Household{.level = level, .population = people});
            world.commit();

            return entity;
        }

        void wallDown(std::int32_t x)
        {
            for (std::int32_t y = 0; y < kExtent.height; ++y)
            {
                (void)built.insert(
                    Cell{.x = x, .y = y},
                    antwika::game::footprintOf(BuildingKind::Well));
            }
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        antwika::game::BuildingIndex built;
    };
}

TEST_F(JourneyTest, NearestGate_FindsTheEdgeCellClosestToTheCell)
{
    EXPECT_EQ(
        nearestGate(Cell{.x = 2, .y = 3}, built, kExtent),
        (Cell{.x = 0, .y = 3}));

    EXPECT_EQ(
        nearestGate(Cell{.x = 6, .y = 3}, built, kExtent),
        (Cell{.x = 7, .y = 3}));

    EXPECT_EQ(
        nearestGate(Cell{.x = 3, .y = 1}, built, kExtent),
        (Cell{.x = 3, .y = 0}));
}

TEST_F(JourneyTest, NearestGate_FindsAWayOutWithNoRoadsAnywhere)
{
    EXPECT_TRUE(
        nearestGate(Cell{.x = 4, .y = 4}, built, kExtent).has_value());
}

TEST_F(JourneyTest, NearestGate_BreaksATieOnTheLowerCell)
{
    const auto middle = Cell{.x = 3, .y = 3};
    const auto reached = nearestGate(middle, built, kExtent);

    ASSERT_TRUE(reached.has_value());
    EXPECT_EQ(*reached, (Cell{.x = 0, .y = 3}));

    (void)built.insert(
        Cell{.x = 6, .y = 6},
        antwika::game::footprintOf(BuildingKind::Well));

    EXPECT_EQ(nearestGate(middle, built, kExtent), reached);
}

TEST_F(JourneyTest, NearestGate_FindsNothingFromInsideAWall)
{
    for (const auto around : {
             Cell{.x = 3, .y = 2},
             Cell{.x = 3, .y = 4},
             Cell{.x = 2, .y = 3},
             Cell{.x = 4, .y = 3}})
    {
        (void)built.insert(
            around, antwika::game::footprintOf(BuildingKind::Well));
    }

    EXPECT_FALSE(
        nearestGate(Cell{.x = 3, .y = 3}, built, kExtent).has_value());
}

TEST_F(JourneyTest, NearestGate_WalksRoundABuildingInTheWay)
{
    wallDown(1);

    const auto reached = nearestGate(Cell{.x = 2, .y = 3}, built, kExtent);

    ASSERT_TRUE(reached.has_value());
    EXPECT_FALSE(built.has(*reached));
    EXPECT_NE(*reached, (Cell{.x = 0, .y = 3}));
    EXPECT_GT(reached->x, 1);
}

TEST_F(JourneyTest, NearestGate_NeverAnswersWithABuiltEdgeCell)
{
    for (std::int32_t y = 0; y < kExtent.height; ++y)
    {
        (void)built.insert(
            Cell{.x = 0, .y = y},
            antwika::game::footprintOf(BuildingKind::Well));
    }

    const auto reached = nearestGate(Cell{.x = 1, .y = 3}, built, kExtent);

    ASSERT_TRUE(reached.has_value());
    EXPECT_NE(reached->x, 0);
}

TEST_F(JourneyTest, NearestVacancy_FindsTheNearestHouseWithRoomLeft)
{
    const auto full = house(
        Cell{.x = 1, .y = 4},
        populationCapacityOf(HousingLevel::Tent),
        HousingLevel::Tent);
    const auto roomy =
        house(Cell{.x = 5, .y = 4}, 1, HousingLevel::Tent);

    EXPECT_EQ(
        nearestVacancy(
            world, Cell{.x = 2, .y = 3}, kNullEntity, built, kExtent),
        roomy);

    EXPECT_NE(
        nearestVacancy(
            world, Cell{.x = 1, .y = 3}, kNullEntity, built, kExtent),
        full);
}

TEST_F(JourneyTest, NearestVacancy_NeverAnswersWithTheHouseBeingLeft)
{
    const auto leaving =
        house(Cell{.x = 3, .y = 4}, 1, HousingLevel::Tent);

    EXPECT_EQ(
        nearestVacancy(
            world, Cell{.x = 3, .y = 3}, leaving, built, kExtent),
        kNullEntity);
}

TEST_F(JourneyTest, NearestVacancy_IgnoresEveryKindNobodyLivesIn)
{
    const auto well = world.create();
    world.add<Cell>(well, Cell{.x = 3, .y = 4});
    world.add<Building>(well, Building{.kind = BuildingKind::Well});
    world.commit();

    EXPECT_EQ(
        nearestVacancy(
            world, Cell{.x = 3, .y = 3}, kNullEntity, built, kExtent),
        kNullEntity);
}

TEST_F(JourneyTest, NearestVacancy_IgnoresAHouseNoRouteReaches)
{
    house(Cell{.x = 3, .y = 7}, 0, HousingLevel::Tent);

    for (const auto around : {
             Cell{.x = 2, .y = 7},
             Cell{.x = 4, .y = 7},
             Cell{.x = 3, .y = 6}})
    {
        (void)built.insert(
            around, antwika::game::footprintOf(BuildingKind::Well));
    }

    EXPECT_EQ(
        nearestVacancy(
            world, Cell{.x = 3, .y = 3}, kNullEntity, built, kExtent),
        kNullEntity);
}

TEST_F(JourneyTest, OperatorEquals_EqualityComparesEveryField)
{
    const Journey journey{
        .towards = Cell{.x = 1, .y = 2}, .house = kNullEntity};

    const auto twin = journey;
    EXPECT_EQ(journey, twin);

    auto elsewhere = journey;
    elsewhere.towards = Cell{.x = 2, .y = 1};
    EXPECT_NE(journey, elsewhere);

    auto joining = journey;
    joining.house = Entity{1};
    EXPECT_NE(journey, joining);
}

TEST_F(JourneyTest, Beside_AnswersForEachSideOfABlockInTurn)
{
    constexpr antwika::game::Footprint kOne{.width = 1, .height = 1};
    constexpr Cell kAt{.x = 4, .y = 4};

    EXPECT_TRUE(beside(Cell{.x = 4, .y = 5}, kAt, kOne));

    EXPECT_TRUE(beside(Cell{.x = 3, .y = 4}, kAt, kOne));

    EXPECT_TRUE(beside(Cell{.x = 4, .y = 3}, kAt, kOne));

    EXPECT_TRUE(beside(Cell{.x = 5, .y = 4}, kAt, kOne));

    EXPECT_FALSE(beside(Cell{.x = 5, .y = 5}, kAt, kOne));
}

TEST_F(JourneyTest, NearestGate_FindsAGateOnTheTopAndBottomEdges)
{
    EXPECT_EQ(
        nearestGate(Cell{.x = 4, .y = 2}, built, kExtent),
        (Cell{.x = 4, .y = 0}));
    EXPECT_EQ(
        nearestGate(Cell{.x = 4, .y = 6}, built, kExtent),
        (Cell{.x = 4, .y = kExtent.height - 1}));
}

TEST_F(JourneyTest, NearestVacancy_TakesTheNearerOfTwoWithRoom)
{
    house(Cell{.x = 6, .y = 4}, 0, HousingLevel::Tent);
    const auto near = house(Cell{.x = 2, .y = 4}, 0, HousingLevel::Tent);

    EXPECT_EQ(
        nearestVacancy(
            world, Cell{.x = 1, .y = 3}, kNullEntity, built, kExtent),
        near);
}

TEST_F(JourneyTest, NearestVacancy_TakesALaterCandidateThatIsNearer)
{
    house(Cell{.x = 1, .y = 4}, 0, HousingLevel::Tent);
    const auto near = house(Cell{.x = 6, .y = 4}, 0, HousingLevel::Tent);

    EXPECT_EQ(
        nearestVacancy(
            world, Cell{.x = 7, .y = 3}, kNullEntity, built, kExtent),
        near);
}
