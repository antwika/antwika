#include <optional>

#include <gtest/gtest.h>

#include <antwika/game/TileAtlas.hpp>

#include "antwika/atlas_editor/SpriteGuides.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"

/**
 * @file
 * @brief What the guides drawn over a slot are, and where they come from.
 *
 * The second place this editor is held to the game's own contract, on
 * DefaultSheetSizeTest's terms and for its reason: a diamond drawn in the
 * wrong place is one an artist paints to, and art painted to it is wrong
 * everywhere the game blits it.
 * The game's include directory reaches this file the same way.
 * The editor itself still builds knowing nothing about the game.
 */
namespace
{
    using antwika::atlas_editor::guidesForTile;
    using antwika::atlas_editor::SpriteGuides;
    using antwika::atlas_editor::TileGrid;
    using antwika::game::AtlasKind;

    // The three sheets, as slot sizes and as the geometry they promise.
    // Written out from wiki/apps/game-texture-atlas.md, not derived.
    // So this reads as the contract rather than as the code again.
    TEST(SpriteGuidesTest, TheOneByOneSheetsSlotHasTheContractsDiamond)
    {
        const auto guides = guidesForTile(TileGrid{.width = 64,
                                                   .height = 96});

        ASSERT_TRUE(guides.has_value());
        EXPECT_EQ(guides->pivot.x, 32);
        EXPECT_EQ(guides->pivot.y, 64);
        EXPECT_EQ(guides->footprint.width, 32U);
        EXPECT_EQ(guides->footprint.height, 16U);
    }

    TEST(SpriteGuidesTest, TheTwoByTwoSheetsSlotHasTheContractsDiamond)
    {
        const auto guides = guidesForTile(TileGrid{.width = 96,
                                                   .height = 112});

        ASSERT_TRUE(guides.has_value());
        EXPECT_EQ(guides->pivot.x, 48);
        EXPECT_EQ(guides->pivot.y, 80);
        EXPECT_EQ(guides->footprint.width, 64U);
        EXPECT_EQ(guides->footprint.height, 32U);
    }

    TEST(SpriteGuidesTest, TheThreeByThreeSheetsSlotHasTheContractsDiamond)
    {
        const auto guides = guidesForTile(TileGrid{.width = 128,
                                                   .height = 128});

        ASSERT_TRUE(guides.has_value());
        EXPECT_EQ(guides->pivot.x, 64);
        EXPECT_EQ(guides->pivot.y, 96);
        EXPECT_EQ(guides->footprint.width, 96U);
        EXPECT_EQ(guides->footprint.height, 48U);
    }

    // And the same three again, against the header that addresses them.
    // The cases above say what the numbers are.
    // This says the numbers are the game's own.
    // So a sprite size moved there is a red test here.
    // Rather than a diamond quietly drawn where no cell is.
    TEST(SpriteGuidesTest, EverySheetsPivotIsTheOneTheGameAnchorsTo)
    {
        for (const auto kind :
             {AtlasKind::OneByOne,
              AtlasKind::TwoByTwo,
              AtlasKind::ThreeByThree})
        {
            const auto spec = antwika::game::atlasSpec(kind);
            const auto guides = guidesForTile(
                TileGrid{
                    .width = spec.spriteSize.width,
                    .height = spec.spriteSize.height});

            ASSERT_TRUE(guides.has_value());
            EXPECT_EQ(guides->pivot.x, spec.pivot.x);
            EXPECT_EQ(guides->pivot.y, spec.pivot.y);
        }
    }

    // The diamond's top corner is at the same y in all three sheets.
    // That is the contract's own way of saying the headroom is fixed.
    TEST(SpriteGuidesTest, EverySheetsDiamondStartsAtTheSameHeadroom)
    {
        for (const auto kind :
             {AtlasKind::OneByOne,
              AtlasKind::TwoByTwo,
              AtlasKind::ThreeByThree})
        {
            const auto sprite =
                antwika::game::atlasSpec(kind).spriteSize;
            const auto guides = guidesForTile(
                TileGrid{
                    .width = sprite.width, .height = sprite.height});

            ASSERT_TRUE(guides.has_value());
            EXPECT_EQ(
                guides->pivot.y
                    - static_cast<std::int32_t>(guides->footprint.height),
                48);
        }
    }

    // A cell's diamond is 32 by 16 whatever sheet draws it.
    // A slot's own diamond is that times the footprint it stands on.
    TEST(SpriteGuidesTest, EverySheetsDiamondIsAWholeNumberOfCells)
    {
        const auto cell = antwika::game::kIsoTileSize;

        for (const auto kind :
             {AtlasKind::OneByOne,
              AtlasKind::TwoByTwo,
              AtlasKind::ThreeByThree})
        {
            const auto sprite =
                antwika::game::atlasSpec(kind).spriteSize;
            const auto guides = guidesForTile(
                TileGrid{
                    .width = sprite.width, .height = sprite.height});

            ASSERT_TRUE(guides.has_value());
            EXPECT_EQ(guides->footprint.width % cell.width, 0U);
            EXPECT_EQ(guides->footprint.height % cell.height, 0U);
            EXPECT_EQ(
                guides->footprint.width / cell.width,
                guides->footprint.height / cell.height);
        }
    }

    // A slot with no room for the margins has no diamond to draw.
    // Refused rather than wrapped, the arithmetic being unsigned.
    // A subtraction past zero is a diamond the size of the world.
    TEST(SpriteGuidesTest, ASlotNarrowerThanItsMarginsHasNoGuides)
    {
        EXPECT_FALSE(
            guidesForTile(TileGrid{.width = 32, .height = 96})
                .has_value());
        EXPECT_FALSE(
            guidesForTile(TileGrid{.width = 8, .height = 96})
                .has_value());
    }

    TEST(SpriteGuidesTest, ASlotShorterThanItsBandsHasNoGuides)
    {
        EXPECT_FALSE(
            guidesForTile(TileGrid{.width = 64, .height = 80})
                .has_value());
        EXPECT_FALSE(
            guidesForTile(TileGrid{.width = 64, .height = 8})
                .has_value());
    }

    // What is left over has to be an isometric diamond.
    // A rectangle of any other shape is not one.
    // Guides drawn to it would be drawn to a projection nothing has.
    TEST(SpriteGuidesTest, ASlotLeavingNoIsometricDiamondHasNoGuides)
    {
        EXPECT_FALSE(
            guidesForTile(TileGrid{.width = 64, .height = 112})
                .has_value());
    }

    // The default slot size is the 1x1 sheet's.
    // So a session opened with no --tile is one the guides come out for.
    TEST(SpriteGuidesTest, TheDefaultSlotSizeHasGuides)
    {
        EXPECT_TRUE(guidesForTile(TileGrid{}).has_value());
    }

    TEST(SpriteGuidesTest, TwoSetsOfGuidesCompareByTheirNumbers)
    {
        constexpr SpriteGuides guides{
            .pivot = {.x = 32, .y = 64},
            .footprint = {.width = 32, .height = 16}};

        // One differing in each half, so neither is compared alone.
        constexpr SpriteGuides wider{
            .pivot = {.x = 32, .y = 64},
            .footprint = {.width = 64, .height = 32}};
        constexpr SpriteGuides lower{
            .pivot = {.x = 48, .y = 80},
            .footprint = {.width = 32, .height = 16}};

        EXPECT_EQ(guides, guides);
        EXPECT_NE(guides, wider);
        EXPECT_NE(guides, lower);
    }
} // namespace
