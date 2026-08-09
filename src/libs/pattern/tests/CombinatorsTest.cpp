#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include "antwika/pattern/Combinators.hpp"
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
using antwika::pattern::during;
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
}

TEST(CombinatorsTest, Fast_FitsMoreCyclesIntoOne)
{
    const auto haps = fast(Cycle(2), note(1)).queryAll(cycles(0, 1));

    ASSERT_EQ(haps.size(), 2U);
    EXPECT_EQ(haps[0].part, Span(Cycle(), Cycle(1, 2)));
    EXPECT_EQ(haps[1].part, Span(Cycle(1, 2), Cycle(1)));
    EXPECT_TRUE(haps[1].hasOnset());
}

TEST(CombinatorsTest, Slow_SpreadsOneCycleOverMore)
{
    const auto haps = slow(Cycle(2), note(1)).queryAll(cycles(0, 2));

    ASSERT_EQ(haps.size(), 1U);
    EXPECT_EQ(haps[0].part, cycles(0, 2));
    EXPECT_EQ(haps[0].whole, cycles(0, 2));
}

TEST(CombinatorsTest, Fast_AndSlowUndoOneAnother)
{
    const auto there = fast(Cycle(3), fastcat({note(1), note(2)}));
    const auto back = slow(Cycle(3), there);

    const auto parts = partsOf(back.queryAll(cycles(0, 1)));

    ASSERT_EQ(parts.size(), 2U);
    EXPECT_EQ(parts[0], Span(Cycle(), Cycle(1, 2)));
    EXPECT_EQ(
        parts,
        partsOf(fastcat({note(1), note(2)}).queryAll(cycles(0, 1))));
}

TEST(CombinatorsTest, Fast_TakesAnExactRatio)
{
    const auto haps =
        fast(Cycle(3, 2), note(1)).queryAll(cycles(0, 3));

    ASSERT_EQ(haps.size(), 5U);
    EXPECT_EQ(haps[1].part, Span(Cycle(2, 3), Cycle(4, 3)));
}

TEST(CombinatorsTest, Fast_RefusesNoSpeedOrBackwards)
{
    EXPECT_THROW((void)fast(Cycle(), note(1)), PatternError);
    EXPECT_THROW((void)fast(Cycle(-1), note(1)), PatternError);
}

TEST(CombinatorsTest, Slow_RefusesNoSpeedOrBackwards)
{
    EXPECT_THROW((void)slow(Cycle(), note(1)), PatternError);
    EXPECT_THROW((void)slow(Cycle(-1), note(1)), PatternError);
}

TEST(CombinatorsTest, Early_BringsAPatternForward)
{
    const auto haps =
        early(Cycle(1, 4), note(1)).queryAll(cycles(0, 1));

    ASSERT_EQ(haps.size(), 2U);
    EXPECT_EQ(haps[0].whole, Span(Cycle(-1, 4), Cycle(3, 4)));
    EXPECT_EQ(haps[1].whole, Span(Cycle(3, 4), Cycle(7, 4)));
}

TEST(CombinatorsTest, Late_PushesAPatternBack)
{
    const auto haps =
        late(Cycle(1, 4), note(1)).queryAll(cycles(0, 1));

    ASSERT_EQ(haps.size(), 2U);
    EXPECT_EQ(haps[0].whole, Span(Cycle(-3, 4), Cycle(1, 4)));
    EXPECT_EQ(haps[1].whole, Span(Cycle(1, 4), Cycle(5, 4)));
}

TEST(CombinatorsTest, Early_AndLateUndoOneAnother)
{
    const auto there = early(Cycle(1, 3), note(1));
    const auto back = late(Cycle(1, 3), there);

    const auto parts = partsOf(back.queryAll(cycles(0, 2)));

    ASSERT_EQ(parts.size(), 2U);
    EXPECT_EQ(parts[0], cycles(0, 1));
    EXPECT_EQ(parts, partsOf(note(1).queryAll(cycles(0, 2))));
}

TEST(CombinatorsTest, Rev_PlaysACycleBackwards)
{
    const auto haps =
        rev(fastcat({note(1), note(2)})).queryAll(cycles(0, 1));

    ASSERT_EQ(haps.size(), 2U);

    EXPECT_EQ(haps[0].value, named(1));
    EXPECT_EQ(haps[0].part, Span(Cycle(1, 2), Cycle(1)));

    EXPECT_EQ(haps[1].value, named(2));
    EXPECT_EQ(haps[1].part, Span(Cycle(), Cycle(1, 2)));
}

