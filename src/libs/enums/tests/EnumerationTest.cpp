#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <string_view>

#include <antwika/enums/Enumeration.hpp>

#include "Colour.hpp"

using antwika::enums::at;
using antwika::enums::index;
using antwika::enums::kAll;
using antwika::enums::kCount;
using antwika::enums::pick;
using antwika::enums::tests::Colour;
using antwika::enums::tests::Side;

TEST(EnumerationTest, Count_ReachesOnePastTheBound)
{
    EXPECT_EQ(kCount<Colour>, 3U);
    EXPECT_EQ(kCount<Side>, 2U);
}

TEST(EnumerationTest, Index_IsTheUnderlyingValue)
{
    EXPECT_EQ(index(Colour::Red), 0U);
    EXPECT_EQ(index(Colour::Blue), 2U);
}

TEST(EnumerationTest, At_TurnsAnIndexBackIntoItsEnumerator)
{
    EXPECT_EQ(at<Colour>(1), Colour::Green);
}

TEST(EnumerationTest, At_WrapsAnIndexPastTheBound)
{
    EXPECT_EQ(at<Colour>(kCount<Colour>), Colour::Red);
}

TEST(EnumerationTest, All_ListsEveryEnumeratorInItsOwnIndexOrder)
{
    ASSERT_EQ(kAll<Colour>.size(), kCount<Colour>);

    for (std::size_t position = 0; position < kCount<Colour>; ++position)
    {
        EXPECT_EQ(index(kAll<Colour>[position]), position);
    }
}

TEST(EnumerationTest, Pick_ReadsTheEntryTheEnumeratorIndexes)
{
    constexpr std::array<std::string_view, kCount<Colour>> kInks{
        "red", "green", "blue"};

    EXPECT_EQ(pick(kInks, Colour::Green), "green");
}

TEST(EnumerationTest, Pick_WrapsAnEnumeratorPastTheBound)
{
    constexpr std::array<std::string_view, kCount<Colour>> kInks{
        "red", "green", "blue"};

    EXPECT_EQ(pick(kInks, static_cast<Colour>(kCount<Colour>)), "red");
}
