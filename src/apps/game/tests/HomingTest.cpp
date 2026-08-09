#include <gtest/gtest.h>

#include <vector>

#include "antwika/game/Homing.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"

using antwika::game::BuildingIndex;
using antwika::game::Cell;
using antwika::game::Direction;
using antwika::game::Footprint;
using antwika::game::GridExtent;
using antwika::game::PathIndex;
using antwika::game::crossingCost;
using antwika::game::stepAcross;
using antwika::game::stepTowards;

namespace
{
    constexpr GridExtent kExtent{.width = 8, .height = 8};

    [[nodiscard]] PathIndex paved(const std::vector<Cell> &cells)
    {
        PathIndex paths;

        for (const auto cell : cells)
        {
            paths.insert(cell);
        }

        return paths;
    }
    constexpr Footprint kOne{.width = 1, .height = 1};

    [[nodiscard]] BuildingIndex builtOn(const std::vector<Cell> &cells)
    {
        BuildingIndex built;

        for (const auto cell : cells)
        {
            (void)built.insert(cell, kOne);
        }

        return built;
    }
}

TEST(HomingTest, StepTowards_HeadsStraightDownACorridor)
{
    const auto paths = paved(
        {{.x = 1, .y = 0}, {.x = 2, .y = 0}, {.x = 3, .y = 0}});

    EXPECT_EQ(
        stepTowards(
            Cell{.x = 1, .y = 0},
            Cell{.x = 4, .y = 0},
            Footprint{},
            paths,
            kExtent),
        Direction::East);
}

TEST(HomingTest, StepTowards_ReportsTheStepOntoAnAdjacentGoal)
{
    const auto paths = paved({{.x = 1, .y = 0}});

    EXPECT_EQ(
        stepTowards(
            Cell{.x = 1, .y = 0},
            Cell{.x = 2, .y = 0},
            Footprint{},
            paths,
            kExtent),
        Direction::East);
}

TEST(HomingTest, StepTowards_GoesRoundAnUnpavedCell)
{
    const auto paths = paved(
        {{.x = 0, .y = 0},
         {.x = 0, .y = 1},
         {.x = 1, .y = 1},
         {.x = 2, .y = 1},
         {.x = 2, .y = 0}});

    EXPECT_EQ(
        stepTowards(
            Cell{.x = 0, .y = 0},
            Cell{.x = 2, .y = 0},
            Footprint{},
            paths,
            kExtent),
        Direction::South);
}

TEST(HomingTest, StepTowards_ReportsNothingWhenTheGoalIsWalledOff)
{
    const auto paths = paved({{.x = 0, .y = 0}});

    EXPECT_FALSE(
        stepTowards(
            Cell{.x = 0, .y = 0},
            Cell{.x = 5, .y = 5},
            Footprint{},
            paths,
            kExtent)
            .has_value());
}

TEST(HomingTest, StepTowards_ReportsNothingFromOutsideTheExtent)
{
    const auto paths = paved({{.x = 1, .y = 1}});

    EXPECT_FALSE(
        stepTowards(
            Cell{.x = -1, .y = 0},
            Cell{.x = 1, .y = 1},
            Footprint{},
            paths,
            kExtent)
            .has_value());
}

TEST(HomingTest, StepTowards_ReportsNothingForAGoalOutsideTheExtent)
{
    const auto paths = paved({{.x = 1, .y = 1}});

    EXPECT_FALSE(
        stepTowards(
            Cell{.x = 1, .y = 1},
            Cell{.x = 99, .y = 99},
            Footprint{},
            paths,
            kExtent)
            .has_value());
}

TEST(HomingTest, StepTowards_ReportsNothingOnADegenerateExtent)
{
    const PathIndex paths;

    EXPECT_FALSE(
        stepTowards(
            Cell{},
            Cell{},
            Footprint{},
            paths,
            GridExtent{.width = 0, .height = 0})
            .has_value());
}

