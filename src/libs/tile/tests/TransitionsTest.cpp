#include <gtest/gtest.h>

#include <cstddef>
#include <vector>

#include <antwika/tilemap/AtlasLayout.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/voxelmap/Voxel.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/tilemap/TileEdges.hpp>

#include <antwika/tile/Transitions.hpp>

namespace
{

    using antwika::tilemap::Atlas;
    using antwika::tilemap::getDefaultTilemap;
    using antwika::tile::getCompositedAtlas;
    using antwika::voxel::EdgeKind;
    using antwika::tile::getFirstUnusedTile;
    using antwika::tile::getMaskEdgeBits;
    using antwika::voxel::Side;
    using antwika::tilemap::Tile;
    using antwika::tilemap::TileEdge;
    using antwika::tile::TileRules;
    using antwika::tilemap::getTileSource;
    using antwika::tile::TransitionTile;
    using antwika::tile::getRulesWithTransitions;

    constexpr antwika::gfx::Color kFirstColor{
        .red = 10, .green = 20, .blue = 30, .alpha = 255};

    constexpr Tile kGrassTile{.atlas = Atlas::Floor, .index = 1};

    constexpr Tile kDirtTile{.atlas = Atlas::Floor, .index = 2};

    constexpr Tile kMaskTile{.atlas = Atlas::Floor, .index = 8};

    constexpr Tile kSlotTile{.atlas = Atlas::Floor, .index = 9};

    [[nodiscard]] antwika::gfx::Bitmap sheetOf(const Atlas atlas)
    {
        const auto size = antwika::tilemap::getAtlasSize(
            antwika::tilemap::tileSizeOf(atlas));

        antwika::gfx::Bitmap bitmap{
            .size = size,
            .pixels = std::vector<std::uint8_t>(
                static_cast<std::size_t>(size.width)
                    * size.height
                    * antwika::gfx::kBytesPerPixel,
                0)};

        return bitmap;
    }

    void inkPixel(
        antwika::gfx::Bitmap &sheetBitmap,
        const Tile tile,
        const std::size_t x,
        const std::size_t y,
        const antwika::gfx::Color color)
    {
        const auto place = getTileSource(tile);
        const auto byteIndex =
            (((static_cast<std::size_t>(place.originPoint.y) + y)
              * sheetBitmap.size.width)
             + static_cast<std::size_t>(place.originPoint.x) + x)
            * antwika::gfx::kBytesPerPixel;

        sheetBitmap.pixels.at(byteIndex) = color.red;
        sheetBitmap.pixels.at(byteIndex + 1) = color.green;
        sheetBitmap.pixels.at(byteIndex + 2) = color.blue;
        sheetBitmap.pixels.at(byteIndex + 3) = color.alpha;
    }

    void inkTile(
        antwika::gfx::Bitmap &sheetBitmap,
        const Tile tile,
        const antwika::gfx::Color color)
    {
        const auto place = getTileSource(tile);

        for (std::size_t y = 0;
             y < static_cast<std::size_t>(place.size.height);
             ++y)
        {
            for (std::size_t x = 0;
                 x < static_cast<std::size_t>(
                     place.size.width);
                 ++x)
            {
                inkPixel(sheetBitmap, tile, x, y, color);
            }
        }
    }

    [[nodiscard]] antwika::gfx::Bitmap getHalvedSheet()
    {
        auto sheet = sheetOf(Atlas::Floor);
        const auto place = getTileSource(kMaskTile);

        for (std::size_t y = 0;
             y < static_cast<std::size_t>(place.size.height);
             ++y)
        {
            for (std::size_t x = static_cast<std::size_t>(
                     place.size.width)
                     / 2;
                 x < static_cast<std::size_t>(
                     place.size.width);
                 ++x)
            {
                inkPixel(
                    sheet,
                    kMaskTile,
                    x,
                    y,
                    antwika::gfx::Color{
                        .red = 200,
                        .green = 0,
                        .blue = 0,
                        .alpha = 255});
            }
        }

        return sheet;
    }

    constexpr TransitionTile kHeldTile{
        .fromTile = kGrassTile,
        .toTile = kDirtTile,
        .maskTile = kMaskTile,
        .outputTile = kSlotTile};

    TEST(TransitionsTest, MaskEdgeBits_ReadsAClearBorderAsTheFirst)
    {
        const auto sheet = sheetOf(Atlas::Floor);
        const auto border =
            getMaskEdgeBits(sheet, kMaskTile, Side::Top, kFirstColor);

        for (const auto bit : border)
        {
            EXPECT_FALSE(bit);
        }
    }

    TEST(TransitionsTest, MaskEdgeBits_ReadsTheFirstInkAsTheFirst)
    {
        auto sheet = sheetOf(Atlas::Floor);

        inkTile(sheet, kMaskTile, kFirstColor);

        const auto border =
            getMaskEdgeBits(sheet, kMaskTile, Side::Left, kFirstColor);

        for (const auto bit : border)
        {
            EXPECT_FALSE(bit);
        }
    }

    TEST(TransitionsTest, MaskEdgeBits_ReadsAHalvedMaskApart)
    {
        const auto sheet = getHalvedSheet();

        for (const auto bit :
             getMaskEdgeBits(sheet, kMaskTile, Side::Left, kFirstColor))
        {
            EXPECT_FALSE(bit);
        }

        for (const auto bit :
             getMaskEdgeBits(sheet, kMaskTile, Side::Right, kFirstColor))
        {
            EXPECT_TRUE(bit);
        }

        const auto top =
            getMaskEdgeBits(sheet, kMaskTile, Side::Top, kFirstColor);

        EXPECT_FALSE(top.front());
        EXPECT_TRUE(top.back());
    }

