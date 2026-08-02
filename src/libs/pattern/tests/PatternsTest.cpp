#include "antwika/pattern/Patterns.hpp"

#include <vector>

#include <gtest/gtest.h>

#include "antwika/pattern/Controls.hpp"
#include "antwika/pattern/Cycle.hpp"
#include "antwika/pattern/Hap.hpp"
#include "antwika/pattern/ParamId.hpp"
#include "antwika/pattern/ParamValue.hpp"
#include "antwika/pattern/Pattern.hpp"
#include "antwika/pattern/PatternError.hpp"
#include "antwika/pattern/Span.hpp"

using antwika::pattern::Controls;
using antwika::pattern::Cycle;
using antwika::pattern::fastcat;
using antwika::pattern::Hap;
using antwika::pattern::ParamId;
using antwika::pattern::ParamValue;
using antwika::pattern::Pattern;
using antwika::pattern::PatternError;
using antwika::pattern::pure;
using antwika::pattern::silence;
using antwika::pattern::slowcat;
using antwika::pattern::stack;
using antwika::pattern::steady;
using antwika::pattern::Span;

namespace
{
    constexpr ParamId kName{1};

    [[nodiscard]] Controls named(std::int64_t which)
    {
        return Controls(kName, ParamValue(which));
    }

    [[nodiscard]] Pattern note(std::int64_t which)
    {
        return pure(named(which));
    }

    [[nodiscard]] Span cycles(std::int64_t from, std::int64_t to)
    {
        return Span(Cycle(from), Cycle(to));
    }
} // namespace

TEST(PatternsTest, SilenceAnswersEveryQueryWithNothing)
{
    EXPECT_TRUE(silence().queryAll(cycles(0, 4)).empty());
}

TEST(PatternsTest, PureRepeatsItsValueOncePerCycle)
{
    const auto haps = note(1).queryAll(cycles(0, 3));

    ASSERT_EQ(haps.size(), 3U);

    for (std::size_t cycle = 0; cycle < 3U; ++cycle)
    {
        const auto begin = Cycle(static_cast<std::int64_t>(cycle));
        const auto end = Cycle(static_cast<std::int64_t>(cycle) + 1);

        EXPECT_EQ(haps[cycle].part, Span(begin, end));
        EXPECT_EQ(haps[cycle].whole, Span(begin, end));
        EXPECT_TRUE(haps[cycle].hasOnset());
    }
}

// A window that cuts an event still reports the whole it belongs to.
// That is what lets a sequencer tell a tail from an onset.
TEST(PatternsTest, PureReportsTheWholeEventAWindowCut)
{
    const auto haps =
        note(1).queryAll(Span(Cycle(1, 2), Cycle(3, 2)));

    ASSERT_EQ(haps.size(), 2U);

    EXPECT_EQ(haps[0].part, Span(Cycle(1, 2), Cycle(1)));
    EXPECT_EQ(haps[0].whole, Span(Cycle(), Cycle(1)));
    EXPECT_FALSE(haps[0].hasOnset());

    EXPECT_EQ(haps[1].part, Span(Cycle(1), Cycle(3, 2)));
    EXPECT_EQ(haps[1].whole, Span(Cycle(1), Cycle(2)));
    EXPECT_TRUE(haps[1].hasOnset());
}

TEST(PatternsTest, StackSoundsEveryLayer)
{
    const auto haps =
        stack({note(1), note(2)}).queryAll(cycles(0, 1));

    ASSERT_EQ(haps.size(), 2U);
    EXPECT_EQ(haps[0].value, named(1));
    EXPECT_EQ(haps[1].value, named(2));
    EXPECT_EQ(haps[0].part, haps[1].part);
}

TEST(PatternsTest, StackOfNothingIsSilent)
{
    EXPECT_TRUE(stack({}).queryAll(cycles(0, 1)).empty());
}

TEST(PatternsTest, SlowcatGivesEachPatternACycleInTurn)
{
    const auto haps =
        slowcat({note(1), note(2)}).queryAll(cycles(0, 4));

    ASSERT_EQ(haps.size(), 4U);
    EXPECT_EQ(haps[0].value, named(1));
    EXPECT_EQ(haps[1].value, named(2));
    EXPECT_EQ(haps[2].value, named(1));
    EXPECT_EQ(haps[3].value, named(2));

    EXPECT_EQ(haps[0].part, cycles(0, 1));
    EXPECT_EQ(haps[3].part, cycles(3, 4));
}

