#include <gtest/gtest.h>

#include "antwika/atlas_editor/AtlasMeta.hpp"
#include "antwika/atlas_editor/SpriteGuides.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"

using antwika::atlas_editor::AtlasKind;
using antwika::atlas_editor::AtlasMeta;
using antwika::atlas_editor::counted;
using antwika::atlas_editor::guidesOf;
using antwika::atlas_editor::metaFor;
using antwika::atlas_editor::sheetSizeOf;
using antwika::atlas_editor::SpriteGuides;
using antwika::atlas_editor::TileGrid;
using antwika::atlas_editor::tilesOf;
using antwika::gfx::Point;
using antwika::gfx::Size;

namespace
{
    constexpr TileGrid kGameSlot{.width = 64, .height = 96};

    constexpr Size kSheet{.width = 512, .height = 768};

    constexpr TileGrid kTinySlot{.width = 16, .height = 8};
}

TEST(AtlasMetaTest, MetaFor_CountsTheSlotsTheSheetIsCutInto)
{
    const auto meta = metaFor(kGameSlot, kSheet);

    EXPECT_EQ(meta.columns, 8U);
    EXPECT_EQ(meta.rows, 8U);
    EXPECT_EQ(meta.sprite, (Size{.width = 64, .height = 96}));
}

TEST(AtlasMetaTest, MetaFor_TakesThePivotAndFootprintTheSlotImplies)
{
    const auto meta = metaFor(kGameSlot, kSheet);

    EXPECT_EQ(meta.kind, AtlasKind::Isometric);
    EXPECT_EQ(meta.pivot, (Point{.x = 32, .y = 64}));
    EXPECT_EQ(meta.isometric, (Size{.width = 32, .height = 16}));
}

TEST(AtlasMetaTest, MetaFor_CallsASlotWithNoRoomForADiamondFlat)
{
    const auto meta = metaFor(kTinySlot, kSheet);

    EXPECT_EQ(meta.kind, AtlasKind::Flat);
    EXPECT_EQ(meta.pivot, (Point{.x = 8, .y = 8}));
    EXPECT_EQ(meta.isometric, Size{});
}

TEST(AtlasMetaTest, MetaFor_CountsNoSlotsForASlotWithoutExtent)
{
    const auto meta = metaFor(TileGrid{.width = 0, .height = 0}, kSheet);

    EXPECT_EQ(meta.columns, 0U);
    EXPECT_EQ(meta.rows, 0U);
}

TEST(AtlasMetaTest, TilesOf_HandsBackTheSlotTheSpriteSizeNames)
{
    const auto meta = metaFor(kGameSlot, kSheet);

    EXPECT_EQ(tilesOf(meta), kGameSlot);
}

TEST(AtlasMetaTest, SheetSizeOf_SpansEveryColumnAndRowOfSlots)
{
    const auto meta = metaFor(kGameSlot, kSheet);

    EXPECT_EQ(sheetSizeOf(meta), kSheet);
}

TEST(AtlasMetaTest, GuidesOf_PlacesThePivotAndFootprintTheAtlasNames)
{
    const auto meta = metaFor(kGameSlot, kSheet);

    EXPECT_EQ(
        guidesOf(meta),
        (SpriteGuides{
            .pivot = {.x = 32, .y = 64},
            .footprint = {.width = 32, .height = 16}}));
}

TEST(AtlasMetaTest, GuidesOf_DrawsNoDiamondOnAFlatAtlas)
{
    auto meta = metaFor(kGameSlot, kSheet);
    meta.kind = AtlasKind::Flat;

    EXPECT_FALSE(guidesOf(meta).has_value());
}

TEST(AtlasMetaTest, GuidesOf_DrawsNoDiamondWithoutAFootprint)
{
    auto meta = metaFor(kGameSlot, kSheet);
    meta.isometric = Size{.width = 32, .height = 0};

    EXPECT_FALSE(guidesOf(meta).has_value());

    meta.isometric = Size{.width = 0, .height = 16};

    EXPECT_FALSE(guidesOf(meta).has_value());
}

TEST(AtlasMetaTest, Counted_TakesTheCountsFromTheSheetInHand)
{
    auto meta = metaFor(kGameSlot, kSheet);
    meta.columns = 99;
    meta.rows = 99;

    const auto recounted =
        counted(meta, Size{.width = 128, .height = 96});

    EXPECT_EQ(recounted.columns, 2U);
    EXPECT_EQ(recounted.rows, 1U);
    EXPECT_EQ(recounted.pivot, meta.pivot);
}

TEST(AtlasMetaTest, OperatorEquals_ComparesEveryFactAnAtlasCarries)
{
    const auto meta = metaFor(kGameSlot, kSheet);

    EXPECT_EQ(meta, metaFor(kGameSlot, kSheet));

    auto turned = meta;
    turned.kind = AtlasKind::Flat;
    EXPECT_NE(meta, turned);

    auto widened = meta;
    widened.columns += 1;
    EXPECT_NE(meta, widened);

    auto taller = meta;
    taller.rows += 1;
    EXPECT_NE(meta, taller);

    auto stretched = meta;
    stretched.sprite.width += 1;
    EXPECT_NE(meta, stretched);

    auto moved = meta;
    moved.pivot.x += 1;
    EXPECT_NE(meta, moved);

    auto squashed = meta;
    squashed.isometric.height += 1;
    EXPECT_NE(meta, squashed);
}
