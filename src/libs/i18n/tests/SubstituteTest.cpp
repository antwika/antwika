#include <gtest/gtest.h>

#include <array>
#include <span>
#include <string_view>

#include <antwika/i18n/Substitute.hpp>

namespace
{

    using antwika::i18n::substitute;

    constexpr std::array<std::string_view, 2> kTwoArgs{"first", "second"};

    constexpr std::array<std::string_view, 12> kTwelveArgs{
        "a0", "a1", "a2", "a3", "a4", "a5",
        "a6", "a7", "a8", "a9", "a10", "a11"};

    constexpr std::array<std::string_view, 60> kManyArgs{};

    std::span<const std::string_view> noArgs()
    {
        return {};
    }

    TEST(SubstituteTest, Substitute_LeavesTextWithoutPlaceholdersAlone)
    {
        EXPECT_EQ(substitute("reset view", kTwoArgs), "reset view");
        EXPECT_EQ(substitute("", kTwoArgs), "");
    }

    TEST(SubstituteTest, Substitute_ReplacesAPlaceholderWithItsArgument)
    {
        EXPECT_EQ(substitute("zoom {0}", kTwoArgs), "zoom first");
    }

    TEST(SubstituteTest, Substitute_ReplacesEveryPlaceholderInOrder)
    {
        EXPECT_EQ(
            substitute("{0} then {1}.", kTwoArgs), "first then second.");
    }

    TEST(SubstituteTest, Substitute_HonoursThePlaceholderOrderGiven)
    {
        EXPECT_EQ(substitute("{1} before {0}", kTwoArgs),
            "second before first");
    }

    TEST(SubstituteTest, Substitute_RepeatsAnArgumentUsedTwice)
    {
        EXPECT_EQ(substitute("{0}{0}", kTwoArgs), "firstfirst");
    }

    TEST(SubstituteTest, Substitute_KeepsAPlaceholderNobodySuppliedAnArgFor)
    {
        EXPECT_EQ(substitute("zoom {5}", kTwoArgs), "zoom {5}");
        EXPECT_EQ(substitute("zoom {0}", noArgs()), "zoom {0}");
    }

    TEST(SubstituteTest, Substitute_KeepsBracesThatNameNoIndex)
    {
        EXPECT_EQ(substitute("{}", kTwoArgs), "{}");
        EXPECT_EQ(substitute("{a}", kTwoArgs), "{a}");
        EXPECT_EQ(substitute("{0a}", kTwoArgs), "{0a}");
        EXPECT_EQ(substitute("{ 0}", kTwoArgs), "{ 0}");
    }

    TEST(SubstituteTest, Substitute_ReplacesAnIndexOfMoreThanOneDigit)
    {
        EXPECT_EQ(substitute("{10}", kTwelveArgs), "a10");
    }

    TEST(SubstituteTest, Substitute_CountsNineAmongTheDigitsAnIndexMayUse)
    {
        EXPECT_EQ(substitute("{9}", kTwelveArgs), "a9");
    }

    TEST(SubstituteTest, Substitute_KeepsALetterBodyHoweverManyArgsExist)
    {
        EXPECT_EQ(substitute("{a}", kManyArgs), "{a}");
    }

    TEST(SubstituteTest, Substitute_ReplacesAnIndexAtTheLongestLengthAllowed)
    {
        EXPECT_EQ(substitute("{0000}", kTwelveArgs), "a0");
    }

    TEST(SubstituteTest, Substitute_KeepsAnIndexTooLongToBeOne)
    {
        EXPECT_EQ(substitute("{00000}", kTwoArgs), "{00000}");
    }

    TEST(SubstituteTest, Substitute_KeepsAnUnclosedBrace)
    {
        EXPECT_EQ(substitute("zoom {0", kTwoArgs), "zoom {0");
        EXPECT_EQ(substitute("{", kTwoArgs), "{");
    }

    TEST(SubstituteTest, Substitute_ResumesAfterABraceItKept)
    {
        EXPECT_EQ(substitute("{x} {0}", kTwoArgs), "{x} first");
    }

}
