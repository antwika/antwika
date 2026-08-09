#include <gtest/gtest.h>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/enums/NameTable.hpp>

#include "Colour.hpp"

using antwika::enums::kCount;
using antwika::enums::NameTable;
using antwika::enums::tests::Colour;

namespace
{
    constexpr NameTable<Colour> kColours{{"red", "green", "blue"}};
}

TEST(NameTableTest, Name_AnswersTheNameAtTheEnumeratorsIndex)
{
    EXPECT_EQ(kColours.name(Colour::Blue), "blue");
}

TEST(NameTableTest, Name_WrapsAnEnumeratorPastTheBound)
{
    EXPECT_EQ(kColours.name(static_cast<Colour>(kCount<Colour>)), "red");
}

TEST(NameTableTest, From_AnswersTheEnumeratorTheNameSitsAt)
{
    EXPECT_EQ(kColours.from("green"), Colour::Green);
}

TEST(NameTableTest, From_AnswersNothingForANameTheTableLacks)
{
    EXPECT_FALSE(kColours.from("puce").has_value());
}
