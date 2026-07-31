#include <gtest/gtest.h>

#include <antwika/gfx/Size.hpp>

#include "antwika/atlas_editor/Pixel.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"

using antwika::atlas_editor::columnsIn;
using antwika::atlas_editor::Pixel;
using antwika::atlas_editor::rowsIn;
using antwika::atlas_editor::slotAt;
using antwika::atlas_editor::TileGrid;
using antwika::gfx::Size;

namespace
{
    // The game's own sheet: eight columns by four rows.
    constexpr Size kAtlas{.width = 1024, .height = 256};
    constexpr TileGrid kTiles{.width = 128, .height = 64};
} // namespace

TEST(TileGridTest, Equality_ComparesBothDimensions)
{
    EXPECT_EQ(kTiles, (TileGrid{.width = 128, .height = 64}));
    EXPECT_NE(kTiles, (TileGrid{.width = 64, .height = 64}));
    EXPECT_NE(kTiles, (TileGrid{.width = 128, .height = 32}));
}

TEST(TileGridTest, ColumnsAndRows_DivideTheGameSheetIntoItsSlots)
{
    EXPECT_EQ(columnsIn(kTiles, kAtlas), 8U);
    EXPECT_EQ(rowsIn(kTiles, kAtlas), 4U);
}

TEST(TileGridTest, ColumnsAndRows_CountWholeSlotsOnly)
{
    const Size ragged{.width = 300, .height = 100};

    EXPECT_EQ(columnsIn(kTiles, ragged), 2U);
    EXPECT_EQ(rowsIn(kTiles, ragged), 1U);
}

TEST(TileGridTest, ColumnsAndRows_AreNoneForAGridWithNoExtent)
{
    EXPECT_EQ(columnsIn(TileGrid{.width = 0, .height = 0}, kAtlas), 0U);
    EXPECT_EQ(rowsIn(TileGrid{.width = 0, .height = 0}, kAtlas), 0U);
}

TEST(TileGridTest, SlotAt_CountsLeftToRightThenTopToBottom)
{
    EXPECT_EQ(slotAt(kTiles, kAtlas, Pixel{.x = 0, .y = 0}), 0U);
    EXPECT_EQ(slotAt(kTiles, kAtlas, Pixel{.x = 127, .y = 63}), 0U);
    EXPECT_EQ(slotAt(kTiles, kAtlas, Pixel{.x = 128, .y = 0}), 1U);
    EXPECT_EQ(slotAt(kTiles, kAtlas, Pixel{.x = 0, .y = 64}), 8U);
    EXPECT_EQ(slotAt(kTiles, kAtlas, Pixel{.x = 1023, .y = 255}), 31U);
}

TEST(TileGridTest, SlotAt_AnswersNothingOutsideTheSheet)
{
    EXPECT_FALSE(
        slotAt(kTiles, kAtlas, Pixel{.x = -1, .y = 0}).has_value());
    EXPECT_FALSE(
        slotAt(kTiles, kAtlas, Pixel{.x = 0, .y = -1}).has_value());
    EXPECT_FALSE(
        slotAt(kTiles, kAtlas, Pixel{.x = 1024, .y = 0}).has_value());
    EXPECT_FALSE(
        slotAt(kTiles, kAtlas, Pixel{.x = 0, .y = 256}).has_value());
}

// A sheet may be a fraction of a tile wider than its slots.
// That strip down its right edge belongs to no slot at all.
TEST(TileGridTest, SlotAt_AnswersNothingInAPartialSlot)
{
    const Size ragged{.width = 300, .height = 64};

    EXPECT_EQ(slotAt(kTiles, ragged, Pixel{.x = 255, .y = 0}), 1U);
    EXPECT_FALSE(
        slotAt(kTiles, ragged, Pixel{.x = 260, .y = 0}).has_value());
}

TEST(TileGridTest, SlotAt_AnswersNothingForAGridWithNoExtent)
{
    EXPECT_FALSE(
        slotAt(TileGrid{.width = 0, .height = 64}, kAtlas, Pixel{})
            .has_value());
    EXPECT_FALSE(
        slotAt(TileGrid{.width = 128, .height = 0}, kAtlas, Pixel{})
            .has_value());
}
