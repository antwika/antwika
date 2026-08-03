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
#include "antwika/game/PathIndex.hpp"

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
    using antwika::game::PathIndex;
    using antwika::game::populationCapacityOf;
    using antwika::game::setHousehold;
    using antwika::log::mocks::MockLogger;

    constexpr GridExtent kExtent{.width = 8, .height = 8};

    class JourneyTest : public ::testing::Test
    {
    protected:
        // A road all the way across the middle row, edge to edge.
        void paveRow(std::int32_t y)
        {
            for (std::int32_t x = 0; x < kExtent.width; ++x)
            {
                paths.insert(Cell{.x = x, .y = y});
            }
        }

        Entity house(Cell at, std::int32_t people, HousingLevel level)
        {
            const auto entity = world.create();
            world.add<Cell>(entity, at);
            world.add<Building>(
                entity, Building{.kind = BuildingKind::House});
            world.commit();

            setHousehold(
                world,
                entity,
                Household{.level = level, .population = people});
            world.commit();

            return entity;
        }

        ::testing::NiceMock<MockLogger> logger;
        World world{logger};
        PathIndex paths;
    };
} // namespace

TEST_F(JourneyTest, NearestGate_FindsTheEdgeRoadClosestToTheCell)
{
    paveRow(3);

    // Standing two cells from the west edge and five from the east.
    EXPECT_EQ(
        nearestGate(Cell{.x = 2, .y = 3}, paths, kExtent),
        (Cell{.x = 0, .y = 3}));

    EXPECT_EQ(
        nearestGate(Cell{.x = 6, .y = 3}, paths, kExtent),
        (Cell{.x = 7, .y = 3}));
}

// A tie is broken by ascending Cell, which is total.
// The two ends of this row are exactly the same distance away.
TEST_F(JourneyTest, NearestGate_BreaksATieOnTheLowerCell)
{
    paveRow(3);

    const auto middle = Cell{.x = 3, .y = 3};
    const auto reached = nearestGate(middle, paths, kExtent);

    ASSERT_TRUE(reached.has_value());
    EXPECT_EQ(*reached, (Cell{.x = 0, .y = 3}));

    // And laying the road again changes nothing about the answer.
    paveRow(3);
    EXPECT_EQ(nearestGate(middle, paths, kExtent), reached);
}

// A road that touches no edge is not a way out of the city.
TEST_F(JourneyTest, NearestGate_FindsNothingWhereNoRoadReachesAnEdge)
{
    for (std::int32_t x = 2; x <= 5; ++x)
    {
        paths.insert(Cell{.x = x, .y = 3});
    }

    EXPECT_FALSE(
        nearestGate(Cell{.x = 3, .y = 3}, paths, kExtent).has_value());
}

// And a cell with no road under it reaches nothing at all.
TEST_F(JourneyTest, NearestGate_FindsNothingFromOffTheRoads)
{
    paveRow(3);

    EXPECT_FALSE(
        nearestGate(Cell{.x = 3, .y = 6}, paths, kExtent).has_value());
}

TEST_F(JourneyTest, NearestVacancy_FindsTheNearestHouseWithRoomLeft)
{
    paveRow(3);

    const auto full = house(
        Cell{.x = 1, .y = 4},
        populationCapacityOf(HousingLevel::Tent),
        HousingLevel::Tent);
    const auto roomy =
        house(Cell{.x = 5, .y = 4}, 1, HousingLevel::Tent);

    EXPECT_EQ(
        nearestVacancy(
            world, Cell{.x = 2, .y = 3}, kNullEntity, paths, kExtent),
        roomy);

    // The full one is never the answer, however close it is.
    EXPECT_NE(
        nearestVacancy(
            world, Cell{.x = 1, .y = 3}, kNullEntity, paths, kExtent),
        full);
}

// The house being left is excluded rather than filtered by occupancy.
// It has room by the very fact of shedding somebody.
// So without that it would take them straight back in.
TEST_F(JourneyTest, NearestVacancy_NeverAnswersWithTheHouseBeingLeft)
{
    paveRow(3);

    const auto leaving =
        house(Cell{.x = 3, .y = 4}, 1, HousingLevel::Tent);

    EXPECT_EQ(
        nearestVacancy(
            world, Cell{.x = 3, .y = 3}, leaving, paths, kExtent),
        kNullEntity);
}

// A well is not somewhere anybody lives, so it is not a vacancy.
TEST_F(JourneyTest, NearestVacancy_IgnoresEveryKindNobodyLivesIn)
{
    paveRow(3);

    const auto well = world.create();
    world.add<Cell>(well, Cell{.x = 3, .y = 4});
    world.add<Building>(well, Building{.kind = BuildingKind::Well});
    world.commit();

    EXPECT_EQ(
        nearestVacancy(
            world, Cell{.x = 3, .y = 3}, kNullEntity, paths, kExtent),
        kNullEntity);
}

