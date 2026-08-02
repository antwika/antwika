#include "antwika/pattern/Combinators.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include "antwika/pattern/Controls.hpp"
#include "antwika/pattern/Cycle.hpp"
#include "antwika/pattern/Hap.hpp"
#include "antwika/pattern/ParamId.hpp"
#include "antwika/pattern/ParamValue.hpp"
#include "antwika/pattern/Pattern.hpp"
#include "antwika/pattern/PatternError.hpp"
#include "antwika/pattern/Patterns.hpp"
#include "antwika/pattern/Span.hpp"

using antwika::pattern::Controls;
using antwika::pattern::Cycle;
using antwika::pattern::degradeBy;
using antwika::pattern::early;
using antwika::pattern::euclid;
using antwika::pattern::fast;
using antwika::pattern::fastcat;
using antwika::pattern::Hap;
using antwika::pattern::late;
using antwika::pattern::ParamId;
using antwika::pattern::ParamValue;
using antwika::pattern::Pattern;
using antwika::pattern::PatternError;
using antwika::pattern::pure;
using antwika::pattern::rev;
using antwika::pattern::slow;
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

    [[nodiscard]] std::vector<Span> partsOf(const std::vector<Hap> &haps)
    {
        std::vector<Span> spans;
        spans.reserve(haps.size());

        for (const auto &hap : haps)
        {
            spans.push_back(hap.part);
        }

        return spans;
    }
} // namespace

TEST(CombinatorsTest, FastFitsMoreCyclesIntoOne)
{
    const auto haps = fast(Cycle(2), note(1)).queryAll(cycles(0, 1));

    ASSERT_EQ(haps.size(), 2U);
    EXPECT_EQ(haps[0].part, Span(Cycle(), Cycle(1, 2)));
    EXPECT_EQ(haps[1].part, Span(Cycle(1, 2), Cycle(1)));
    EXPECT_TRUE(haps[1].hasOnset());
}

TEST(CombinatorsTest, SlowSpreadsOneCycleOverMore)
{
    const auto haps = slow(Cycle(2), note(1)).queryAll(cycles(0, 2));

    ASSERT_EQ(haps.size(), 1U);
    EXPECT_EQ(haps[0].part, cycles(0, 2));
    EXPECT_EQ(haps[0].whole, cycles(0, 2));
}

TEST(CombinatorsTest, FastAndSlowUndoOneAnother)
{
    const auto there = fast(Cycle(3), fastcat({note(1), note(2)}));
    const auto back = slow(Cycle(3), there);

    EXPECT_EQ(
        partsOf(back.queryAll(cycles(0, 1))),
        partsOf(fastcat({note(1), note(2)}).queryAll(cycles(0, 1))));
}

// An exact ratio rather than a float.
// Composing several never accumulates drift.
TEST(CombinatorsTest, FastTakesAnExactRatio)
{
    const auto haps =
        fast(Cycle(3, 2), note(1)).queryAll(cycles(0, 3));

    ASSERT_EQ(haps.size(), 5U);
    EXPECT_EQ(haps[1].part, Span(Cycle(2, 3), Cycle(4, 3)));
}

TEST(CombinatorsTest, RefusesToRunAtNoSpeedOrBackwards)
{
    EXPECT_THROW((void)fast(Cycle(), note(1)), PatternError);
    EXPECT_THROW((void)fast(Cycle(-1), note(1)), PatternError);
    EXPECT_THROW((void)slow(Cycle(), note(1)), PatternError);
    EXPECT_THROW((void)slow(Cycle(-1), note(1)), PatternError);
}

TEST(CombinatorsTest, EarlyBringsAPatternForward)
{
    const auto haps =
        early(Cycle(1, 4), note(1)).queryAll(cycles(0, 1));

    ASSERT_EQ(haps.size(), 2U);
    EXPECT_EQ(haps[0].whole, Span(Cycle(-1, 4), Cycle(3, 4)));
    EXPECT_EQ(haps[1].whole, Span(Cycle(3, 4), Cycle(7, 4)));
}

TEST(CombinatorsTest, LatePushesAPatternBack)
{
    const auto haps =
        late(Cycle(1, 4), note(1)).queryAll(cycles(0, 1));

    ASSERT_EQ(haps.size(), 2U);
    EXPECT_EQ(haps[0].whole, Span(Cycle(-3, 4), Cycle(1, 4)));
    EXPECT_EQ(haps[1].whole, Span(Cycle(1, 4), Cycle(5, 4)));
}

TEST(CombinatorsTest, EarlyAndLateUndoOneAnother)
{
    const auto there = early(Cycle(1, 3), note(1));
    const auto back = late(Cycle(1, 3), there);

    EXPECT_EQ(
        partsOf(back.queryAll(cycles(0, 2))),
        partsOf(note(1).queryAll(cycles(0, 2))));
}

TEST(CombinatorsTest, RevPlaysACycleBackwards)
{
    const auto haps =
        rev(fastcat({note(1), note(2)})).queryAll(cycles(0, 1));

    ASSERT_EQ(haps.size(), 2U);

    EXPECT_EQ(haps[0].value, named(1));
    EXPECT_EQ(haps[0].part, Span(Cycle(1, 2), Cycle(1)));

    EXPECT_EQ(haps[1].value, named(2));
    EXPECT_EQ(haps[1].part, Span(Cycle(), Cycle(1, 2)));
}

// Reflected inside its own cycle rather than around zero.
// Nothing moves into a neighbouring cycle.
// Reversing twice is the original.
TEST(CombinatorsTest, RevTwiceIsThePatternAgain)
{
    const auto once = fastcat({note(1), note(2), note(3)});

    EXPECT_EQ(
        partsOf(rev(rev(once)).queryAll(cycles(0, 2))),
        partsOf(once.queryAll(cycles(0, 2))));
}

