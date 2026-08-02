#include "antwika/notation/NumberWords.hpp"

#include <gtest/gtest.h>

#include <antwika/pattern/ParamId.hpp>
#include <antwika/pattern/ParamValue.hpp>

#include "antwika/notation/NotationError.hpp"

using antwika::notation::NotationError;
using antwika::notation::NumberWords;
using antwika::pattern::ParamId;
using antwika::pattern::ParamValue;

namespace
{
    constexpr ParamId kName{7};
} // namespace

TEST(NumberWordsTest, ReadsAWholeNumberUnderItsOwnName)
{
    const NumberWords words(kName);

    const auto read = words.read("42");

    EXPECT_EQ(read.size(), 1U);
    EXPECT_EQ(read.get(kName), ParamValue(42));
}

TEST(NumberWordsTest, ReadsANegativeNumber)
{
    const NumberWords words(kName);

    EXPECT_EQ(words.read("-3").get(kName), ParamValue(-3));
}

// A control that is not a whole number, without leaving exactness.
TEST(NumberWordsTest, ReadsAFraction)
{
    const NumberWords words(kName);

    EXPECT_EQ(words.read("3%2").get(kName), ParamValue(3, 2));
}

TEST(NumberWordsTest, RefusesAWordThatIsNotANumber)
{
    const NumberWords words(kName);

    EXPECT_THROW((void)words.read("bd"), NotationError);
    EXPECT_THROW((void)words.read("1x"), NotationError);
    EXPECT_THROW((void)words.read("1%x"), NotationError);
}

TEST(NumberWordsTest, RefusesAHalfWrittenFraction)
{
    const NumberWords words(kName);

    EXPECT_THROW((void)words.read("3%"), NotationError);
    EXPECT_THROW((void)words.read("%2"), NotationError);
}
