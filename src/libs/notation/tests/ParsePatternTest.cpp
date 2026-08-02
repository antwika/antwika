#include "antwika/notation/ParsePattern.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include <antwika/pattern/Controls.hpp>
#include <antwika/pattern/Cycle.hpp>
#include <antwika/pattern/Hap.hpp>
#include <antwika/pattern/ParamId.hpp>
#include <antwika/pattern/ParamValue.hpp>
#include <antwika/pattern/PatternError.hpp>
#include <antwika/pattern/Span.hpp>

#include "antwika/notation/NotationError.hpp"
#include "antwika/notation/IWordReader.hpp"
#include "antwika/notation/NumberWords.hpp"

using antwika::notation::NotationError;
using antwika::notation::NumberWords;
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

    // Accepts any word at all.
    // So the grammar's idea of a word is testable on its own.
    class AnyWord final : public antwika::notation::IWordReader
    {
    public:
        [[nodiscard]] Controls read(
            std::string_view, std::size_t) const override
        {
            return Controls(kName, ParamValue(1));
        }
    };

    [[nodiscard]] std::int64_t valueOf(const Hap &hap)
    {
        return hap.value.get(kName)->raw() >> 32;
    }
} // namespace

TEST(ParsePatternTest, ReadsOneWordAsOneEventACycle)
{
    const auto haps = read("3");

    ASSERT_EQ(haps.size(), 1U);
    EXPECT_EQ(valueOf(haps[0]), 3);
    EXPECT_EQ(haps[0].part, cycles(0, 1));
}

TEST(ParsePatternTest, SplitsACycleBetweenTheWordsOfASequence)
{
    const auto haps = read("0 3 5");

    ASSERT_EQ(haps.size(), 3U);
    EXPECT_EQ(valueOf(haps[0]), 0);
    EXPECT_EQ(valueOf(haps[1]), 3);
    EXPECT_EQ(valueOf(haps[2]), 5);

    EXPECT_EQ(haps[1].part, Span(Cycle(1, 3), Cycle(2, 3)));
}

TEST(ParsePatternTest, ReadsARestAsSilence)
{
    const auto haps = read("0 ~ 5");

    ASSERT_EQ(haps.size(), 2U);
    EXPECT_EQ(haps[0].part, Span(Cycle(), Cycle(1, 3)));
    EXPECT_EQ(haps[1].part, Span(Cycle(2, 3), Cycle(1)));
}

TEST(ParsePatternTest, ReadsAGroupAsOneSlot)
{
    const auto haps = read("0 [3 5]");

    ASSERT_EQ(haps.size(), 3U);
    EXPECT_EQ(haps[0].part, Span(Cycle(), Cycle(1, 2)));
    EXPECT_EQ(haps[1].part, Span(Cycle(1, 2), Cycle(3, 4)));
    EXPECT_EQ(haps[2].part, Span(Cycle(3, 4), Cycle(1)));
}

TEST(ParsePatternTest, ReadsACommaAsBothAtOnce)
{
    const auto haps = read("[0, 5]");

    ASSERT_EQ(haps.size(), 2U);
    EXPECT_EQ(valueOf(haps[0]), 0);
    EXPECT_EQ(valueOf(haps[1]), 5);
    EXPECT_EQ(haps[0].part, haps[1].part);
}

TEST(ParsePatternTest, ReadsATopLevelCommaAsBothAtOnce)
{
    const auto haps = read("0, 5");

    ASSERT_EQ(haps.size(), 2U);
    EXPECT_EQ(haps[0].part, cycles(0, 1));
    EXPECT_EQ(haps[1].part, cycles(0, 1));
}

TEST(ParsePatternTest, ReadsAngleBracketsAsOnePerCycle)
{
    const auto haps = read("<3 5>", 0, 3);

    ASSERT_EQ(haps.size(), 3U);
    EXPECT_EQ(valueOf(haps[0]), 3);
    EXPECT_EQ(valueOf(haps[1]), 5);
    EXPECT_EQ(valueOf(haps[2]), 3);
}

TEST(ParsePatternTest, ReadsAStarAsFaster)
{
    const auto haps = read("3*2");

    ASSERT_EQ(haps.size(), 2U);
    EXPECT_EQ(haps[0].part, Span(Cycle(), Cycle(1, 2)));
    EXPECT_EQ(haps[1].part, Span(Cycle(1, 2), Cycle(1)));
}

