#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include <antwika/notation/fakes/FakeConstantWord.hpp>
#include <antwika/notation/fakes/FakeOffsetWord.hpp>
#include <antwika/pattern/Controls.hpp>
#include <antwika/pattern/Cycle.hpp>
#include <antwika/pattern/Hap.hpp>
#include <antwika/pattern/ParamId.hpp>
#include <antwika/pattern/ParamValue.hpp>
#include <antwika/pattern/PatternError.hpp>
#include <antwika/pattern/Span.hpp>

#include "antwika/notation/ParsePattern.hpp"
#include "antwika/notation/NotationError.hpp"
#include "antwika/notation/IWordReader.hpp"
#include "antwika/notation/NumberWords.hpp"

using antwika::notation::NotationError;
using antwika::notation::NumberWords;
using antwika::notation::fakes::FakeConstantWord;
using antwika::notation::fakes::FakeOffsetWord;
using antwika::notation::parsePattern;
using antwika::pattern::Controls;
using antwika::pattern::Cycle;
using antwika::pattern::Hap;
using antwika::pattern::ParamId;
using antwika::pattern::ParamValue;
using antwika::pattern::PatternError;
using antwika::pattern::Span;

namespace
{
    constexpr ParamId kName{1};

    const NumberWords kWords{kName};

    [[nodiscard]] Span cycles(std::int64_t from, std::int64_t to)
    {
        return Span(Cycle(from), Cycle(to));
    }

    [[nodiscard]] std::vector<Hap> read(
        std::string_view source, std::int64_t from = 0,
        std::int64_t to = 1)
    {
        return parsePattern(source, kWords).queryAll(cycles(from, to));
    }

    [[nodiscard]] std::int64_t valueOf(const Hap &hap)
    {
        return hap.value.get(kName)->raw() >> 32;
    }
}

TEST(ParsePatternTest, Read_TakesOneWordAsOneEventACycle)
{
    const auto haps = read("3");

    ASSERT_EQ(haps.size(), 1U);
    EXPECT_EQ(valueOf(haps[0]), 3);
    EXPECT_EQ(haps[0].part, cycles(0, 1));
}

TEST(ParsePatternTest, Read_SplitsACycleBetweenASequence)
{
    const auto haps = read("0 3 5");

    ASSERT_EQ(haps.size(), 3U);
    EXPECT_EQ(valueOf(haps[0]), 0);
    EXPECT_EQ(valueOf(haps[1]), 3);
    EXPECT_EQ(valueOf(haps[2]), 5);

    EXPECT_EQ(haps[1].part, Span(Cycle(1, 3), Cycle(2, 3)));
}

TEST(ParsePatternTest, Read_TakesARestAsSilence)
{
    const auto haps = read("0 ~ 5");

    ASSERT_EQ(haps.size(), 2U);
    EXPECT_EQ(haps[0].part, Span(Cycle(), Cycle(1, 3)));
    EXPECT_EQ(haps[1].part, Span(Cycle(2, 3), Cycle(1)));
}

TEST(ParsePatternTest, Read_TakesAGroupAsOneSlot)
{
    const auto haps = read("0 [3 5]");

    ASSERT_EQ(haps.size(), 3U);
    EXPECT_EQ(haps[0].part, Span(Cycle(), Cycle(1, 2)));
    EXPECT_EQ(haps[1].part, Span(Cycle(1, 2), Cycle(3, 4)));
    EXPECT_EQ(haps[2].part, Span(Cycle(3, 4), Cycle(1)));
}

TEST(ParsePatternTest, Read_TakesACommaAsBothAtOnce)
{
    const auto haps = read("[0, 5]");

    ASSERT_EQ(haps.size(), 2U);
    EXPECT_EQ(valueOf(haps[0]), 0);
    EXPECT_EQ(valueOf(haps[1]), 5);
    EXPECT_EQ(haps[0].part, haps[1].part);
}

TEST(ParsePatternTest, Read_TakesATopLevelCommaAsBoth)
{
    const auto haps = read("0, 5");

    ASSERT_EQ(haps.size(), 2U);
    EXPECT_EQ(haps[0].part, cycles(0, 1));
    EXPECT_EQ(haps[1].part, cycles(0, 1));
}

TEST(ParsePatternTest, Read_TakesAngleBracketsAsOnePerCycle)
{
    const auto haps = read("<3 5>", 0, 3);

    ASSERT_EQ(haps.size(), 3U);
    EXPECT_EQ(valueOf(haps[0]), 3);
    EXPECT_EQ(valueOf(haps[1]), 5);
    EXPECT_EQ(valueOf(haps[2]), 3);
}