    TEST(TransitionsTest, CompositedAtlas_WeavesBothMaterialsIn)
    {
        auto sheet = getHalvedSheet();
        constexpr antwika::gfx::Color kGreenColor{
            .red = 0, .green = 200, .blue = 0, .alpha = 255};
        constexpr antwika::gfx::Color kBrownColor{
            .red = 120, .green = 80, .blue = 0, .alpha = 255};

        inkTile(sheet, kGrassTile, kGreenColor);
        inkTile(sheet, kDirtTile, kBrownColor);

        const auto woven = getCompositedAtlas(
            sheet,
            Atlas::Floor,
            std::vector<TransitionTile>{kHeldTile},
            std::vector<antwika::gfx::Color>{kFirstColor});
        const auto place = getTileSource(kSlotTile);
        const auto left =
            (((static_cast<std::size_t>(place.originPoint.y))
              * woven.size.width)
             + static_cast<std::size_t>(place.originPoint.x))
            * antwika::gfx::kBytesPerPixel;
        const auto right =
            left
            + ((static_cast<std::size_t>(place.size.width)
                - 1)
               * antwika::gfx::kBytesPerPixel);

        EXPECT_EQ(woven.pixels.at(left + 1), kGreenColor.green);
        EXPECT_EQ(woven.pixels.at(right), kBrownColor.red);
        EXPECT_EQ(
            getCompositedAtlas(
                woven,
                Atlas::Floor,
                std::vector<TransitionTile>{kHeldTile},
                std::vector<antwika::gfx::Color>{kFirstColor}),
            woven);
    }

    TEST(TransitionsTest, CompositedAtlas_LeavesTheRestAlone)
    {
        auto sheet = getHalvedSheet();
        auto woven = getCompositedAtlas(
            sheet,
            Atlas::Floor,
            std::vector<TransitionTile>{kHeldTile},
            std::vector<antwika::gfx::Color>{kFirstColor});

        inkTile(woven, kSlotTile, antwika::gfx::Color{});
        inkTile(sheet, kSlotTile, antwika::gfx::Color{});

        EXPECT_EQ(woven, sheet);
    }

    TEST(TransitionsTest, FirstUnusedTile_FindsTheLowestUnheldTile)
    {
        auto tiles = getDefaultTilemap();

        EXPECT_FALSE(
            getFirstUnusedTile(tiles, Atlas::Floor).has_value());

        antwika::tilemap::clearTile(
            tiles,
            antwika::geometry::GridCell{
                .column = 23, .row = 0});

        const auto tile = getFirstUnusedTile(tiles, Atlas::Floor);

        ASSERT_TRUE(tile.has_value());
        EXPECT_EQ(tile->index, 7);
    }

    TEST(TransitionsTest, RulesWithTransitions_PureEdgesInherit)
    {
        const auto sheet = getHalvedSheet();
        TileRules rules;
        const TileEdge leftEdge{
            .side = Side::Left, .edge = EdgeKind::Boundary};

        rules.allow(kGrassTile, leftEdge, kGrassTile);
        rules.setAllowsBoundary(kGrassTile, leftEdge, true);

        const auto updatedRules = getRulesWithTransitions(
            rules,
            std::vector<TransitionTile>{kHeldTile},
            sheetOf(Atlas::Wall),
            sheet,
            std::vector<antwika::gfx::Color>{kFirstColor});

        EXPECT_TRUE(updatedRules.allows(kSlotTile, leftEdge, kGrassTile));
        EXPECT_TRUE(
            updatedRules.allows(kSlotTile, leftEdge, kSlotTile) == false);
        EXPECT_TRUE(updatedRules.allowsBoundary(kSlotTile, leftEdge));
        EXPECT_TRUE(
            updatedRules.allows(
                kGrassTile,
                antwika::voxel::getFacing(leftEdge),
                kSlotTile));
        EXPECT_EQ(rules.getSize(), 1U);
    }

    TEST(TransitionsTest, RulesWithTransitions_MixedEdgesNeverMeetAir)
    {
        const auto sheet = getHalvedSheet();
        const auto updatedRules = getRulesWithTransitions(
            TileRules{},
            std::vector<TransitionTile>{kHeldTile},
            sheetOf(Atlas::Wall),
            sheet,
            std::vector<antwika::gfx::Color>{kFirstColor});
        const TileEdge topEdge{
            .side = Side::Top, .edge = EdgeKind::Boundary};

        EXPECT_FALSE(updatedRules.allowsBoundary(kSlotTile, topEdge));
        EXPECT_FALSE(
            updatedRules.hasNoRuleFor(kSlotTile, topEdge, Atlas::Floor));
    }

    TEST(TransitionsTest, RulesWithTransitions_MixedEdgesPairAcross)
    {
        const auto sheet = getHalvedSheet();
        const auto updatedRules = getRulesWithTransitions(
            TileRules{},
            std::vector<TransitionTile>{kHeldTile},
            sheetOf(Atlas::Wall),
            sheet,
            std::vector<antwika::gfx::Color>{kFirstColor});
        const TileEdge topEdge{
            .side = Side::Top, .edge = EdgeKind::Boundary};
        const TileEdge bottomEdge{
            .side = Side::Bottom, .edge = EdgeKind::Boundary};

        EXPECT_TRUE(updatedRules.allows(kSlotTile, topEdge, kSlotTile));
        EXPECT_TRUE(updatedRules.allows(kSlotTile, bottomEdge, kSlotTile));
    }

}
