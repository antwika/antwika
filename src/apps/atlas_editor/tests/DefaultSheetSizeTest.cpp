#include <gtest/gtest.h>

#include <antwika/game/TileAtlas.hpp>

#include "AtlasSpecsFixture.hpp"

#include "antwika/atlas_editor/AtlasEditor.hpp"

namespace
{
    using antwika::atlas_editor::kDefaultSheetSize;
    using antwika::game::AtlasKind;
    using antwika::game::testing::kTestSpecs;

    constexpr auto kGameSheet =
        kTestSpecs.of(AtlasKind::OneByOne).sheetSize();

    TEST(DefaultSheetSizeTest, AtlasSpec_SizesTheBlankSheetForTheGame)
    {
        EXPECT_EQ(kDefaultSheetSize, kGameSheet);
    }

    TEST(DefaultSheetSizeTest, AtlasSpec_MakesTheSheetWholeSlots)
    {
        const auto sprite =
            antwika::game::atlasSpec(
                kTestSpecs,
                AtlasKind::OneByOne).spriteSize;

        EXPECT_EQ(
            kDefaultSheetSize.width,
            kTestSpecs.of(AtlasKind::OneByOne).columns * sprite.width);
        EXPECT_EQ(
            kDefaultSheetSize.height,
            kTestSpecs.of(AtlasKind::OneByOne).rows * sprite.height);
    }
}
