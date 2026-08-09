#include <gtest/gtest.h>

#include <algorithm>
#include <string>

#include "antwika/ecs_commons/Name.hpp"

using antwika::ecs_commons::kNameMaxLength;
using antwika::ecs_commons::makeName;
using antwika::ecs_commons::Name;
using antwika::ecs_commons::view;

TEST(NameTest, Name_HoldsThirtyOneCharactersAndNoTerminator)
{
    EXPECT_EQ(kNameMaxLength, 31U);
    EXPECT_EQ(Name{}.text.size(), 31U);
    EXPECT_EQ(sizeof(Name), 31U);
}

TEST(NameTest, MakeName_RoundTripsShortText)
{
    EXPECT_EQ(view(makeName("worker-0")), "worker-0");
}

TEST(NameTest, View_ReadsADefaultNameAsEmpty)
{
    EXPECT_EQ(view(Name{}), "");
}

TEST(NameTest, MakeName_TruncatesOverlongText)
{
    const std::string tooLong(kNameMaxLength + 5, 'x');

    EXPECT_EQ(view(makeName(tooLong)), std::string(kNameMaxLength, 'x'));
}

TEST(NameTest, MakeName_KeepsEveryCharacterOfAnExactFit)
{
    const std::string exact(kNameMaxLength, 'y');

    EXPECT_EQ(view(makeName(exact)), exact);
}

TEST(NameTest, MakeName_LeavesNoTerminatorOnAnExactFit)
{
    const Name full = makeName(std::string(kNameMaxLength, 'z'));

    EXPECT_EQ(full.text.size(), kNameMaxLength);
    EXPECT_EQ(
        std::find(full.text.begin(), full.text.end(), '\0'),
        full.text.end());
}

TEST(NameTest, OperatorEquals_ComparesTheWholeBuffer)
{
    const std::string upToTheLast(kNameMaxLength - 1, 'w');

    EXPECT_EQ(makeName("a"), makeName("a"));
    EXPECT_NE(makeName("a"), makeName("b"));

    EXPECT_EQ(makeName(upToTheLast + "a"), makeName(upToTheLast + "a"));
    EXPECT_NE(makeName(upToTheLast + "a"), makeName(upToTheLast + "b"));
}

TEST(NameTest, MakeName_IsUsableAtCompileTime)
{
    static constexpr Name kName = makeName("static");
    static_assert(view(kName) == "static");

    SUCCEED();
}