// The rhythm nearly every traditional pattern in the world comes from.
TEST(CombinatorsTest, EuclidSpreadsOnsetsAsEvenlyAsStepsAllow)
{
    const auto haps = euclid(3, 8, note(1)).queryAll(cycles(0, 1));

    ASSERT_EQ(haps.size(), 3U);
    EXPECT_EQ(haps[0].part, Span(Cycle(), Cycle(1, 8)));
    EXPECT_EQ(haps[1].part, Span(Cycle(3, 8), Cycle(1, 2)));
    EXPECT_EQ(haps[2].part, Span(Cycle(3, 4), Cycle(7, 8)));
}

TEST(CombinatorsTest, EuclidSoundsEveryStepWhenAskedForAllOfThem)
{
    EXPECT_EQ(euclid(4, 4, note(1)).queryAll(cycles(0, 1)).size(), 4U);
}

TEST(CombinatorsTest, EuclidSoundsNothingWhenAskedForNoOnsets)
{
    EXPECT_TRUE(euclid(0, 8, note(1)).queryAll(cycles(0, 1)).empty());
}

TEST(CombinatorsTest, EuclidRefusesARhythmThatCannotExist)
{
    EXPECT_THROW((void)euclid(3, 0, note(1)), PatternError);
    EXPECT_THROW((void)euclid(-1, 8, note(1)), PatternError);
    EXPECT_THROW((void)euclid(9, 8, note(1)), PatternError);
}

TEST(CombinatorsTest, DegradeByNothingKeepsEverything)
{
    const auto kept =
        degradeBy(ParamValue(), 7, fastcat({note(1), note(2)}))
            .queryAll(cycles(0, 8));

    EXPECT_EQ(kept.size(), 16U);
}

TEST(CombinatorsTest, DegradeByCertaintyKeepsNothing)
{
    const auto kept =
        degradeBy(ParamValue(1), 7, fastcat({note(1), note(2)}))
            .queryAll(cycles(0, 8));

    EXPECT_TRUE(kept.empty());
}

TEST(CombinatorsTest, DegradeByRefusesAChanceOutsideItsRange)
{
    EXPECT_THROW(
        (void)degradeBy(ParamValue(-1, 2), 7, note(1)), PatternError);

    EXPECT_THROW((void)degradeBy(ParamValue(2), 7, note(1)), PatternError);
}

TEST(CombinatorsTest, DegradeThinsOutSomethingWithoutEmptyingIt)
{
    const auto thinned =
        degradeBy(ParamValue(1, 2), 7, fast(Cycle(8), note(1)))
            .queryAll(cycles(0, 8));

    EXPECT_GT(thinned.size(), 8U);
    EXPECT_LT(thinned.size(), 56U);
}

// The property the whole design rests on:
// Asking about cycle four hundred answers as playing there would.
TEST(CombinatorsTest, DegradeAnswersTheSameHoweverItIsAskedFor)
{
    const auto thinned =
        degradeBy(ParamValue(1, 2), 7, fast(Cycle(4), note(1)));

    const auto straight = thinned.queryAll(cycles(400, 401));

    std::vector<Hap> walked;

    for (std::int64_t cycle = 396; cycle < 401; ++cycle)
    {
        for (const auto &hap :
             thinned.queryAll(cycles(cycle, cycle + 1)))
        {
            if (cycle == 400)
            {
                walked.push_back(hap);
            }
        }
    }

    EXPECT_EQ(straight, walked);
}

// A continuous value has no onset to drop.
// Its part is only ever the window that was asked about.
// So thinning one would make it answer differently per slice.
TEST(CombinatorsTest, DegradeKeepsAContinuousValueHoweverItIsAskedFor)
{
    const auto thinned =
        degradeBy(ParamValue(1, 2), 7, steady(named(9)));

    EXPECT_EQ(thinned.queryAll(cycles(0, 4)).size(), 1U);

    for (std::int64_t cycle = 0; cycle < 4; ++cycle)
    {
        EXPECT_EQ(thinned.queryAll(cycles(cycle, cycle + 1)).size(), 1U)
            << cycle;
    }
}

TEST(CombinatorsTest, DegradeByCertaintyStillKeepsAContinuousValue)
{
    const auto kept =
        degradeBy(ParamValue(1), 7, steady(named(9)))
            .queryAll(cycles(0, 1));

    ASSERT_EQ(kept.size(), 1U);
    EXPECT_FALSE(kept[0].whole.has_value());
}

TEST(CombinatorsTest, DegradeDiffersBySeed)
{
    const auto inner = fast(Cycle(16), note(1));

    const auto one =
        degradeBy(ParamValue(1, 2), 1, inner).queryAll(cycles(0, 4));

    const auto other =
        degradeBy(ParamValue(1, 2), 2, inner).queryAll(cycles(0, 4));

    EXPECT_NE(partsOf(one), partsOf(other));
}

// A continuous value has no whole to map, and keeps having none.
TEST(CombinatorsTest, TimeMappingKeepsAContinuousValueContinuous)
{
    const auto haps =
        fast(Cycle(2), steady(named(9))).queryAll(cycles(0, 1));

    ASSERT_EQ(haps.size(), 1U);
    EXPECT_FALSE(haps[0].whole.has_value());
    EXPECT_EQ(haps[0].part, cycles(0, 1));
}

TEST(CombinatorsTest, RevKeepsAContinuousValueContinuous)
{
    const auto haps = rev(steady(named(9))).queryAll(cycles(0, 1));

    ASSERT_EQ(haps.size(), 1U);
    EXPECT_FALSE(haps[0].whole.has_value());
}
