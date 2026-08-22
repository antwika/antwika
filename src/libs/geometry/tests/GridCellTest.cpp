#include <gtest/gtest.h>

#include <cstdint>
#include <cstdlib>

#include <antwika/geometry/GridCell.hpp>
#include <antwika/geometry/GridStep.hpp>
#include <antwika/geometry/Size.hpp>

using antwika::geometry::cellWithin;
using antwika::geometry::GridCell;
using antwika::geometry::GridStep;
using antwika::geometry::kFourNeighbourSteps;
using antwika::geometry::Size;
using antwika::geometry::steppedFrom;

namespace
{
    constexpr Size kBoardSize{.width = 4, .height = 3};
}

TEST(GridCellTest, CellWithin_TakesAColumnAndRowInsideTheGrid)
{
    const auto foundCell = cellWithin(kBoardSize, 2, 1);

    ASSERT_TRUE(foundCell.has_value());
    EXPECT_EQ(foundCell->column, 2U);
    EXPECT_EQ(foundCell->row, 1U);
}

TEST(GridCellTest, CellWithin_RefusesAColumnOrRowBeforeTheGrid)
{
    EXPECT_FALSE(cellWithin(kBoardSize, -1, 0).has_value());
    EXPECT_FALSE(cellWithin(kBoardSize, 0, -1).has_value());
}

TEST(GridCellTest, CellWithin_RefusesAColumnOrRowPastTheGrid)
{
    EXPECT_FALSE(cellWithin(kBoardSize, 4, 0).has_value());
    EXPECT_FALSE(cellWithin(kBoardSize, 0, 3).has_value());
}

TEST(GridCellTest, SteppedFrom_TakesTheCellTheStepLandsOn)
{
    const auto foundCell =
        steppedFrom(kBoardSize, GridCell{.column = 1, .row = 1},
                    GridStep{.acrossStep = 1});

    ASSERT_TRUE(foundCell.has_value());
    EXPECT_EQ(*foundCell, (GridCell{.column = 2, .row = 1}));
}

TEST(GridCellTest, SteppedFrom_RefusesAStepOffTheGrid)
{
    EXPECT_FALSE(
        steppedFrom(kBoardSize, GridCell{.column = 0, .row = 0},
                    GridStep{.acrossStep = -1})
            .has_value());
}

TEST(GridCellTest, FourNeighbours_StepOneCellEachWay)
{
    ASSERT_EQ(kFourNeighbourSteps.size(), 4U);

    for (const auto step : kFourNeighbourSteps)
    {
        const auto reach =
            std::abs(step.acrossStep) + std::abs(step.downStep);

        EXPECT_EQ(reach, 1);
    }
}