TEST(ParsePatternTest, Read_TakesAStarAsFaster)
{
    const auto haps = read("3*2");

    ASSERT_EQ(haps.size(), 2U);
    EXPECT_EQ(haps[0].part, Span(Cycle(), Cycle(1, 2)));
    EXPECT_EQ(haps[1].part, Span(Cycle(1, 2), Cycle(1)));
}

TEST(ParsePatternTest, Read_TakesASlashAsSlower)
{
    const auto haps = read("3/2", 0, 2);

    ASSERT_EQ(haps.size(), 1U);
    EXPECT_EQ(haps[0].part, cycles(0, 2));
}

TEST(ParsePatternTest, Read_TakesAPercentAsAnExactRatio)
{
    const auto haps = read("3*3%2", 0, 2);

    ASSERT_EQ(haps.size(), 3U);
    EXPECT_EQ(haps[0].part, Span(Cycle(), Cycle(2, 3)));
    EXPECT_EQ(haps[1].part, Span(Cycle(2, 3), Cycle(4, 3)));
}

TEST(ParsePatternTest, Read_TakesABangAsRepeatedSlots)
{
    const auto haps = read("3!3");

    ASSERT_EQ(haps.size(), 3U);
    EXPECT_EQ(haps[0].part, Span(Cycle(), Cycle(1, 3)));
    EXPECT_EQ(haps[2].part, Span(Cycle(2, 3), Cycle(1)));
}

TEST(ParsePatternTest, Read_TakesBracketsAsEuclidean)
{
    const auto haps = read("3(3,8)");

    ASSERT_EQ(haps.size(), 3U);
    EXPECT_EQ(haps[0].part, Span(Cycle(), Cycle(1, 8)));
    EXPECT_EQ(haps[1].part, Span(Cycle(3, 8), Cycle(1, 2)));
    EXPECT_EQ(haps[2].part, Span(Cycle(3, 4), Cycle(7, 8)));
}

TEST(ParsePatternTest, Read_TakesAQuestionMarkAsDroppingHalf)
{
    const auto thinned = read("3*16?", 0, 8);
    const auto whole = read("3*16", 0, 8);

    ASSERT_EQ(whole.size(), 128U);
    EXPECT_LT(thinned.size(), whole.size());
    EXPECT_GT(thinned.size(), 0U);
}

TEST(ParsePatternTest, Read_SeedsEachQuestionMarkSeparately)
{
    const auto haps = read("[3*16?, 5*16?]", 0, 4);

    std::vector<Span> first;
    std::vector<Span> second;

    for (const auto &hap : haps)
    {
        (valueOf(hap) == 3 ? first : second).push_back(hap.part);
    }

    ASSERT_FALSE(first.empty());
    ASSERT_FALSE(second.empty());
    EXPECT_NE(first, second);
}

TEST(ParsePatternTest, Read_StacksModifiers)
{
    const auto haps = read("3*2!2");

    EXPECT_EQ(haps.size(), 4U);
}

TEST(ParsePatternTest, Read_IgnoresSurroundingSpace)
{
    EXPECT_EQ(read("  0   3  ").size(), 2U);
}

TEST(ParsePatternTest, Read_RefusesAnEmptyPattern)
{
    EXPECT_THROW((void)read(""), NotationError);
    EXPECT_THROW((void)read("   "), NotationError);
}

TEST(ParsePatternTest, Read_RefusesAnUnclosedBracket)
{
    EXPECT_THROW((void)read("[0 3"), NotationError);
    EXPECT_THROW((void)read("<0 3"), NotationError);
    EXPECT_THROW((void)read("0 3]"), NotationError);
    EXPECT_THROW((void)read("3(3,8"), NotationError);
}

TEST(ParsePatternTest, Read_RefusesAnEmptyGroup)
{
    EXPECT_THROW((void)read("[]"), NotationError);
    EXPECT_THROW((void)read("<>"), NotationError);
    EXPECT_THROW((void)read("0 [,]"), NotationError);
}

TEST(ParsePatternTest, Read_RefusesAModifierWithNoNumber)
{
    EXPECT_THROW((void)read("3*"), NotationError);
    EXPECT_THROW((void)read("3!"), NotationError);
    EXPECT_THROW((void)read("3(,8)"), NotationError);
    EXPECT_THROW((void)read("3*3%"), NotationError);
}

TEST(ParsePatternTest, Read_RefusesAnUnknownCharacter)
{
    EXPECT_THROW((void)read("3 @ 5"), NotationError);

    EXPECT_THROW((void)read("3 {"), NotationError);
}

TEST(ParsePatternTest, Read_RefusesNoSlotsAtAll)
{
    EXPECT_THROW((void)read("3!0"), NotationError);
}

