#include <gtest/gtest.h>

#include <antwika/geometry/Size.hpp>

using antwika::geometry::Size;

TEST(SizeTest, Size_DefaultsToNoExtentAtAll)
{
    constexpr Size size{};

    EXPECT_EQ(size.width, 0U);
    EXPECT_EQ(size.height, 0U);
}

TEST(SizeTest, OperatorEquals_SeparatesSizesDifferingInEitherExtent)
{
    constexpr Size size{.width = 3, .height = 4};

    EXPECT_EQ(size, (Size{.width = 3, .height = 4}));
    EXPECT_NE(size, (Size{.width = 4, .height = 4}));
    EXPECT_NE(size, (Size{.width = 3, .height = 5}));
}
