#include <gtest/gtest.h>

#include <cstdint>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/ui/HoverPointer.hpp"
#include "antwika/ui/Overlays.hpp"

using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::ui::HoverPointer;
using antwika::ui::overlaid;
using antwika::ui::Overlays;

namespace
{
    [[nodiscard]] Rect boxAt(std::int32_t x)
    {
        return Rect{
            .origin = {.x = x, .y = 0},
            .size = {.width = 10, .height = 10}};
    }

    [[nodiscard]] HoverPointer over(std::int32_t x, std::int32_t y)
    {
        return HoverPointer{.position = Point{.x = x, .y = y}};
    }
}

TEST(OverlaysTest, Overlaid_SaysNoWhenNothingReportsAPosition)
{
    EXPECT_FALSE(overlaid(Overlays{boxAt(0)}, HoverPointer{}));
}

TEST(OverlaysTest, Overlaid_SaysNoWithNoOverlays)
{
    EXPECT_FALSE(overlaid(Overlays{}, over(5, 5)));
}

TEST(OverlaysTest, Overlaid_SaysYesInsideAnOverlay)
{
    EXPECT_TRUE(overlaid(Overlays{boxAt(0)}, over(5, 5)));
}

TEST(OverlaysTest, Overlaid_SaysNoOutsideEveryOverlay)
{
    EXPECT_FALSE(overlaid(Overlays{boxAt(0), boxAt(20)}, over(15, 5)));
}

TEST(OverlaysTest, Overlaid_ReachesPastTheFirstOverlay)
{
    EXPECT_TRUE(overlaid(Overlays{boxAt(0), boxAt(20)}, over(25, 5)));
}

TEST(OverlaysTest, Overlaid_TreatsAnOverlaysAreaAsHalfOpen)
{
    EXPECT_TRUE(overlaid(Overlays{boxAt(0)}, over(9, 9)));
    EXPECT_FALSE(overlaid(Overlays{boxAt(0)}, over(10, 9)));
}
