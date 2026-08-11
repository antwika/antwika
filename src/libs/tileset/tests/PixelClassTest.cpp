#include <gtest/gtest.h>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/tileset/PixelClass.hpp>

using antwika::tileset::PixelClass;
using antwika::tileset::toString;

TEST(PixelClassTest, PixelClass_CountsThreeClasses)
{
    EXPECT_EQ(antwika::enums::kCount<PixelClass>, 3U);
}

TEST(PixelClassTest, PixelClass_StartsBlankSoAZeroedFrameIsEmpty)
{
    EXPECT_EQ(PixelClass{}, PixelClass::Blank);
}

TEST(PixelClassTest, ToString_NamesEveryClass)
{
    EXPECT_EQ(toString(PixelClass::Blank), "blank");
    EXPECT_EQ(toString(PixelClass::Paper), "paper");
    EXPECT_EQ(toString(PixelClass::Ink), "ink");
}

TEST(PixelClassTest, ToString_FallsBackForAValueThatNamesNoClass)
{
    EXPECT_EQ(toString(static_cast<PixelClass>(42)), "unknown");
}