TEST(HomingTest, StepTowards_ReportsNothingWhenAlreadyThere)
{
    const auto paths = paved({{.x = 1, .y = 1}});

    EXPECT_FALSE(
        stepTowards(
            Cell{.x = 1, .y = 1},
            Cell{.x = 1, .y = 1},
            Footprint{},
            paths,
            kExtent)
            .has_value());
}

TEST(HomingTest, StepTowards_IgnoresARoadOutsideTheExtent)
{
    const auto paths =
        paved({{.x = 1, .y = 0}, {.x = 2, .y = 0}, {.x = 99, .y = 99}});

    EXPECT_EQ(
        stepTowards(
            Cell{.x = 1, .y = 0},
            Cell{.x = 3, .y = 0},
            Footprint{},
            paths,
            kExtent),
        Direction::East);
}

TEST(HomingTest, StepTowards_TakesTheEastArmOfATie)
{
    const auto paths = paved(
        {{.x = 0, .y = 0},
         {.x = 1, .y = 0},
         {.x = 0, .y = 1},
         {.x = 1, .y = 1}});

    EXPECT_EQ(
        stepTowards(
            Cell{.x = 0, .y = 0},
            Cell{.x = 1, .y = 1},
            Footprint{},
            paths,
            kExtent),
        Direction::East);
}

TEST(HomingTest, StepTowards_ReadsEveryHeadingOffTheDelta)
{
    const auto paths = paved(
        {{.x = 2, .y = 2},
         {.x = 1, .y = 2},
         {.x = 3, .y = 2},
         {.x = 2, .y = 1},
         {.x = 2, .y = 3}});

    constexpr Cell middle{.x = 2, .y = 2};

    EXPECT_EQ(
        stepTowards(
            middle,
            Cell{.x = 0, .y = 2},
            Footprint{},
            paths,
            kExtent),
        Direction::West);
    EXPECT_EQ(
        stepTowards(
            middle,
            Cell{.x = 4, .y = 2},
            Footprint{},
            paths,
            kExtent),
        Direction::East);
    EXPECT_EQ(
        stepTowards(
            middle,
            Cell{.x = 2, .y = 0},
            Footprint{},
            paths,
            kExtent),
        Direction::North);
    EXPECT_EQ(
        stepTowards(
            middle,
            Cell{.x = 2, .y = 4},
            Footprint{},
            paths,
            kExtent),
        Direction::South);
}

TEST(HomingTest, StepTowards_ReachesABlockByItsNearestCell)
{
    const auto paths = paved({{.x = 4, .y = 1}, {.x = 4, .y = 2}});

    EXPECT_EQ(
        stepTowards(
            Cell{.x = 4, .y = 1},
            Cell{.x = 2, .y = 1},
            Footprint{.width = 2, .height = 2},
            paths,
            kExtent),
        Direction::West);
}

TEST(HomingTest, StepTowards_ReportsNothingForABlockWalledOff)
{
    const auto paths = paved({{.x = 7, .y = 7}});

    EXPECT_FALSE(
        stepTowards(
            Cell{.x = 7, .y = 7},
            Cell{.x = 0, .y = 0},
            Footprint{.width = 2, .height = 2},
            paths,
            kExtent)
            .has_value());
}

TEST(HomingTest, StepTowards_IgnoresBlockCellsOutsideTheExtent)
{
    const auto paths = paved({{.x = 6, .y = 7}});

    EXPECT_EQ(
        stepTowards(
            Cell{.x = 6, .y = 7},
            Cell{.x = 7, .y = 7},
            Footprint{.width = 2, .height = 2},
            paths,
            kExtent),
        Direction::East);
}

TEST(HomingTest, StepAcross_WalksStraightAtAGoalWithNoRoadsAtAll)
{
    const BuildingIndex built;

    EXPECT_EQ(
        stepAcross(
            Cell{.x = 1, .y = 4},
            Cell{.x = 5, .y = 4},
            kOne,
            built,
            kExtent),
        Direction::East);
}

TEST(HomingTest, StepAcross_GoesRoundABuildingInTheWay)
{
    const auto built = builtOn(
        {{.x = 2, .y = 3}, {.x = 2, .y = 4}, {.x = 2, .y = 5}});

    const auto heading = stepAcross(
        Cell{.x = 1, .y = 4}, Cell{.x = 3, .y = 4}, kOne, built, kExtent);

    ASSERT_TRUE(heading.has_value());
    EXPECT_NE(*heading, Direction::East);
}

