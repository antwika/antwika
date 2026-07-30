#include <gtest/gtest.h>

#include <vector>

#include "antwika/game/Cell.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Walking.hpp"

using antwika::game::Cell;
using antwika::game::Neighbours;
using antwika::game::PathIndex;

TEST(PathIndexTest, Has_ReportsNothingBeforeAnythingIsInserted)
{
    const PathIndex paths;

    EXPECT_FALSE(paths.has(Cell{}));
    EXPECT_EQ(paths.size(), 0U);
    EXPECT_TRUE(paths.cells().empty());
}

TEST(PathIndexTest, Insert_RecordsACellAndReportsItIsNew)
{
    PathIndex paths;

    EXPECT_TRUE(paths.insert(Cell{.x = 2, .y = 3}));
    EXPECT_TRUE(paths.has(Cell{.x = 2, .y = 3}));
    EXPECT_EQ(paths.size(), 1U);
}

TEST(PathIndexTest, Insert_ReportsACellItAlreadyHeld)
{
    PathIndex paths;
    paths.insert(Cell{.x = 2, .y = 3});

    EXPECT_FALSE(paths.insert(Cell{.x = 2, .y = 3}));
    EXPECT_EQ(paths.size(), 1U);
}

TEST(PathIndexTest, Has_KeepsCellsApart)
{
    PathIndex paths;
    paths.insert(Cell{.x = 2, .y = 3});

    EXPECT_FALSE(paths.has(Cell{.x = 3, .y = 2}));
    EXPECT_FALSE(paths.has(Cell{.x = 2, .y = 4}));
}

TEST(PathIndexTest, NeighboursOf_ReportsNoneForAnIsolatedCell)
{
    PathIndex paths;
    paths.insert(Cell{.x = 5, .y = 5});

    EXPECT_EQ(paths.neighboursOf(Cell{.x = 5, .y = 5}), Neighbours{});
}

TEST(PathIndexTest, NeighboursOf_FindsEachDirectionIndependently)
{
    constexpr Cell centre{.x = 5, .y = 5};

    {
        PathIndex paths;
        paths.insert(Cell{.x = 5, .y = 4});
        EXPECT_EQ(
            paths.neighboursOf(centre), (Neighbours{.north = true}));
    }
    {
        PathIndex paths;
        paths.insert(Cell{.x = 6, .y = 5});
        EXPECT_EQ(paths.neighboursOf(centre), (Neighbours{.east = true}));
    }
    {
        PathIndex paths;
        paths.insert(Cell{.x = 5, .y = 6});
        EXPECT_EQ(
            paths.neighboursOf(centre), (Neighbours{.south = true}));
    }
    {
        PathIndex paths;
        paths.insert(Cell{.x = 4, .y = 5});
        EXPECT_EQ(paths.neighboursOf(centre), (Neighbours{.west = true}));
    }
}

TEST(PathIndexTest, NeighboursOf_IgnoresTheCellItself)
{
    PathIndex paths;
    paths.insert(Cell{.x = 0, .y = 0});

    EXPECT_EQ(paths.neighboursOf(Cell{.x = 0, .y = 0}), Neighbours{});
}

TEST(PathIndexTest, NeighboursOf_ReachesNegativeCoordinates)
{
    PathIndex paths;
    paths.insert(Cell{.x = -1, .y = 0});
    paths.insert(Cell{.x = 0, .y = -1});

    EXPECT_EQ(
        paths.neighboursOf(Cell{}),
        (Neighbours{.north = true, .west = true}));
}

// The order reaches the drawing calls.
// So it has to be the same every run, not merely exist.
TEST(PathIndexTest, Cells_ComeBackInAscendingOrderWhateverWentInFirst)
{
    PathIndex paths;
    for (const auto cell : std::vector<Cell>{
             {.x = 1, .y = 1},
             {.x = -1, .y = 4},
             {.x = 1, .y = 0},
             {.x = 0, .y = 9}})
    {
        paths.insert(cell);
    }

    EXPECT_EQ(
        std::vector<Cell>(paths.cells().begin(), paths.cells().end()),
        (std::vector<Cell>{
            {.x = -1, .y = 4},
            {.x = 0, .y = 9},
            {.x = 1, .y = 0},
            {.x = 1, .y = 1}}));
}