TEST(ParsePatternTest, ReadsASlashAsSlower)
{
    const auto haps = read("3/2", 0, 2);

    ASSERT_EQ(haps.size(), 1U);
    EXPECT_EQ(haps[0].part, cycles(0, 2));
}

// An exact ratio is the point, and a decimal would not be one.
TEST(ParsePatternTest, ReadsAPercentAsAnExactRatio)
{
    const auto haps = read("3*3%2", 0, 2);

    ASSERT_EQ(haps.size(), 3U);
    EXPECT_EQ(haps[0].part, Span(Cycle(), Cycle(2, 3)));
    EXPECT_EQ(haps[1].part, Span(Cycle(2, 3), Cycle(4, 3)));
}

TEST(ParsePatternTest, ReadsABangAsRepeatedSlots)
{
    const auto haps = read("3!3");

    ASSERT_EQ(haps.size(), 3U);
    EXPECT_EQ(haps[0].part, Span(Cycle(), Cycle(1, 3)));
    EXPECT_EQ(haps[2].part, Span(Cycle(2, 3), Cycle(1)));
}

TEST(ParsePatternTest, ReadsBracketsAsAEuclideanRhythm)
{
    const auto haps = read("3(3,8)");

    ASSERT_EQ(haps.size(), 3U);
    EXPECT_EQ(haps[0].part, Span(Cycle(), Cycle(1, 8)));
    EXPECT_EQ(haps[1].part, Span(Cycle(3, 8), Cycle(1, 2)));
    EXPECT_EQ(haps[2].part, Span(Cycle(3, 4), Cycle(7, 8)));
}

TEST(ParsePatternTest, ReadsAQuestionMarkAsDroppingHalfTheEvents)
{
    const auto thinned = read("3*16?", 0, 8);
    const auto whole = read("3*16", 0, 8);

    EXPECT_LT(thinned.size(), whole.size());
    EXPECT_GT(thinned.size(), 0U);
}

// Counted left to right, so two of them thin out differently.
TEST(ParsePatternTest, EachQuestionMarkGetsItsOwnSeed)
{
    const auto haps = read("[3*16?, 5*16?]", 0, 4);

    std::vector<Span> first;
    std::vector<Span> second;

    for (const auto &hap : haps)
    {
        (valueOf(hap) == 3 ? first : second).push_back(hap.part);
    }

    EXPECT_NE(first, second);
}

TEST(ParsePatternTest, ModifiersStack)
{
    const auto haps = read("3*2!2");

    EXPECT_EQ(haps.size(), 4U);
}

TEST(ParsePatternTest, IgnoresSurroundingSpace)
{
    EXPECT_EQ(read("  0   3  ").size(), 2U);
}

TEST(ParsePatternTest, RefusesAnEmptyPattern)
{
    EXPECT_THROW((void)read(""), NotationError);
    EXPECT_THROW((void)read("   "), NotationError);
}

TEST(ParsePatternTest, RefusesABracketNothingCloses)
{
    EXPECT_THROW((void)read("[0 3"), NotationError);
    EXPECT_THROW((void)read("<0 3"), NotationError);
    EXPECT_THROW((void)read("0 3]"), NotationError);
    EXPECT_THROW((void)read("3(3,8"), NotationError);
}

TEST(ParsePatternTest, RefusesAGroupHoldingNothing)
{
    EXPECT_THROW((void)read("[]"), NotationError);
    EXPECT_THROW((void)read("<>"), NotationError);
    EXPECT_THROW((void)read("0 [,]"), NotationError);
}

TEST(ParsePatternTest, RefusesAModifierWithNoNumber)
{
    EXPECT_THROW((void)read("3*"), NotationError);
    EXPECT_THROW((void)read("3!"), NotationError);
    EXPECT_THROW((void)read("3(,8)"), NotationError);
    EXPECT_THROW((void)read("3*3%"), NotationError);
}

TEST(ParsePatternTest, RefusesACharacterTheGrammarDoesNotKnow)
{
    EXPECT_THROW((void)read("3 @ 5"), NotationError);

    // Just past 'z', so it fails the last half of the letter test.
    EXPECT_THROW((void)read("3 {"), NotationError);
}

TEST(ParsePatternTest, RefusesNoSlotsAtAll)
{
    EXPECT_THROW((void)read("3!0"), NotationError);
}