TEST(ParsePatternTest, Read_RefusesABareRepeatMark)
{
    EXPECT_THROW((void)read("0! 3"), NotationError);
    EXPECT_THROW((void)read("0!"), NotationError);
    EXPECT_THROW((void)read("0!x"), NotationError);
}

TEST(ParsePatternTest, Read_TakesANineAsARepeatCount)
{
    EXPECT_EQ(read("0!9").size(), 9U);
}

TEST(ParsePatternTest, Read_ReadsNoNumberPastTheEndOfThePattern)
{
    constexpr std::string_view longer{"0!24"};

    EXPECT_EQ(
        parsePattern(longer.substr(0, 3), kWords)
            .queryAll(cycles(0, 1))
            .size(),
        2U);
}

TEST(ParsePatternTest, Read_RefusesASecondRepeatCount)
{
    EXPECT_THROW((void)read("0!2!3"), NotationError);
}

TEST(ParsePatternTest, Read_TakesAWordHoldingAFraction)
{
    const auto haps = read("3%2");

    ASSERT_EQ(haps.size(), 1U);
    EXPECT_EQ(*haps[0].value.get(kName), ParamValue(3, 2));
}

TEST(ParsePatternTest, Read_RefusesACountAboveItsLimit)
{
    EXPECT_THROW((void)read("0!2000000000"), NotationError);
    EXPECT_THROW((void)read("0(3,2000000000)"), NotationError);
    EXPECT_THROW((void)read("0*999999999"), NotationError);
}

TEST(ParsePatternTest, Read_RefusesANumberTooLongToHold)
{
    EXPECT_THROW(
        (void)read("0!99999999999999999999999"), NotationError);
}

TEST(ParsePatternTest, Read_AcceptsACountAtItsLimit)
{
    EXPECT_EQ(read("0!1024").size(), 1024U);
    EXPECT_EQ(read("0*1024").size(), 1024U);
}

TEST(ParsePatternTest, Read_RefusesSpeedsPastTheLimit)
{
    EXPECT_THROW((void)read("0*64*64*64"), NotationError);
    EXPECT_THROW((void)read("[0*64]*64"), NotationError);
    EXPECT_THROW((void)read("<0*64>*64"), NotationError);
    EXPECT_THROW((void)read("[~, 0*64]*64"), NotationError);

    EXPECT_THROW((void)read("0/1%1024/1%2"), NotationError);
}

TEST(ParsePatternTest, Read_LetsEachTermHaveItsOwnSpeed)
{
    EXPECT_EQ(read("0*64 3*64 5*64").size(), 192U);
}

TEST(ParsePatternTest, Read_RefusesEuclidStepsPastTheLimit)
{
    EXPECT_THROW((void)read("0(3,64)(3,64)"), NotationError);
    EXPECT_THROW((void)read("0(3,64)*64"), NotationError);
}

TEST(ParsePatternTest, Read_RefusesNestingPastTheLimit)
{
    EXPECT_THROW((void)read("[[0!64]!64]"), NotationError);
    EXPECT_THROW((void)read("[[[0!16]!16]!16]"), NotationError);
}

TEST(ParsePatternTest, Read_LetsTheAlgebraRefuseACleanParse)
{
    EXPECT_THROW((void)read("3(9,8)"), PatternError);
    EXPECT_THROW((void)read("3*0"), PatternError);
}

TEST(ParsePatternTest, Read_TakesEveryCharacterAWordMayHold)
{
    const FakeConstantWord any{kName};

    const auto haps = parsePattern("AZaz09Bd_x.y#z+w-v 3", any)
                          .queryAll(cycles(0, 1));

    EXPECT_EQ(haps.size(), 2U);
}

TEST(ParsePatternTest, Read_TreatsATabAsSpace)
{
    EXPECT_EQ(read("0\t3").size(), 2U);
}

TEST(ParsePatternTest, Read_RefusesTheWrongClosingBracket)
{
    EXPECT_THROW((void)read("[0 3>"), NotationError);
}

TEST(ParsePatternTest, Read_HandsTheReaderEachWordsOffset)
{
    const FakeOffsetWord reader{kName};

    const auto haps = parsePattern("0 [3 5] <7 9>", reader)
                          .queryAll(cycles(0, 2));

    std::vector<std::int64_t> offsets;
    offsets.reserve(haps.size());

    for (const auto &hap : haps)
    {
        offsets.push_back(valueOf(hap));
    }

    std::ranges::sort(offsets);

    const std::vector<std::int64_t> expected{0, 0, 3, 3, 5, 5, 9, 11};

    EXPECT_EQ(offsets, expected);
}

