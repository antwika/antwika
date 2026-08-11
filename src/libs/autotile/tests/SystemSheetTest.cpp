#include <gtest/gtest.h>

#include <antwika/autotile/Metrics.hpp>
#include <antwika/autotile/SystemSheet.hpp>
#include <antwika/autotile/TileDraw.hpp>
#include <antwika/geometry/Rect.hpp>

using antwika::autotile::DrawKind;
using antwika::autotile::kHalfTile;
using antwika::autotile::systemSource;
using antwika::geometry::Rect;

namespace
{
    Rect slotAt(const std::int32_t column)
    {
        return Rect{
            .origin = {.x = column, .y = 0},
            .size = {.width = kHalfTile, .height = kHalfTile}};
    }
}

TEST(SystemSheetTest, SystemSource_LaysTheFourPiecesOutLeftToRight)
{
    EXPECT_EQ(systemSource(DrawKind::WallBand), slotAt(0));
    EXPECT_EQ(systemSource(DrawKind::WallRim), slotAt(kHalfTile));
    EXPECT_EQ(systemSource(DrawKind::BridgeDeck), slotAt(2 * kHalfTile));
    EXPECT_EQ(systemSource(DrawKind::Shade), slotAt(3 * kHalfTile));
}

TEST(SystemSheetTest, SystemSource_KeepsEveryPieceEightPixelsSquare)
{
    for (const auto kind : {
             DrawKind::WallBand,
             DrawKind::WallRim,
             DrawKind::BridgeDeck,
             DrawKind::Shade})
    {
        const auto source = systemSource(kind);

        EXPECT_EQ(source.size.width, kHalfTile);
        EXPECT_EQ(source.size.height, kHalfTile);
        EXPECT_EQ(source.origin.y, 0);
    }
}

TEST(SystemSheetTest, SystemSource_YieldsTheFirstSlotForASpriteKind)
{
    EXPECT_EQ(systemSource(DrawKind::Sprite), slotAt(0));
}
