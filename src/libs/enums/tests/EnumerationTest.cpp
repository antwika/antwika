#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <string_view>

#include <antwika/enums/Enumeration.hpp>

#include "Color.hpp"

using antwika::enums::wrapToEnum;
using antwika::enums::index;
using antwika::enums::kAll;
using antwika::enums::kCount;
using antwika::enums::lookup;
using antwika::enums::tests::Color;
using antwika::enums::tests::Side;

TEST(EnumerationTest, Count_ReachesOnePastTheBound)
{
    EXPECT_EQ(kCount<Color>, 3U);
    EXPECT_EQ(kCount<Side>, 2U);
}

TEST(EnumerationTest, Index_IsTheUnderlyingValue)
{
    EXPECT_EQ(index(Color::Red), 0U);
    EXPECT_EQ(index(Color::Blue), 2U);
}

TEST(EnumerationTest, WrapToEnum_TurnsAnIndexBackIntoItsEnumerator)
{
    EXPECT_EQ(wrapToEnum<Color>(1), Color::Green);
}

TEST(EnumerationTest, WrapToEnum_WrapsAnIndexPastTheBound)
{
    EXPECT_EQ(wrapToEnum<Color>(kCount<Color>), Color::Red);
}

TEST(EnumerationTest, All_ListsEveryEnumeratorInItsOwnIndexOrder)
{
    ASSERT_EQ(kAll<Color>.size(), kCount<Color>);

    for (std::size_t position = 0; position < kCount<Color>; ++position)
    {
        EXPECT_EQ(index(kAll<Color>[position]), position);
    }
}

TEST(EnumerationTest, Lookup_ReadsTheEntryTheEnumeratorIndexes)
{
    constexpr std::array<std::string_view, kCount<Color>> kInks{
        "red", "green", "blue"};

    EXPECT_EQ(lookup(kInks, Color::Green), "green");
}

TEST(EnumerationTest, Lookup_WrapsAnEnumeratorPastTheBound)
{
    constexpr std::array<std::string_view, kCount<Color>> kInks{
        "red", "green", "blue"};

    EXPECT_EQ(lookup(kInks, static_cast<Color>(kCount<Color>)), "red");
}