TEST(ParsePatternTest, Read_HoldsASlotLongerOnATie)
{
    const auto haps = read("0 _ 3");

    ASSERT_EQ(haps.size(), 2U);
    EXPECT_EQ(
        haps[0].whole,
        Span(Cycle(0), Cycle(2, 3)));
    EXPECT_EQ(valueOf(haps[0]), 0);
    EXPECT_EQ(
        haps[1].whole,
        Span(Cycle(2, 3), Cycle(1)));
    EXPECT_EQ(valueOf(haps[1]), 3);
}

TEST(ParsePatternTest, Read_AccumulatesOneSlotPerTie)
{
    const auto haps = read("0 _ _ 3");

    ASSERT_EQ(haps.size(), 2U);
    EXPECT_EQ(haps[0].whole, Span(Cycle(0), Cycle(3, 4)));
    EXPECT_EQ(haps[1].whole, Span(Cycle(3, 4), Cycle(1)));
}

TEST(ParsePatternTest, Read_GivesALoneTieTheWholeSlot)
{
    const auto haps = read("0 _");

    ASSERT_EQ(haps.size(), 1U);
    EXPECT_EQ(haps[0].whole, Span(Cycle(0), Cycle(1)));
}

TEST(ParsePatternTest, Read_AllowsATieInsideABracket)
{
    const auto haps = read("[0 _ 3] 5");

    ASSERT_EQ(haps.size(), 3U);
    EXPECT_EQ(haps[0].whole, Span(Cycle(0), Cycle(1, 3)));
    EXPECT_EQ(haps[1].whole, Span(Cycle(1, 3), Cycle(1, 2)));
    EXPECT_EQ(haps[2].whole, Span(Cycle(1, 2), Cycle(1)));
}

TEST(ParsePatternTest, Read_AllowsATieToHoldARest)
{
    const auto haps = read("~ _ 3");

    ASSERT_EQ(haps.size(), 1U);
    EXPECT_EQ(haps[0].whole, Span(Cycle(2, 3), Cycle(1)));
}

TEST(ParsePatternTest, Read_FragmentsAHeldNoteHonestly)
{
    const auto haps = parsePattern("0 _ 3", kWords)
                          .queryAll(Span(Cycle(1, 3), Cycle(2, 3)));

    ASSERT_EQ(haps.size(), 1U);
    EXPECT_EQ(haps[0].part, Span(Cycle(1, 3), Cycle(2, 3)));
    EXPECT_EQ(haps[0].whole, Span(Cycle(0), Cycle(2, 3)));
    EXPECT_FALSE(haps[0].hasOnset());
}

TEST(ParsePatternTest, Read_TurnsAnAlternationInAHeldSlot)
{
    const auto first = read("<0 7> _ 3", 0, 1);
    const auto second = read("<0 7> _ 3", 1, 2);

    ASSERT_EQ(first.size(), 2U);
    ASSERT_EQ(second.size(), 2U);
    EXPECT_EQ(valueOf(first[0]), 0);
    EXPECT_EQ(valueOf(second[0]), 7);
}

TEST(ParsePatternTest, Read_KeepsAnUnderscoreInsideAWord)
{
    const FakeConstantWord any{kName};

    const auto haps =
        parsePattern("a_b _", any).queryAll(cycles(0, 1));

    ASSERT_EQ(haps.size(), 1U);
    EXPECT_EQ(haps[0].whole, Span(Cycle(0), Cycle(1)));
}

TEST(ParsePatternTest, Read_LooksNoFurtherThanThePatternForATie)
{
    constexpr std::string_view longer{"0 _x"};

    const auto haps = parsePattern(longer.substr(0, 3), kWords)
                          .queryAll(cycles(0, 1));

    ASSERT_EQ(haps.size(), 1U);
    EXPECT_EQ(haps[0].whole, Span(Cycle(0), Cycle(1)));
}

TEST(ParsePatternTest, Read_RefusesATieWithNoSlotBeforeIt)
{
    EXPECT_THROW((void)read("_ 3"), NotationError);
    EXPECT_THROW((void)read("[_ 3]"), NotationError);
    EXPECT_THROW((void)read("0, _"), NotationError);
}

TEST(ParsePatternTest, Read_RefusesATieInsideAnAlternation)
{
    EXPECT_THROW((void)read("<0 _>"), NotationError);
}

TEST(ParsePatternTest, Read_CountsTiesTowardsTheDensityBound)
{
    EXPECT_THROW((void)read("[[[0!16 _]!16]!16]"), NotationError);
}

TEST(ParsePatternTest, Read_AllowsAWordToOpenWithAnUnderscore)
{
    const FakeConstantWord any{kName};

    const auto haps =
        parsePattern("_x 3", any).queryAll(cycles(0, 1));

    ASSERT_EQ(haps.size(), 2U);
    EXPECT_EQ(haps[0].whole, Span(Cycle(0), Cycle(1, 2)));
}