// A house nothing can walk to is no use to somebody on foot.
TEST_F(JourneyTest, NearestVacancy_IgnoresAHouseNoRouteReaches)
{
    paveRow(3);
    house(Cell{.x = 3, .y = 7}, 0, HousingLevel::Tent);

    EXPECT_EQ(
        nearestVacancy(
            world, Cell{.x = 3, .y = 3}, kNullEntity, paths, kExtent),
        kNullEntity);
}

TEST_F(JourneyTest, EqualityComparesEveryField)
{
    const Journey journey{
        .towards = Cell{.x = 1, .y = 2}, .house = kNullEntity};

    EXPECT_EQ(journey, journey);

    auto elsewhere = journey;
    elsewhere.towards = Cell{.x = 2, .y = 1};
    EXPECT_NE(journey, elsewhere);

    auto joining = journey;
    joining.house = Entity{1};
    EXPECT_NE(journey, joining);
}

// beside() short-circuits north, east, south, west in that order.
// So each arm needs a case where it is the one that answers.
TEST_F(JourneyTest, Beside_AnswersForEachSideOfABlockInTurn)
{
    constexpr antwika::game::Footprint kOne{.width = 1, .height = 1};
    constexpr Cell kAt{.x = 4, .y = 4};

    // South of the block, so its north neighbour is the block.
    EXPECT_TRUE(beside(Cell{.x = 4, .y = 5}, kAt, kOne));

    // West of it, so its east neighbour is.
    EXPECT_TRUE(beside(Cell{.x = 3, .y = 4}, kAt, kOne));

    // North of it, so its south neighbour is.
    EXPECT_TRUE(beside(Cell{.x = 4, .y = 3}, kAt, kOne));

    // East of it, so its west neighbour is.
    EXPECT_TRUE(beside(Cell{.x = 5, .y = 4}, kAt, kOne));

    // And a cell on the diagonal touches none of its sides.
    EXPECT_FALSE(beside(Cell{.x = 5, .y = 5}, kAt, kOne));
}

// A road outside the extent is not a gate, however far out it is.
// There is no city beyond the grid for it to lead to.
TEST_F(JourneyTest, NearestGate_IgnoresARoadOutsideTheExtent)
{
    paveRow(3);
    paths.insert(Cell{.x = -1, .y = 3});
    paths.insert(Cell{.x = 99, .y = 3});

    EXPECT_EQ(
        nearestGate(Cell{.x = 1, .y = 3}, paths, kExtent),
        (Cell{.x = 0, .y = 3}));
}

// The top and bottom edges are ways out too, not only the sides.
// A column of road down the middle has a gate at each end of it.
TEST_F(JourneyTest, NearestGate_FindsAGateOnTheTopAndBottomEdges)
{
    for (std::int32_t y = 0; y < kExtent.height; ++y)
    {
        paths.insert(Cell{.x = 4, .y = y});
    }

    EXPECT_EQ(
        nearestGate(Cell{.x = 4, .y = 2}, paths, kExtent),
        (Cell{.x = 4, .y = 0}));
    EXPECT_EQ(
        nearestGate(Cell{.x = 4, .y = 6}, paths, kExtent),
        (Cell{.x = 4, .y = kExtent.height - 1}));
}

// Two houses with room, so the nearer of them is the answer.
// Which is the comparison a single candidate never makes.
TEST_F(JourneyTest, NearestVacancy_TakesTheNearerOfTwoWithRoom)
{
    paveRow(3);

    house(Cell{.x = 6, .y = 4}, 0, HousingLevel::Tent);
    const auto near = house(Cell{.x = 2, .y = 4}, 0, HousingLevel::Tent);

    EXPECT_EQ(
        nearestVacancy(
            world, Cell{.x = 1, .y = 3}, kNullEntity, paths, kExtent),
        near);
}

// And the other way round: the later candidate is the nearer one.
// Which is the comparison the first candidate never makes.
TEST_F(JourneyTest, NearestVacancy_TakesALaterCandidateThatIsNearer)
{
    paveRow(3);

    house(Cell{.x = 1, .y = 4}, 0, HousingLevel::Tent);
    const auto near = house(Cell{.x = 6, .y = 4}, 0, HousingLevel::Tent);

    EXPECT_EQ(
        nearestVacancy(
            world, Cell{.x = 7, .y = 3}, kNullEntity, paths, kExtent),
        near);
}
