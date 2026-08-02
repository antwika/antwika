#include "antwika/game/Footprint.hpp"

#include <gtest/gtest.h>

#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/GridExtent.hpp"

using antwika::game::BuildingKind;
using antwika::game::Cell;
using antwika::game::covers;
using antwika::game::fitsIn;
using antwika::game::Footprint;
using antwika::game::footprintOf;
using antwika::game::GridExtent;
using antwika::game::kBuildingKindCount;

namespace
{
    constexpr GridExtent kExtent{.width = 8, .height = 8};
} // namespace

TEST(FootprintTest, AFreshFootprintIsOneCell)
{
    constexpr Footprint one;

    EXPECT_EQ(one.width, 1);
    EXPECT_EQ(one.height, 1);
}

TEST(FootprintTest, EveryKindHasASquareFootprint)
{
    for (std::size_t index = 0; index < kBuildingKindCount; ++index)
    {
        const auto footprint =
            footprintOf(static_cast<BuildingKind>(index));

        EXPECT_EQ(footprint.width, footprint.height) << index;
        EXPECT_GT(footprint.width, 0) << index;
    }
}

TEST(FootprintTest, Covers_TakesEveryCellOfTheBlockAndNoOther)
{
    constexpr Cell origin{.x = 2, .y = 3};
    constexpr Footprint block{.width = 2, .height = 2};

    EXPECT_TRUE(covers(origin, block, Cell{.x = 2, .y = 3}));
    EXPECT_TRUE(covers(origin, block, Cell{.x = 3, .y = 3}));
    EXPECT_TRUE(covers(origin, block, Cell{.x = 2, .y = 4}));
    EXPECT_TRUE(covers(origin, block, Cell{.x = 3, .y = 4}));

    EXPECT_FALSE(covers(origin, block, Cell{.x = 1, .y = 3}));
    EXPECT_FALSE(covers(origin, block, Cell{.x = 4, .y = 3}));
    EXPECT_FALSE(covers(origin, block, Cell{.x = 2, .y = 2}));
    EXPECT_FALSE(covers(origin, block, Cell{.x = 2, .y = 5}));
}

TEST(FootprintTest, FitsIn_RefusesABlockHangingOffTheEdge)
{
    constexpr Footprint block{.width = 3, .height = 3};

    EXPECT_TRUE(fitsIn(Cell{.x = 5, .y = 5}, block, kExtent));
    EXPECT_FALSE(fitsIn(Cell{.x = 6, .y = 5}, block, kExtent));
    EXPECT_FALSE(fitsIn(Cell{.x = 5, .y = 6}, block, kExtent));
    EXPECT_FALSE(fitsIn(Cell{.x = -1, .y = 0}, block, kExtent));
}

TEST(FootprintTest, FitsIn_TakesAOneCellBlockAnywhereInTheExtent)
{
    EXPECT_TRUE(fitsIn(Cell{.x = 7, .y = 7}, Footprint{}, kExtent));
    EXPECT_FALSE(fitsIn(Cell{.x = 8, .y = 7}, Footprint{}, kExtent));
}

TEST(FootprintTest, EqualityComparesBothExtents)
{
    constexpr Footprint block{.width = 2, .height = 3};

    EXPECT_EQ(block, (Footprint{.width = 2, .height = 3}));
    EXPECT_NE(block, (Footprint{.width = 3, .height = 3}));
    EXPECT_NE(block, (Footprint{.width = 2, .height = 2}));
}

// The footprints are the contract with whoever draws the art.
// So the table is asserted outright rather than read off.
TEST(FootprintTest, EveryKindIsTheSizeTheAtlasContractSays)
{
    EXPECT_EQ(footprintOf(BuildingKind::House), (Footprint{1, 1}));
    EXPECT_EQ(footprintOf(BuildingKind::Farm), (Footprint{2, 2}));
    EXPECT_EQ(footprintOf(BuildingKind::ClayPit), (Footprint{2, 2}));
    EXPECT_EQ(footprintOf(BuildingKind::Workshop), (Footprint{2, 2}));
    EXPECT_EQ(footprintOf(BuildingKind::Storage), (Footprint{3, 3}));
    EXPECT_EQ(footprintOf(BuildingKind::Market), (Footprint{2, 2}));
    EXPECT_EQ(footprintOf(BuildingKind::Well), (Footprint{1, 1}));
    EXPECT_EQ(footprintOf(BuildingKind::Doctor), (Footprint{1, 1}));
    EXPECT_EQ(footprintOf(BuildingKind::FireStation), (Footprint{1, 1}));
    EXPECT_EQ(
        footprintOf(BuildingKind::EngineerPost), (Footprint{1, 1}));
}
