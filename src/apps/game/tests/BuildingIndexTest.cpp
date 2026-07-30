#include <gtest/gtest.h>

#include <vector>

#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Cell.hpp"

using antwika::game::BuildingIndex;
using antwika::game::Cell;

TEST(BuildingIndexTest, Insert_RecordsACellOnce)
{
    BuildingIndex index;

    EXPECT_TRUE(index.insert(Cell{.x = 1, .y = 2}));
    EXPECT_FALSE(index.insert(Cell{.x = 1, .y = 2}));
    EXPECT_EQ(index.size(), 1U);
}

TEST(BuildingIndexTest, Has_AnswersForRecordedAndUnrecordedCells)
{
    BuildingIndex index;
    index.insert(Cell{.x = 1, .y = 2});

    EXPECT_TRUE(index.has(Cell{.x = 1, .y = 2}));
    EXPECT_FALSE(index.has(Cell{.x = 2, .y = 1}));
}

TEST(BuildingIndexTest, Erase_FreesTheGroundAgain)
{
    BuildingIndex index;
    index.insert(Cell{.x = 1, .y = 2});

    EXPECT_TRUE(index.erase(Cell{.x = 1, .y = 2}));
    EXPECT_FALSE(index.has(Cell{.x = 1, .y = 2}));
    EXPECT_EQ(index.size(), 0U);
}

TEST(BuildingIndexTest, Erase_SaysSoWhenThereWasNothingThere)
{
    BuildingIndex index;

    EXPECT_FALSE(index.erase(Cell{.x = 9, .y = 9}));
}

// Ordered, so nothing about a hash can reach what is drawn first.
TEST(BuildingIndexTest, Cells_ComeBackInAscendingOrder)
{
    BuildingIndex index;
    index.insert(Cell{.x = 2, .y = 0});
    index.insert(Cell{.x = 0, .y = 5});
    index.insert(Cell{.x = 0, .y = 1});

    const std::vector<Cell> ordered(
        index.cells().begin(), index.cells().end());

    EXPECT_EQ(
        ordered,
        (std::vector<Cell>{
            Cell{.x = 0, .y = 1},
            Cell{.x = 0, .y = 5},
            Cell{.x = 2, .y = 0}}));
}
