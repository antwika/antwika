#include <gtest/gtest.h>

#include <antwika/pattern/ParamId.hpp>
#include <antwika/pattern/ParamValue.hpp>

#include "antwika/notation/NumberWords.hpp"
#include "antwika/notation/NotationError.hpp"

using antwika::notation::NotationError;
using antwika::notation::NumberWords;
using antwika::pattern::ParamId;
using antwika::pattern::ParamValue;

namespace
{
    constexpr ParamId kName{7};
}

TEST(NumberWordsTest, Read_TakesAWholeNumberUnderItsName)
{
    const NumberWords words(kName);

    const auto read = words.read("42", 0);

    EXPECT_EQ(read.size(), 1U);
    EXPECT_EQ(read.get(kName), ParamValue(42));
}

TEST(NumberWordsTest, Read_TakesANegativeNumber)
{
    const NumberWords words(kName);

    EXPECT_EQ(words.read("-3", 0).get(kName), ParamValue(-3));
}

TEST(NumberWordsTest, Read_TakesAFraction)
{
    const NumberWords words(kName);

    EXPECT_EQ(words.read("3%2", 0).get(kName), ParamValue(3, 2));
}

TEST(NumberWordsTest, Read_RefusesAWordThatIsNotANumber)
{
    const NumberWords words(kName);

    EXPECT_THROW((void)words.read("bd", 0), NotationError);
    EXPECT_THROW((void)words.read("1x", 0), NotationError);
    EXPECT_THROW((void)words.read("1%x", 0), NotationError);
}

TEST(NumberWordsTest, Read_RefusesAHalfWrittenFraction)
{
    const NumberWords words(kName);

    EXPECT_THROW((void)words.read("3%", 0), NotationError);
    EXPECT_THROW((void)words.read("%2", 0), NotationError);
}