TEST(CombinatorsTest, Rev_TwiceIsThePatternAgain)
{
    const auto once = fastcat({note(1), note(2), note(3)});

    const auto parts = partsOf(rev(rev(once)).queryAll(cycles(0, 2)));

    ASSERT_EQ(parts.size(), 6U);
    EXPECT_EQ(parts[0], Span(Cycle(), Cycle(1, 3)));
    EXPECT_EQ(parts, partsOf(once.queryAll(cycles(0, 2))));
}

TEST(CombinatorsTest, Euclid_SpreadsOnsetsAsEvenlyAsPossible)
{
    const auto haps = euclid(3, 8, note(1)).queryAll(cycles(0, 1));

    ASSERT_EQ(haps.size(), 3U);
    EXPECT_EQ(haps[0].part, Span(Cycle(), Cycle(1, 8)));
    EXPECT_EQ(haps[1].part, Span(Cycle(3, 8), Cycle(1, 2)));
    EXPECT_EQ(haps[2].part, Span(Cycle(3, 4), Cycle(7, 8)));
}

TEST(CombinatorsTest, Euclid_SoundsEveryStepWhenAskedForAll)
{
    EXPECT_EQ(euclid(4, 4, note(1)).queryAll(cycles(0, 1)).size(), 4U);
}

TEST(CombinatorsTest, Euclid_SoundsNothingForNoOnsets)
{
    EXPECT_TRUE(euclid(0, 8, note(1)).queryAll(cycles(0, 1)).empty());
}

TEST(CombinatorsTest, Euclid_RefusesAnImpossibleRhythm)
{
    EXPECT_THROW((void)euclid(3, 0, note(1)), PatternError);
    EXPECT_THROW((void)euclid(-1, 8, note(1)), PatternError);
    EXPECT_THROW((void)euclid(9, 8, note(1)), PatternError);
}

TEST(CombinatorsTest, DegradeBy_KeepsEverythingAtNothing)
{
    const auto kept =
        degradeBy(ParamValue(), 7, fastcat({note(1), note(2)}))
            .queryAll(cycles(0, 8));

    EXPECT_EQ(kept.size(), 16U);
}

TEST(CombinatorsTest, DegradeBy_KeepsNothingAtCertainty)
{
    const auto kept =
        degradeBy(ParamValue(1), 7, fastcat({note(1), note(2)}))
            .queryAll(cycles(0, 8));

    EXPECT_TRUE(kept.empty());
}

TEST(CombinatorsTest, DegradeBy_RefusesAChanceOutOfRange)
{
    EXPECT_THROW(
        (void)degradeBy(ParamValue(-1, 2), 7, note(1)), PatternError);

    EXPECT_THROW((void)degradeBy(ParamValue(2), 7, note(1)), PatternError);
}

TEST(CombinatorsTest, DegradeBy_ThinsOutWithoutEmptying)
{
    const auto thinned =
        degradeBy(ParamValue(1, 2), 7, fast(Cycle(8), note(1)))
            .queryAll(cycles(0, 8));

    EXPECT_EQ(thinned.size(), 36U);
}

TEST(CombinatorsTest, DegradeBy_AnswersTheSameHoweverAsked)
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

    ASSERT_FALSE(straight.empty());
    EXPECT_EQ(straight, walked);
}

TEST(CombinatorsTest, DegradeBy_KeepsAContinuousValueHoweverAsked)
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

TEST(CombinatorsTest, DegradeBy_KeepsAContinuousValueAtCertainty)
{
    const auto kept =
        degradeBy(ParamValue(1), 7, steady(named(9)))
            .queryAll(cycles(0, 1));

    ASSERT_EQ(kept.size(), 1U);
    EXPECT_FALSE(kept[0].whole.has_value());
}

TEST(CombinatorsTest, DegradeBy_DiffersBySeed)
{
    const auto inner = fast(Cycle(16), note(1));

    const auto one =
        degradeBy(ParamValue(1, 2), 1, inner).queryAll(cycles(0, 4));

    const auto other =
        degradeBy(ParamValue(1, 2), 2, inner).queryAll(cycles(0, 4));

    ASSERT_FALSE(one.empty());
    ASSERT_FALSE(other.empty());
    EXPECT_NE(partsOf(one), partsOf(other));
}

TEST(CombinatorsTest, Fast_KeepsAContinuousValueContinuous)
{
    const auto haps =
        fast(Cycle(2), steady(named(9))).queryAll(cycles(0, 1));

    ASSERT_EQ(haps.size(), 1U);
    EXPECT_FALSE(haps[0].whole.has_value());
    EXPECT_EQ(haps[0].part, cycles(0, 1));
}

TEST(CombinatorsTest, Rev_KeepsAContinuousValueContinuous)
{
    const auto haps = rev(steady(named(9))).queryAll(cycles(0, 1));

    ASSERT_EQ(haps.size(), 1U);
    EXPECT_FALSE(haps[0].whole.has_value());
}