TEST(HomingTest, StepAcross_ReportsNothingFromInsideAWall)
{
    const auto built = builtOn(
        {{.x = 0, .y = 1},
         {.x = 2, .y = 1},
         {.x = 1, .y = 0},
         {.x = 1, .y = 2}});

    EXPECT_FALSE(
        stepAcross(
            Cell{.x = 1, .y = 1},
            Cell{.x = 5, .y = 5},
            kOne,
            built,
            kExtent)
            .has_value());
}

TEST(HomingTest, StepAcross_ReachesABlockItIsStandingBeside)
{
    const auto built = builtOn(
        {{.x = 4, .y = 4},
         {.x = 5, .y = 4},
         {.x = 4, .y = 5},
         {.x = 5, .y = 5}});

    EXPECT_EQ(
        stepAcross(
            Cell{.x = 3, .y = 4},
            Cell{.x = 4, .y = 4},
            Footprint{.width = 2, .height = 2},
            built,
            kExtent),
        Direction::East);
}

TEST(HomingTest, StepAcross_ReportsNothingWhenAlreadyThere)
{
    const BuildingIndex built;

    EXPECT_FALSE(
        stepAcross(
            Cell{.x = 3, .y = 3},
            Cell{.x = 3, .y = 3},
            kOne,
            built,
            kExtent)
            .has_value());
}

TEST(HomingTest, StepAcross_ReportsNothingOutsideTheExtent)
{
    const BuildingIndex built;

    EXPECT_FALSE(
        stepAcross(
            Cell{.x = -1, .y = 0},
            Cell{.x = 3, .y = 3},
            kOne,
            built,
            kExtent)
            .has_value());
    EXPECT_FALSE(
        stepAcross(
            Cell{.x = 3, .y = 3},
            Cell{.x = 99, .y = 3},
            kOne,
            built,
            kExtent)
            .has_value());
}

TEST(HomingTest, StepAcross_IgnoresABuildingOutsideTheExtent)
{
    const auto built = builtOn({{.x = -1, .y = 4}, {.x = 99, .y = 4}});

    EXPECT_EQ(
        stepAcross(
            Cell{.x = 1, .y = 4},
            Cell{.x = 5, .y = 4},
            kOne,
            built,
            kExtent),
        Direction::East);
}

TEST(HomingTest, CrossingCost_CountsTheStepsOverOpenGround)
{
    const BuildingIndex built;

    EXPECT_EQ(
        crossingCost(
            Cell{.x = 1, .y = 4},
            Cell{.x = 5, .y = 4},
            kOne,
            built,
            kExtent),
        4);

    EXPECT_EQ(
        crossingCost(
            Cell{.x = 1, .y = 4},
            Cell{.x = 1, .y = 4},
            kOne,
            built,
            kExtent),
        0);
}

TEST(HomingTest, CrossingCost_ReportsNothingWithNoRouteAtAll)
{
    const auto built = builtOn(
        {{.x = 0, .y = 1},
         {.x = 2, .y = 1},
         {.x = 1, .y = 0},
         {.x = 1, .y = 2}});

    EXPECT_FALSE(
        crossingCost(
            Cell{.x = 1, .y = 1},
            Cell{.x = 5, .y = 5},
            kOne,
            built,
            kExtent)
            .has_value());
}

TEST(HomingTest, StepAcross_IgnoresTheRoadsEntirely)
{
    const BuildingIndex built;
    const auto paths = paved({{.x = 1, .y = 4}});

    EXPECT_FALSE(
        stepTowards(
            Cell{.x = 1, .y = 4},
            Cell{.x = 5, .y = 4},
            kOne,
            paths,
            kExtent)
            .has_value());

    EXPECT_EQ(
        stepAcross(
            Cell{.x = 1, .y = 4},
            Cell{.x = 5, .y = 4},
            kOne,
            built,
            kExtent),
        Direction::East);
}
