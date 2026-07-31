#include "antwika/game/BuildingIndex.hpp"

#include <gtest/gtest.h>

#include "antwika/game/Cell.hpp"
#include "antwika/game/Footprint.hpp"

using antwika::game::BuildingIndex;
using antwika::game::Cell;
using antwika::game::Footprint;

TEST(BuildingIndexTest, AFreshIndexHoldsNothing)
{
    const BuildingIndex built;

    EXPECT_EQ(built.size(), 0U);
    EXPECT_FALSE(built.has(Cell{}));
    EXPECT_TRUE(built.cells().empty());
}

TEST(BuildingIndexTest, Insert_RecordsACellOnce)
{
    BuildingIndex built;

    EXPECT_TRUE(built.insert(Cell{.x = 1, .y = 2}, Footprint{}));
    EXPECT_FALSE(built.insert(Cell{.x = 1, .y = 2}, Footprint{}));
    EXPECT_EQ(built.size(), 1U);
    EXPECT_TRUE(built.has(Cell{.x = 1, .y = 2}));
}

TEST(BuildingIndexTest, Erase_ClearsACellAndSaysWhetherItHadOne)
{
    BuildingIndex built;
    built.insert(Cell{.x = 1, .y = 2}, Footprint{});

    EXPECT_TRUE(built.erase(Cell{.x = 1, .y = 2}, Footprint{}));
    EXPECT_FALSE(built.erase(Cell{.x = 1, .y = 2}, Footprint{}));
    EXPECT_FALSE(built.has(Cell{.x = 1, .y = 2}));
    EXPECT_EQ(built.size(), 0U);
}

// A demolished cell can be built on again.
// Which is why this is shared rather than private to one sink.
TEST(BuildingIndexTest, ACellClearedCanBeRecordedAgain)
{
    BuildingIndex built;
    built.insert(Cell{.x = 3, .y = 3}, Footprint{});
    built.erase(Cell{.x = 3, .y = 3}, Footprint{});

    EXPECT_TRUE(built.insert(Cell{.x = 3, .y = 3}, Footprint{}));
}

TEST(BuildingIndexTest, Cells_ComeBackInAscendingOrder)
{
    BuildingIndex built;
    built.insert(Cell{.x = 2, .y = 0}, Footprint{});
    built.insert(Cell{.x = 0, .y = 5}, Footprint{});
    built.insert(Cell{.x = 1, .y = 1}, Footprint{});

    const std::vector<Cell> ordered(
        built.cells().begin(), built.cells().end());

    EXPECT_EQ(
        ordered,
        (std::vector<Cell>{
            {.x = 0, .y = 5}, {.x = 1, .y = 1}, {.x = 2, .y = 0}}));
}