TEST(CombinatorsTest, During_PlaysInsideItsWindowsOnly)
{
    const auto scheduled =
        during(4, {cycles(1, 2)}, note(1));

    EXPECT_TRUE(scheduled.queryAll(cycles(0, 1)).empty());
    EXPECT_EQ(scheduled.queryAll(cycles(1, 2)).size(), 1U);
    EXPECT_TRUE(scheduled.queryAll(cycles(2, 4)).empty());
}

TEST(CombinatorsTest, During_RestartsAtEachWindow)
{
    const auto alternation = antwika::pattern::slowcat(
        {note(1), note(2)});

    const auto scheduled = during(
        8,
        {cycles(2, 4), cycles(6, 8)},
        alternation);

    const auto haps = scheduled.queryAll(cycles(0, 8));

    ASSERT_EQ(haps.size(), 4U);
    EXPECT_EQ(haps[0].value, named(1));
    EXPECT_EQ(haps[1].value, named(2));
    EXPECT_EQ(haps[2].value, named(1));
    EXPECT_EQ(haps[3].value, named(2));
}

TEST(CombinatorsTest, During_RepeatsEveryPeriod)
{
    const auto scheduled = during(4, {cycles(1, 2)}, note(1));

    const auto first = scheduled.queryAll(cycles(1, 2));
    const auto again = scheduled.queryAll(cycles(5, 6));

    ASSERT_EQ(first.size(), 1U);
    ASSERT_EQ(again.size(), 1U);
    EXPECT_EQ(again[0].part, cycles(5, 6));
    EXPECT_EQ(again[0].value, first[0].value);
}

TEST(CombinatorsTest, During_RepeatsBehindZeroToo)
{
    const auto scheduled = during(4, {cycles(1, 2)}, note(1));

    const auto haps = scheduled.queryAll(cycles(-4, 0));

    ASSERT_EQ(haps.size(), 1U);
    EXPECT_EQ(haps[0].part, cycles(-3, -2));

    const auto inside = scheduled.queryAll(cycles(-3, -2));

    ASSERT_EQ(inside.size(), 1U);
    EXPECT_EQ(inside[0].part, cycles(-3, -2));
}

TEST(CombinatorsTest, During_CutsAtTheEdgeAndKeepsTheWhole)
{
    const auto held = slow(Cycle(2), note(1));

    const auto scheduled = during(4, {cycles(0, 1)}, held);

    const auto haps = scheduled.queryAll(cycles(0, 4));

    ASSERT_EQ(haps.size(), 1U);
    EXPECT_EQ(haps[0].part, cycles(0, 1));
    EXPECT_EQ(haps[0].whole, cycles(0, 2));
    EXPECT_TRUE(haps[0].hasOnset());
}

TEST(CombinatorsTest, During_KeepsAContinuousValueContinuous)
{
    const auto scheduled =
        during(4, {cycles(1, 3)}, steady(named(9)));

    const auto haps = scheduled.queryAll(cycles(1, 2));

    ASSERT_EQ(haps.size(), 1U);
    EXPECT_FALSE(haps[0].whole.has_value());
    EXPECT_EQ(haps[0].part, cycles(1, 2));
}

TEST(CombinatorsTest, During_AnswersTheSameHoweverAsked)
{
    const auto scheduled = during(
        8,
        {cycles(2, 4), cycles(6, 8)},
        fast(Cycle(2), note(1)));

    const auto wide = scheduled.queryAll(cycles(0, 8));

    std::vector<Hap> narrow;

    for (std::int64_t cycle = 0; cycle < 8; ++cycle)
    {
        for (const auto &hap :
             scheduled.queryAll(cycles(cycle, cycle + 1)))
        {
            narrow.push_back(hap);
        }
    }

    ASSERT_EQ(wide.size(), 8U);
    EXPECT_EQ(wide, narrow);
}

TEST(CombinatorsTest, During_RefusesAPeriodOfNothing)
{
    EXPECT_THROW(
        (void)during(0, {cycles(0, 1)}, note(1)), PatternError);
}

TEST(CombinatorsTest, During_RefusesAScheduleOfNoWindows)
{
    EXPECT_THROW((void)during(4, {}, note(1)), PatternError);
}

TEST(CombinatorsTest, During_RefusesOverlappingWindows)
{
    EXPECT_THROW(
        (void)during(4, {cycles(0, 2), cycles(1, 3)}, note(1)),
        PatternError);
}

TEST(CombinatorsTest, During_RefusesAWindowPastItsPeriod)
{
    EXPECT_THROW(
        (void)during(4, {cycles(2, 5)}, note(1)), PatternError);
}
