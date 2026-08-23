#include <gtest/gtest.h>

#include <cstdint>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/ui/HoverPointer.hpp"
#include "antwika/ui/OccluderRects.hpp"

using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::ui::HoverPointer;
using antwika::ui::isOccluded;
using antwika::ui::OccluderRects;

namespace
{
    [[nodiscard]] Rect boxAt(std::int32_t x)
    {
        return Rect{
            .originPoint = {.x = x, .y = 0},
            .size = {.width = 10, .height = 10}};
    }

    [[nodiscard]] HoverPointer getOver(std::int32_t x, std::int32_t y)
    {
        return HoverPointer{.positionPoint = Point{.x = x, .y = y}};
    }
}

TEST(OccluderRectsTest, IsOccluded_SaysNoWhenNothingReportsAPosition)
{
    EXPECT_FALSE(isOccluded(OccluderRects{boxAt(0)}, HoverPointer{}));
}

TEST(OccluderRectsTest, IsOccluded_SaysNoWithNoOverlays)
{
    EXPECT_FALSE(isOccluded(OccluderRects{}, getOver(5, 5)));
}

TEST(OccluderRectsTest, IsOccluded_SaysYesInsideAnOverlay)
{
    EXPECT_TRUE(isOccluded(OccluderRects{boxAt(0)}, getOver(5, 5)));
}

TEST(OccluderRectsTest, IsOccluded_SaysNoOutsideEveryOverlay)
{
    EXPECT_FALSE(isOccluded(OccluderRects{boxAt(0), boxAt(20)}, getOver(15, 5)));
}

TEST(OccluderRectsTest, IsOccluded_ReachesPastTheFirstOverlay)
{
    EXPECT_TRUE(isOccluded(OccluderRects{boxAt(0), boxAt(20)}, getOver(25, 5)));
}

TEST(OccluderRectsTest, IsOccluded_TreatsAnOverlaysAreaAsHalfOpen)
{
    EXPECT_TRUE(isOccluded(OccluderRects{boxAt(0)}, getOver(9, 9)));
    EXPECT_FALSE(isOccluded(OccluderRects{boxAt(0)}, getOver(10, 9)));
}

TEST(OccluderRectsTest, IsOccluded_SaysNoForARectangleWithNoOverlays)
{
    EXPECT_FALSE(isOccluded(OccluderRects{}, boxAt(0)));
}

TEST(OccluderRectsTest, IsOccluded_SaysYesForARectangleAnOverlayMeets)
{
    EXPECT_TRUE(isOccluded(OccluderRects{boxAt(0)}, boxAt(9)));
}

TEST(OccluderRectsTest, IsOccluded_SaysNoForARectangleBesideAnOverlay)
{
    EXPECT_FALSE(isOccluded(OccluderRects{boxAt(0)}, boxAt(10)));
    EXPECT_FALSE(isOccluded(OccluderRects{boxAt(20)}, boxAt(10)));
}

TEST(OccluderRectsTest, IsOccluded_SaysNoForARectangleUnderAnOverlay)
{
    const Rect belowRect{
        .originPoint = {.x = 0, .y = 10},
        .size = {.width = 10, .height = 10}};

    EXPECT_FALSE(isOccluded(OccluderRects{boxAt(0)}, belowRect));
    EXPECT_FALSE(isOccluded(OccluderRects{belowRect}, boxAt(0)));
}

TEST(OccluderRectsTest, IsOccluded_ReachesPastTheFirstOverlayForARectangle)
{
    EXPECT_TRUE(isOccluded(OccluderRects{boxAt(40), boxAt(20)}, boxAt(25)));
}

TEST(OccluderRectsTest, IsOccluded_SaysNoForARectangleWithNoAreaOfItsOwn)
{
    const Rect emptyRect{
        .originPoint = {.x = 5, .y = 5}, .size = {.width = 0, .height = 0}};

    EXPECT_FALSE(isOccluded(OccluderRects{boxAt(0)}, emptyRect));
}