TEST(PatternsTest, SlowcatKeepsGoingBeforeCycleZero)
{
    const auto haps =
        slowcat({note(1), note(2)}).queryAll(cycles(-2, 0));

    ASSERT_EQ(haps.size(), 2U);
    EXPECT_EQ(haps[0].value, named(1));
    EXPECT_EQ(haps[1].value, named(2));
}

TEST(PatternsTest, SlowcatOfNothingIsRefused)
{
    EXPECT_THROW((void)slowcat({}), PatternError);
}

// What a bare sequence in the mini-notation will mean.
TEST(PatternsTest, FastcatFitsEveryPatternIntoOneCycle)
{
    const auto haps =
        fastcat({note(1), note(2)}).queryAll(cycles(0, 1));

    ASSERT_EQ(haps.size(), 2U);

    EXPECT_EQ(haps[0].value, named(1));
    EXPECT_EQ(haps[0].part, Span(Cycle(), Cycle(1, 2)));
    EXPECT_TRUE(haps[0].hasOnset());

    EXPECT_EQ(haps[1].value, named(2));
    EXPECT_EQ(haps[1].part, Span(Cycle(1, 2), Cycle(1)));
    EXPECT_TRUE(haps[1].hasOnset());
}

TEST(PatternsTest, FastcatRepeatsEveryCycle)
{
    const auto haps =
        fastcat({note(1), note(2)}).queryAll(cycles(0, 2));

    ASSERT_EQ(haps.size(), 4U);
    EXPECT_EQ(haps[2].part, Span(Cycle(1), Cycle(3, 2)));
    EXPECT_EQ(haps[3].part, Span(Cycle(3, 2), Cycle(2)));
}

TEST(PatternsTest, FastcatOfNothingIsRefused)
{
    EXPECT_THROW((void)fastcat({}), PatternError);
}

// Three subdivisions land where no fixed resolution holds them.
// Here they are exact.
TEST(PatternsTest, FastcatSplitsIntoThirdsExactly)
{
    const auto haps =
        fastcat({note(1), note(2), note(3)}).queryAll(cycles(0, 1));

    ASSERT_EQ(haps.size(), 3U);
    EXPECT_EQ(haps[0].part, Span(Cycle(), Cycle(1, 3)));
    EXPECT_EQ(haps[1].part, Span(Cycle(1, 3), Cycle(2, 3)));
    EXPECT_EQ(haps[2].part, Span(Cycle(2, 3), Cycle(1)));
}

// Seven is exactly what a 960-pulse resolution cannot divide by.
TEST(PatternsTest, FastcatSplitsIntoSeventhsExactly)
{
    std::vector<Pattern> parts;

    for (std::int64_t which = 0; which < 7; ++which)
    {
        parts.push_back(note(which));
    }

    const auto haps = fastcat(std::move(parts)).queryAll(cycles(0, 1));

    ASSERT_EQ(haps.size(), 7U);
    EXPECT_EQ(haps[3].part, Span(Cycle(3, 7), Cycle(4, 7)));
}

// Continuous rather than an event, which the optional whole is for.
// It never begins, so a sequencer reads it and never triggers it.
TEST(PatternsTest, SteadyIsAlwaysThereAndNeverBegins)
{
    const auto haps = steady(named(9)).queryAll(cycles(0, 3));

    ASSERT_EQ(haps.size(), 1U);
    EXPECT_EQ(haps[0].part, cycles(0, 3));
    EXPECT_FALSE(haps[0].whole.has_value());
    EXPECT_FALSE(haps[0].hasOnset());
    EXPECT_EQ(haps[0].value, named(9));
}

TEST(PatternsTest, SteadySurvivesBeingStackedWithEvents)
{
    const auto haps =
        stack({note(1), steady(named(9))}).queryAll(cycles(0, 1));

    ASSERT_EQ(haps.size(), 2U);
    EXPECT_TRUE(haps[0].hasOnset());
    EXPECT_FALSE(haps[1].hasOnset());
}

TEST(PatternsTest, SteadyStaysWholeLessThroughASequence)
{
    const auto haps =
        slowcat({steady(named(9)), note(1)}).queryAll(cycles(0, 2));

    ASSERT_EQ(haps.size(), 2U);
    EXPECT_FALSE(haps[0].whole.has_value());
    EXPECT_EQ(haps[0].part, cycles(0, 1));
    EXPECT_TRUE(haps[1].hasOnset());
}
