#include <gtest/gtest.h>

#include "antwika/atlas_editor/AtlasEditor.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"

namespace
{
    using antwika::atlas_editor::kDefaultSheetSize;
    using antwika::atlas_editor::kDefaultTileHeight;
    using antwika::atlas_editor::kDefaultTileWidth;

    TEST(DefaultSheetSizeTest, kDefaultSheetSize_MakesWholeSlots)
    {
        EXPECT_EQ(kDefaultSheetSize.width % kDefaultTileWidth, 0U);
        EXPECT_EQ(kDefaultSheetSize.height % kDefaultTileHeight, 0U);
    }

    TEST(DefaultSheetSizeTest, kDefaultSheetSize_HoldsASquareOfSlots)
    {
        EXPECT_EQ(
            kDefaultSheetSize.width / kDefaultTileWidth,
            kDefaultSheetSize.height / kDefaultTileHeight);
    }
}