// The fraction NumberWords promises, reachable through the grammar.
// '%' is a word character for exactly this.
TEST(ParsePatternTest, ReadsAWordHoldingAFraction)
{
    const auto haps = read("3%2");

    ASSERT_EQ(haps.size(), 1U);
    EXPECT_EQ(*haps[0].value.get(kName), ParamValue(3, 2));
}

// A count this big is a vector no machine can hold.
// So it has to fail loudly here rather than as a bad_alloc later.
TEST(ParsePatternTest, RefusesACountAboveItsLimit)
{
    EXPECT_THROW((void)read("0!2000000000"), NotationError);
    EXPECT_THROW((void)read("0(3,2000000000)"), NotationError);
    EXPECT_THROW((void)read("0*999999999"), NotationError);
}

// Too long to be an integer at all, rather than merely too large.
// The old accumulation overflowed here, which is undefined.
TEST(ParsePatternTest, RefusesANumberTooLongToHold)
{
    EXPECT_THROW(
        (void)read("0!99999999999999999999999"), NotationError);
}

TEST(ParsePatternTest, AcceptsACountAtItsLimit)
{
    EXPECT_EQ(read("0!1024").size(), 1024U);
    EXPECT_EQ(read("0*1024").size(), 1024U);
}

// A single factor within the limit still composes past it.
// So it is the product along a nesting path that is bounded.
TEST(ParsePatternTest, RefusesSpeedFactorsThatMultiplyPastTheLimit)
{
    EXPECT_THROW((void)read("0*64*64*64"), NotationError);
    EXPECT_THROW((void)read("[0*64]*64"), NotationError);
    EXPECT_THROW((void)read("<0*64>*64"), NotationError);
    EXPECT_THROW((void)read("[~, 0*64]*64"), NotationError);

    // A slash by a fraction below one is a speed-up too.
    EXPECT_THROW((void)read("0/1%1024/1%2"), NotationError);
}

TEST(ParsePatternTest, LetsSeparateTermsEachHaveTheirOwnSpeed)
{
    EXPECT_EQ(read("0*64 3*64 5*64").size(), 192U);
}

// It parses cleanly and asks for something no pattern could be.
// So the algebra refuses it rather than the grammar.
TEST(ParsePatternTest, LetsTheAlgebraRefuseWhatItParsedCleanly)
{
    EXPECT_THROW((void)read("3(9,8)"), PatternError);
    EXPECT_THROW((void)read("3*0"), PatternError);
}

// A word is letters, digits and a handful of marks.
// Which of them a reader accepts is the reader's own business.
TEST(ParsePatternTest, ReadsEveryCharacterAWordMayHold)
{
    const AnyWord any;

    const auto haps = parsePattern("Bd_x.y#z+w-v 3", any)
                          .queryAll(cycles(0, 1));

    EXPECT_EQ(haps.size(), 2U);
}

TEST(ParsePatternTest, TreatsATabAsSpace)
{
    EXPECT_EQ(read("0\t3").size(), 2U);
}

TEST(ParsePatternTest, RefusesTheWrongBracketClosingAGroup)
{
    EXPECT_THROW((void)read("[0 3>"), NotationError);
}

// The reader is told where each word starts.
// A live editor points controls back at the source with it.
TEST(ParsePatternTest, HandsTheReaderEachWordsOffset)
{
    // Encodes the offset it was given as the control's value.
    class OffsetWord final : public antwika::notation::IWordReader
    {
    public:
        [[nodiscard]] Controls read(
            std::string_view, std::size_t at) const override
        {
            return Controls(
                kName,
                ParamValue(static_cast<std::int64_t>(at)));
        }
    };

    const OffsetWord reader;

    const auto haps = parsePattern("0 [3 5] <7 9>", reader)
                          .queryAll(cycles(0, 2));

    std::vector<std::int64_t> offsets;
    offsets.reserve(haps.size());

    for (const auto &hap : haps)
    {
        offsets.push_back(valueOf(hap));
    }

    std::ranges::sort(offsets);

    // "0" at 0, "3" at 3, "5" at 5 twice, "7" at 9, "9" at 11.
    const std::vector<std::int64_t> expected{0, 0, 3, 3, 5, 5, 9, 11};

    EXPECT_EQ(offsets, expected);
}
