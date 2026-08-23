#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <vector>

#include <antwika/gfx/Glyphs.hpp>

#include "antwika/ui/TextWrap.hpp"

using antwika::ui::getWrapText;

namespace
{
    constexpr std::uint32_t kAdvance = antwika::gfx::kGlyphAdvance;
}

namespace
{

    [[nodiscard]] std::vector<std::string> linesOf(
        const std::string_view text, const std::size_t columns)
    {
        std::vector<std::string> lines;

        for (const auto line : getWrapText(text, columns))
        {
            lines.emplace_back(line);
        }

        return lines;
    }

}

TEST(TextWrapTest, WrapText_LeavesTextThatAlreadyFitsWhole)
{
    EXPECT_EQ(
        linesOf("short", 10), (std::vector<std::string>{"short"}));
}

TEST(TextWrapTest, WrapText_BreaksAtTheSpacesBetweenWords)
{
    EXPECT_EQ(
        linesOf("one two three four", 9),
        (std::vector<std::string>{"one two", "three", "four"}));
}

TEST(TextWrapTest, WrapText_KeepsNoSpaceItBrokeAt)
{
    for (const auto &line : linesOf("alpha beta gamma", 11))
    {
        EXPECT_NE(line.front(), ' ');
        EXPECT_NE(line.back(), ' ');
    }
}

TEST(TextWrapTest, WrapText_FillsALineRightUpToItsWidth)
{
    EXPECT_EQ(
        linesOf("abcde fg", 5),
        (std::vector<std::string>{"abcde", "fg"}));
}

TEST(TextWrapTest, WrapText_BreaksAWordLongerThanALineAcrossLines)
{
    EXPECT_EQ(
        linesOf("abcdefghij", 4),
        (std::vector<std::string>{"abcd", "efgh", "ij"}));
}

TEST(TextWrapTest, WrapText_BreaksALongWordStandingAfterAShortOne)
{
    EXPECT_EQ(
        linesOf("hi abcdefghij", 4),
        (std::vector<std::string>{"hi", "abcd", "efgh", "ij"}));
}

TEST(TextWrapTest, WrapText_KeepsTheLineBreaksTheTextAlreadyHeld)
{
    EXPECT_EQ(
        linesOf("one\ntwo", 40),
        (std::vector<std::string>{"one", "two"}));
}

TEST(TextWrapTest, WrapText_BreaksEachLineOfTextOnItsOwn)
{
    EXPECT_EQ(
        linesOf("one two\nthree four", 7),
        (std::vector<std::string>{
            "one two", "three", "four"}));
}

TEST(TextWrapTest, WrapText_StandsAnEmptyLineWhereTheTextHeldOne)
{
    EXPECT_EQ(
        linesOf("one\n\ntwo", 8),
        (std::vector<std::string>{"one", "", "two"}));
}

TEST(TextWrapTest, WrapText_TakesTextWithNothingInItAsOneEmptyLine)
{
    EXPECT_EQ(linesOf("", 8), (std::vector<std::string>{""}));
}

TEST(TextWrapTest, WrapText_LeavesTextWholeWhereNoLineHasRoomAtAll)
{
    EXPECT_EQ(
        linesOf("one two", 0),
        (std::vector<std::string>{"one two"}));
}

TEST(TextWrapTest, WrapText_TakesARunOfSpacesAsOneBreak)
{
    EXPECT_EQ(
        linesOf("one    two", 5),
        (std::vector<std::string>{"one", "two"}));
}

TEST(TextWrapTest, WrapText_TakesALineOfNothingButSpacesAsEmpty)
{
    EXPECT_EQ(linesOf("   ", 2), (std::vector<std::string>{""}));
}

TEST(TextWrapTest, WrapText_LeavesTheSpacesLeadingATextBehind)
{
    EXPECT_EQ(
        linesOf("  one two", 3),
        (std::vector<std::string>{"one", "two"}));
}

TEST(TextWrapTest, WrapColumns_CountsWhatFitsOnceThePaddingIsOff)
{
    const antwika::ui::Theme theme{
        .textScale = 1, .buttonPadding = 4};

    EXPECT_EQ(
        antwika::ui::getWrapColumns(theme, 8 + (10 * kAdvance)), 10U);
}

TEST(TextWrapTest, WrapColumns_CountsNothingWhereThePaddingTakesItAll)
{
    const antwika::ui::Theme theme{
        .textScale = 1, .buttonPadding = 40};

    EXPECT_EQ(antwika::ui::getWrapColumns(theme, 20), 0U);
    EXPECT_EQ(antwika::ui::getWrapColumns(theme, 80), 0U);
}

TEST(TextWrapTest, WrapColumns_CountsNothingForAFaceWithNoWidth)
{
    const antwika::ui::Theme theme{
        .textScale = 0, .buttonPadding = 0};

    EXPECT_EQ(antwika::ui::getWrapColumns(theme, 200), 0U);
}
