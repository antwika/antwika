#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/RoadPlan.hpp"

using antwika::game::BuildingIndex;
using antwika::game::Cell;
using antwika::game::Footprint;
using antwika::game::GridExtent;
using antwika::game::planRoad;

namespace
{
    constexpr GridExtent kExtent{.width = 8, .height = 8};
} // namespace

TEST(RoadPlanTest, PlanRoad_RunsStraightAlongARow)
{
    const BuildingIndex built;

    const auto plan =
        planRoad(Cell{.x = 1, .y = 2}, Cell{.x = 4, .y = 2}, kExtent, built);

    EXPECT_TRUE(plan.valid);
    EXPECT_EQ(
        plan.cells,
        (std::vector<Cell>{
            Cell{.x = 1, .y = 2},
            Cell{.x = 2, .y = 2},
            Cell{.x = 3, .y = 2},
            Cell{.x = 4, .y = 2}}));
}

// One cell named twice is a route of one, which is a plain click.
TEST(RoadPlanTest, PlanRoad_PlansOneCellForOneCell)
{
    const BuildingIndex built;

    const auto plan =
        planRoad(Cell{.x = 3, .y = 3}, Cell{.x = 3, .y = 3}, kExtent, built);

    EXPECT_TRUE(plan.valid);
    EXPECT_EQ(plan.cells, (std::vector<Cell>{Cell{.x = 3, .y = 3}}));
}

// A building is what a route may not cross.
TEST(RoadPlanTest, PlanRoad_GoesRoundABuilding)
{
    BuildingIndex built;
    built.insert(Cell{.x = 2, .y = 2}, Footprint{});

    const auto plan =
        planRoad(Cell{.x = 1, .y = 2}, Cell{.x = 3, .y = 2}, kExtent, built);

    ASSERT_TRUE(plan.valid);

    for (const auto cell : plan.cells)
    {
        EXPECT_FALSE(built.has(cell));
    }
}

// The same question twice is the same answer.
// Which is the whole reason a replay may depend on a route at all.
TEST(RoadPlanTest, PlanRoad_AnswersTheSameWayTwice)
{
    BuildingIndex built;
    built.insert(Cell{.x = 4, .y = 4}, Footprint{});

    const auto first =
        planRoad(Cell{.x = 0, .y = 0}, Cell{.x = 7, .y = 7}, kExtent, built);
    const auto second =
        planRoad(Cell{.x = 0, .y = 0}, Cell{.x = 7, .y = 7}, kExtent, built);

    EXPECT_EQ(first, second);
}

// A refusal shown is a refusal somebody can act on.
TEST(RoadPlanTest, PlanRoad_NamesTheTwoCellsWhenThereIsNoRoute)
{
    BuildingIndex built;

    for (std::int32_t y = 0; y < kExtent.height; ++y)
    {
        built.insert(Cell{.x = 3, .y = y}, Footprint{});
    }

    const auto plan =
        planRoad(Cell{.x = 1, .y = 2}, Cell{.x = 5, .y = 2}, kExtent, built);

    EXPECT_FALSE(plan.valid);
    EXPECT_EQ(
        plan.cells,
        (std::vector<Cell>{Cell{.x = 1, .y = 2}, Cell{.x = 5, .y = 2}}));
}

// A cell off the grid is not a cell somebody could have meant.
TEST(RoadPlanTest, PlanRoad_SaysNothingAboutACellOffTheGrid)
{
    const BuildingIndex built;

    EXPECT_TRUE(
        planRoad(Cell{.x = -1, .y = 2}, Cell{.x = 4, .y = 2}, kExtent, built)
            .cells.empty());
    EXPECT_TRUE(
        planRoad(Cell{.x = 1, .y = 2}, Cell{.x = 40, .y = 2}, kExtent, built)
            .cells.empty());
}

// A building outside the extent blocks nothing inside it.
// The extent is what bounds a plan, as it bounds a placement.
TEST(RoadPlanTest, PlanRoad_IgnoresABuildingOffTheGrid)
{
    BuildingIndex built;
    built.insert(Cell{.x = 20, .y = 20}, Footprint{});

    const auto plan =
        planRoad(Cell{.x = 1, .y = 2}, Cell{.x = 2, .y = 2}, kExtent, built);

    EXPECT_TRUE(plan.valid);
    EXPECT_EQ(plan.cells.size(), 2U);
}
