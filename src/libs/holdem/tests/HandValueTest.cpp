#include <gtest/gtest.h>

#include <antwika/holdem/HandCategory.hpp>
#include <antwika/holdem/HandValue.hpp>

using antwika::holdem::categoryOf;
using antwika::holdem::HandCategory;
using antwika::holdem::HandValue;
using antwika::holdem::kCategoryShift;
using antwika::holdem::rawValue;

TEST(HandValueTest, CategoryOf_ReadsTheCategoryBackOutOfTheValue)
{
    const auto value = static_cast<HandValue>(
        (static_cast<std::uint32_t>(HandCategory::FullHouse)
         << kCategoryShift)
        | 0xABCDEU);
    EXPECT_EQ(categoryOf(value), HandCategory::FullHouse);
}

TEST(HandValueTest, RawValue_UnwrapsTheBackingInteger)
{
    EXPECT_EQ(rawValue(static_cast<HandValue>(1234U)), 1234U);
}

TEST(HandValueTest, CategoryShift_LeavesRoomForFiveFourBitRankSlots)
{
    EXPECT_EQ(kCategoryShift, 20U);
}
