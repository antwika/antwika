#include "antwika/game/Homing.hpp"

#include <vector>

#include <gtest/gtest.h>

#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"

using antwika::game::Cell;
using antwika::game::Direction;
using antwika::game::Footprint;
using antwika::game::GridExtent;
using antwika::game::PathIndex;
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
} // namespace

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
    // A building does not stand on a road.
    // So arriving is a step onto the one cell that is not one.
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
    // The direct line is not a road, so the route has to bend.
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

// GridGraph refuses a non-positive extent.
// So this is answered before it is built, not by catching.
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
    // PathIndex takes any cell.
    // The search is numbered over the extent.
    // So a road beyond it must not be written through the flags.
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

// The claim the whole design rests on.
// Two equally short routes resolve the same way every run.
// Which is the only reason a replay may depend on a path.
TEST(HomingTest, StepTowards_BreaksATieTheSameWayEveryTime)
{
    // A square, so both ways round are the same length.
    const auto paths = paved(
        {{.x = 0, .y = 0},
         {.x = 1, .y = 0},
         {.x = 0, .y = 1},
         {.x = 1, .y = 1}});

    const auto first =
        stepTowards(
            Cell{.x = 0, .y = 0},
            Cell{.x = 1, .y = 1},
            Footprint{},
            paths,
            kExtent);

    for (int again = 0; again < 8; ++again)
    {
        EXPECT_EQ(
            stepTowards(
                Cell{.x = 0, .y = 0},
                Cell{.x = 1, .y = 1},
                Footprint{},
                paths,
                kExtent),
            first);
    }
}

// Each of the four headings is read off the delta.
// So this walks out of a crossroads in every direction.
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

// The goal is a block.
// So every cell of it is reachable and any of them is an arrival.
TEST(HomingTest, StepTowards_ReachesABlockByItsNearestCell)
{
    const auto paths = paved({{.x = 4, .y = 1}, {.x = 4, .y = 2}});

    // A two-by-two at (2,1) covers (2,1), (3,1), (2,2) and (3,2).
    // So a walker east of it steps onto the near corner.
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

// Placement never allows a block off the edge.
// But this is a free function.
// Its guard has to hold for what it is handed.
TEST(HomingTest, StepTowards_IgnoresBlockCellsOutsideTheExtent)
{
    const auto paths = paved({{.x = 6, .y = 7}});

    // A two-by-two at the far corner runs off both edges.
    // So only the corner cell itself may be passed through.
    EXPECT_EQ(
        stepTowards(
            Cell{.x = 6, .y = 7},
            Cell{.x = 7, .y = 7},
            Footprint{.width = 2, .height = 2},
            paths,
            kExtent),
        Direction::East);
}
