#include <gtest/gtest.h>

#include "antwika/atlas/AtlasMeta.hpp"

using antwika::atlas::AtlasKind;
using antwika::atlas::AtlasMeta;
using antwika::atlas::counted;
using antwika::atlas::sheetSizeOf;
using antwika::gfx::Point;
using antwika::gfx::Size;

namespace
{
    AtlasMeta described()
    {
        return AtlasMeta{
            .kind = AtlasKind::Isometric,
            .columns = 8,
            .rows = 8,
            .sprite = {.width = 64, .height = 96},
            .pivot = {.x = 32, .y = 64},
            .isometric = {.width = 32, .height = 16}};
    }
}

TEST(AtlasMetaTest, SheetSizeOf_SpansEveryColumnAndRowOfSlots)
{
    EXPECT_EQ(
        sheetSizeOf(described()),
        (Size{.width = 512, .height = 768}));
}

TEST(AtlasMetaTest, Counted_TakesTheCountsFromTheSheetInHand)
{
    auto meta = described();
    meta.columns = 99;
    meta.rows = 99;

    const auto recounted =
        counted(meta, Size{.width = 128, .height = 96});

    EXPECT_EQ(recounted.columns, 2U);
    EXPECT_EQ(recounted.rows, 1U);
    EXPECT_EQ(recounted.pivot, meta.pivot);
}

TEST(AtlasMetaTest, Counted_CountsNoSlotsForASlotWithoutExtent)
{
    auto meta = described();
    meta.sprite = Size{};

    const auto recounted =
        counted(meta, Size{.width = 128, .height = 96});

    EXPECT_EQ(recounted.columns, 0U);
    EXPECT_EQ(recounted.rows, 0U);
}

TEST(AtlasMetaTest, OperatorEquals_ComparesEveryFactAnAtlasCarries)
{
    const auto meta = described();

    EXPECT_EQ(meta, described());

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
