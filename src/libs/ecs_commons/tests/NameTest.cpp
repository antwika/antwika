#include <string>

#include <gtest/gtest.h>

#include "antwika/ecs_commons/Name.hpp"

using antwika::ecs_commons::kNameMaxLength;
using antwika::ecs_commons::makeName;
using antwika::ecs_commons::Name;
using antwika::ecs_commons::view;

TEST(NameTest, RoundTripsTextShorterThanTheBuffer)
{
    EXPECT_EQ(view(makeName("worker-0")), "worker-0");
}

TEST(NameTest, AnEmptyNameReadsBackAsEmpty)
{
    EXPECT_EQ(view(Name{}), "");
}

TEST(NameTest, TruncatesTextLongerThanTheBuffer)
{
    const std::string tooLong(kNameMaxLength + 5, 'x');

    EXPECT_EQ(view(makeName(tooLong)), std::string(kNameMaxLength, 'x'));
}

TEST(NameTest, TextThatExactlyFillsTheBufferKeepsEveryCharacter)
{
    const std::string exact(kNameMaxLength, 'y');

    EXPECT_EQ(view(makeName(exact)), exact);
}

TEST(NameTest, ComparesByTheWholeBuffer)
{
    EXPECT_EQ(makeName("a"), makeName("a"));
    EXPECT_NE(makeName("a"), makeName("b"));
}

TEST(NameTest, IsUsableAtCompileTime)
{
    static constexpr Name kName = makeName("static");
    static_assert(view(kName) == "static");

    SUCCEED();
}
