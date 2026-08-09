#include <gtest/gtest.h>

#include <optional>

#include <antwika/game/TileAtlas.hpp>

#include "AtlasSpecsFixture.hpp"

#include "antwika/atlas_editor/SpriteGuides.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"

namespace
{
    using antwika::atlas_editor::guidesForTile;
    using antwika::atlas_editor::SpriteGuides;
    using antwika::atlas_editor::TileGrid;
    using antwika::game::AtlasKind;
    using antwika::game::testing::kTestSpecs;

    TEST(SpriteGuidesTest, GuidesForTile_DiamondsAOneByOneSlot)
    {
        const auto guides = guidesForTile(TileGrid{.width = 64,
                                                   .height = 96});

        ASSERT_TRUE(guides.has_value());
        EXPECT_EQ(guides->pivot.x, 32);
        EXPECT_EQ(guides->pivot.y, 64);
        EXPECT_EQ(guides->footprint.width, 32U);
        EXPECT_EQ(guides->footprint.height, 16U);
    }

    TEST(SpriteGuidesTest, GuidesForTile_DiamondsATwoByTwoSlot)
    {
        const auto guides = guidesForTile(TileGrid{.width = 96,
                                                   .height = 112});

        ASSERT_TRUE(guides.has_value());
        EXPECT_EQ(guides->pivot.x, 48);
        EXPECT_EQ(guides->pivot.y, 80);
        EXPECT_EQ(guides->footprint.width, 64U);
        EXPECT_EQ(guides->footprint.height, 32U);
    }

    TEST(SpriteGuidesTest, GuidesForTile_DiamondsAThreeByThreeSlot)
    {
        const auto guides = guidesForTile(TileGrid{.width = 128,
                                                   .height = 128});

        ASSERT_TRUE(guides.has_value());
        EXPECT_EQ(guides->pivot.x, 64);
        EXPECT_EQ(guides->pivot.y, 96);
        EXPECT_EQ(guides->footprint.width, 96U);
        EXPECT_EQ(guides->footprint.height, 48U);
    }

    TEST(SpriteGuidesTest, GuidesForTile_PivotsWhereTheGameAnchors)
    {
        for (const auto kind :
             {AtlasKind::OneByOne,
              AtlasKind::TwoByTwo,
              AtlasKind::ThreeByThree})
        {
            const auto spec = antwika::game::atlasSpec(kTestSpecs, kind);
            const auto guides = guidesForTile(
                TileGrid{
                    .width = spec.spriteSize.width,
                    .height = spec.spriteSize.height});

            ASSERT_TRUE(guides.has_value());
            EXPECT_EQ(guides->pivot.x, spec.pivot.x);
            EXPECT_EQ(guides->pivot.y, spec.pivot.y);
        }
    }

    TEST(SpriteGuidesTest, GuidesForTile_StartsAtTheSameHeadroom)
    {
        for (const auto kind :
             {AtlasKind::OneByOne,
              AtlasKind::TwoByTwo,
              AtlasKind::ThreeByThree})
        {
            const auto sprite =
                antwika::game::atlasSpec(kTestSpecs, kind).spriteSize;
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

    TEST(SpriteGuidesTest, GuidesForTile_MakesADiamondOfWholeCells)
    {
        const auto cell = antwika::game::kIsoTileSize;

        for (const auto kind :
             {AtlasKind::OneByOne,
              AtlasKind::TwoByTwo,
              AtlasKind::ThreeByThree})
        {
            const auto sprite =
                antwika::game::atlasSpec(kTestSpecs, kind).spriteSize;
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

    TEST(SpriteGuidesTest, GuidesForTile_GivesNoneToANarrowSlot)
    {
        EXPECT_FALSE(
            guidesForTile(TileGrid{.width = 32, .height = 96})
                .has_value());
        EXPECT_FALSE(
            guidesForTile(TileGrid{.width = 8, .height = 96})
                .has_value());
    }

    TEST(SpriteGuidesTest, GuidesForTile_GivesNoneToAShortSlot)
    {
        EXPECT_FALSE(
            guidesForTile(TileGrid{.width = 64, .height = 80})
                .has_value());
        EXPECT_FALSE(
            guidesForTile(TileGrid{.width = 64, .height = 8})
                .has_value());
    }

    TEST(SpriteGuidesTest, GuidesForTile_GivesNoneWithoutADiamond)
    {
        EXPECT_FALSE(
            guidesForTile(TileGrid{.width = 64, .height = 112})
                .has_value());
    }

    TEST(SpriteGuidesTest, GuidesForTile_GivesSomeToTheDefaultSlot)
    {
        EXPECT_TRUE(guidesForTile(TileGrid{}).has_value());
    }

    TEST(SpriteGuidesTest, OperatorEquals_ComparesGuidesByTheirNumbers)
    {
        constexpr SpriteGuides guides{
            .pivot = {.x = 32, .y = 64},
            .footprint = {.width = 32, .height = 16}};

        constexpr SpriteGuides wider{
            .pivot = {.x = 32, .y = 64},
            .footprint = {.width = 64, .height = 32}};
        constexpr SpriteGuides lower{
            .pivot = {.x = 48, .y = 80},
            .footprint = {.width = 32, .height = 16}};

        const auto twin = guides;
        EXPECT_EQ(guides, twin);
        EXPECT_NE(guides, wider);
        EXPECT_NE(guides, lower);
    }
}
