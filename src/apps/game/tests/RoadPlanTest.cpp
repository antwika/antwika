#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <vector>

#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/BuildTool.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/RoadPlan.hpp"

using antwika::game::BuildingIndex;
using antwika::game::Cell;
using antwika::game::Footprint;
using antwika::game::GridExtent;
using antwika::game::BuildTool;
using antwika::game::planDrag;
using antwika::game::planPlots;
using antwika::game::planRoad;

namespace
{
    constexpr GridExtent kExtent{.width = 8, .height = 8};
}

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

TEST(RoadPlanTest, PlanRoad_PlansOneCellForOneCell)
{
    const BuildingIndex built;

    const auto plan =
        planRoad(Cell{.x = 3, .y = 3}, Cell{.x = 3, .y = 3}, kExtent, built);

    EXPECT_TRUE(plan.valid);
    EXPECT_EQ(plan.cells, (std::vector<Cell>{Cell{.x = 3, .y = 3}}));
}

TEST(RoadPlanTest, PlanRoad_GoesRoundABuilding)
{
    BuildingIndex built;
    built.insert(Cell{.x = 2, .y = 2}, Footprint{});

    const auto plan =
        planRoad(Cell{.x = 1, .y = 2}, Cell{.x = 3, .y = 2}, kExtent, built);

    ASSERT_TRUE(plan.valid);
    ASSERT_FALSE(plan.cells.empty());

    EXPECT_EQ(plan.cells.front(), (Cell{.x = 1, .y = 2}));
    EXPECT_EQ(plan.cells.back(), (Cell{.x = 3, .y = 2}));
    EXPECT_GT(plan.cells.size(), 3U);

    for (const auto cell : plan.cells)
    {
        EXPECT_FALSE(built.has(cell));
    }
}

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

TEST(RoadPlanTest, PlanRoad_IgnoresABuildingOffTheGrid)
{
    BuildingIndex built;
    built.insert(Cell{.x = 20, .y = 20}, Footprint{});

    const auto plan =
        planRoad(Cell{.x = 1, .y = 2}, Cell{.x = 2, .y = 2}, kExtent, built);

    EXPECT_TRUE(plan.valid);
    EXPECT_EQ(plan.cells.size(), 2U);
}

TEST(RoadPlanTest, PlanPlots_BoxesInEveryCellBetweenTwoCorners)
{
    const auto plan =
        planPlots(Cell{.x = 1, .y = 1}, Cell{.x = 2, .y = 2}, kExtent);

    EXPECT_TRUE(plan.valid);
    EXPECT_EQ(
        plan.cells,
        (std::vector<Cell>{
            Cell{.x = 1, .y = 1},
            Cell{.x = 2, .y = 1},
            Cell{.x = 1, .y = 2},
            Cell{.x = 2, .y = 2}}));
}

TEST(RoadPlanTest, PlanPlots_ReadsACornerPairInEitherOrder)
{
    EXPECT_EQ(
        planPlots(Cell{.x = 4, .y = 5}, Cell{.x = 2, .y = 3}, kExtent),
        planPlots(Cell{.x = 2, .y = 3}, Cell{.x = 4, .y = 5}, kExtent));
}

TEST(RoadPlanTest, PlanPlots_BoxesInTheOneCellADragNeverLeft)
{
    const auto plan =
        planPlots(Cell{.x = 3, .y = 3}, Cell{.x = 3, .y = 3}, kExtent);

    EXPECT_EQ(plan.cells, (std::vector<Cell>{Cell{.x = 3, .y = 3}}));
}

TEST(RoadPlanTest, PlanPlots_SaysNothingAboutACellOffTheGrid)
{
    EXPECT_FALSE(
        planPlots(Cell{.x = -1, .y = 2}, Cell{.x = 4, .y = 2}, kExtent)
            .valid);
    EXPECT_FALSE(
        planPlots(Cell{.x = 1, .y = 2}, Cell{.x = 40, .y = 2}, kExtent)
            .valid);
}

TEST(RoadPlanTest, PlanDrag_PlansARoadForTheRoadTool)
{
    const BuildingIndex built;

    EXPECT_EQ(
        planDrag(
            BuildTool::Road,
            Cell{.x = 1, .y = 2},
            Cell{.x = 4, .y = 2},
            kExtent,
            built),
        planRoad(
            Cell{.x = 1, .y = 2}, Cell{.x = 4, .y = 2}, kExtent, built));
}

TEST(RoadPlanTest, PlanDrag_PlansABoxOfPlotsForTheHouseTool)
{
    const BuildingIndex built;

    EXPECT_EQ(
        planDrag(
            BuildTool::House,
            Cell{.x = 1, .y = 2},
            Cell{.x = 4, .y = 4},
            kExtent,
            built),
        planPlots(
            Cell{.x = 1, .y = 2}, Cell{.x = 4, .y = 4}, kExtent));
}

TEST(RoadPlanTest, PlanDrag_PlansNothingForAToolThatDoesNotDrag)
{
    const BuildingIndex built;

    EXPECT_FALSE(
        planDrag(
            BuildTool::Farm,
            Cell{.x = 1, .y = 2},
            Cell{.x = 4, .y = 4},
            kExtent,
            built)
            .valid);
    EXPECT_FALSE(
        planDrag(
            std::nullopt,
            Cell{.x = 1, .y = 2},
            Cell{.x = 4, .y = 4},
            kExtent,
            built)
            .valid);
}
