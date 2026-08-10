#include <gtest/gtest.h>

#include <antwika/geometry/Size.hpp>
#include <antwika/geometry/SizeF.hpp>

using antwika::geometry::Size;
using antwika::geometry::SizeF;

TEST(SizeFTest, Default_IsEmpty)
{
    constexpr SizeF size;

    EXPECT_FLOAT_EQ(size.width, 0.0F);
    EXPECT_FLOAT_EQ(size.height, 0.0F);
}

TEST(SizeFTest, Extents_AreKeptAsGiven)
{
    constexpr SizeF size{2.5F, 4.75F};

    EXPECT_FLOAT_EQ(size.width, 2.5F);
    EXPECT_FLOAT_EQ(size.height, 4.75F);
}

TEST(SizeFTest, AnIntegerSize_WidensWithoutLosingItsExtents)
{
    constexpr SizeF size = Size{.width = 16, .height = 9};

    EXPECT_FLOAT_EQ(size.width, 16.0F);
    EXPECT_FLOAT_EQ(size.height, 9.0F);
}

TEST(SizeFTest, Equality_ComparesBothExtents)
{
    constexpr SizeF size{1.0F, 2.0F};

    EXPECT_EQ(size, (SizeF{1.0F, 2.0F}));
    EXPECT_NE(size, (SizeF{1.0F, 3.0F}));
    EXPECT_NE(size, (SizeF{3.0F, 2.0F}));
}
