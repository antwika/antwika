#include "antwika/game/Placement.hpp"

#include <gtest/gtest.h>

#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"

using antwika::game::BuildingIndex;
using antwika::game::canPave;
using antwika::game::canPlace;
using antwika::game::Cell;
using antwika::game::Footprint;
using antwika::game::GridExtent;
using antwika::game::PathIndex;

namespace
{
    constexpr GridExtent kExtent{.width = 8, .height = 8};
    constexpr Footprint kBlock{.width = 2, .height = 2};
} // namespace

TEST(PlacementTest, CanPlace_TakesClearGroundInsideTheExtent)
{
    const PathIndex paths;
    const BuildingIndex built;

    EXPECT_TRUE(canPlace(Cell{.x = 1, .y = 1}, kBlock, kExtent, paths, built));
}

TEST(PlacementTest, CanPlace_RefusesABlockHangingOffTheGrid)
{
    const PathIndex paths;
    const BuildingIndex built;

    EXPECT_FALSE(canPlace(Cell{.x = 7, .y = 1}, kBlock, kExtent, paths, built));
}

// A road anywhere under the block is enough, not only under its origin.
TEST(PlacementTest, CanPlace_RefusesABlockOverAnyRoadCell)
{
    PathIndex paths;
    paths.insert(Cell{.x = 2, .y = 2});
    const BuildingIndex built;

    EXPECT_FALSE(canPlace(Cell{.x = 1, .y = 1}, kBlock, kExtent, paths, built));
}

TEST(PlacementTest, CanPlace_RefusesABlockOverlappingAnotherBuilding)
{
    const PathIndex paths;
    BuildingIndex built;
    built.insert(Cell{.x = 2, .y = 2}, kBlock);

    // Overlaps only at one corner, which is enough.
    EXPECT_FALSE(canPlace(Cell{.x = 1, .y = 1}, kBlock, kExtent, paths, built));
    EXPECT_TRUE(canPlace(Cell{.x = 0, .y = 0}, kBlock, kExtent, paths, built));
}

TEST(PlacementTest, CanPave_IsTheSameQuestionForOneCell)
{
    PathIndex paths;
    paths.insert(Cell{.x = 3, .y = 3});
    BuildingIndex built;
    built.insert(Cell{.x = 5, .y = 5}, Footprint{});

    EXPECT_TRUE(canPave(Cell{.x = 1, .y = 1}, kExtent, paths, built));
    EXPECT_FALSE(canPave(Cell{.x = 3, .y = 3}, kExtent, paths, built));
    EXPECT_FALSE(canPave(Cell{.x = 5, .y = 5}, kExtent, paths, built));
    EXPECT_FALSE(canPave(Cell{.x = 9, .y = 1}, kExtent, paths, built));
}
