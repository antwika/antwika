#include <gtest/gtest.h>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/enums/NameTable.hpp>

#include "Color.hpp"

using antwika::enums::kCount;
using antwika::enums::NameTable;
using antwika::enums::tests::Color;

namespace
{
    constexpr NameTable<Color> kColors{{"red", "green", "blue"}};
}

TEST(NameTableTest, Name_AnswersTheNameAtTheEnumeratorsIndex)
{
    EXPECT_EQ(kColors.name(Color::Blue), "blue");
}

TEST(NameTableTest, Name_WrapsAnEnumeratorPastTheBound)
{
    EXPECT_EQ(kColors.name(static_cast<Color>(kCount<Color>)), "red");
}

TEST(NameTableTest, From_AnswersTheEnumeratorTheNameSitsAt)
{
    EXPECT_EQ(kColors.from("green"), Color::Green);
}

TEST(NameTableTest, From_AnswersNothingForANameTheTableLacks)
{
    EXPECT_FALSE(kColors.from("puce").has_value());
}

TEST(NameTableTest, IsComplete_HoldsWhenEveryEnumeratorIsNamed)
{
    static_assert(kColors.isComplete());

    EXPECT_TRUE(kColors.isComplete());
}

TEST(NameTableTest, IsComplete_FailsWhenATailEnumeratorIsUnnamed)
{
    constexpr NameTable<Color> shortColors{{"red", "green"}};

    static_assert(!shortColors.isComplete());

    EXPECT_FALSE(shortColors.isComplete());
}
