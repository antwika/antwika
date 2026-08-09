#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "antwika/pattern/Patterns.hpp"
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
}

TEST(PatternsTest, Silence_AnswersEveryQueryWithNothing)
{
    EXPECT_TRUE(silence().queryAll(cycles(0, 4)).empty());
}

TEST(PatternsTest, Pure_RepeatsItsValueOncePerCycle)
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

TEST(PatternsTest, Pure_ReportsTheWholeOfACutEvent)
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

TEST(PatternsTest, Stack_SoundsEveryLayer)
{
    const auto haps =
        stack({note(1), note(2)}).queryAll(cycles(0, 1));

    ASSERT_EQ(haps.size(), 2U);
    EXPECT_EQ(haps[0].value, named(1));
    EXPECT_EQ(haps[1].value, named(2));
    EXPECT_EQ(haps[0].part, haps[1].part);
}

TEST(PatternsTest, Stack_IsSilentForNothing)
{
    EXPECT_TRUE(stack({}).queryAll(cycles(0, 1)).empty());
}

TEST(PatternsTest, Slowcat_GivesEachPatternACycleInTurn)
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

TEST(PatternsTest, Slowcat_KeepsGoingBeforeCycleZero)
{
    const auto haps =
        slowcat({note(1), note(2)}).queryAll(cycles(-2, 0));

    ASSERT_EQ(haps.size(), 2U);
    EXPECT_EQ(haps[0].value, named(1));
    EXPECT_EQ(haps[1].value, named(2));
}

TEST(PatternsTest, Slowcat_RefusesNothing)
{
    EXPECT_THROW((void)slowcat({}), PatternError);
}

TEST(PatternsTest, Fastcat_FitsEveryPatternIntoOneCycle)
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

TEST(PatternsTest, Fastcat_RepeatsEveryCycle)
{
    const auto haps =
        fastcat({note(1), note(2)}).queryAll(cycles(0, 2));

    ASSERT_EQ(haps.size(), 4U);
    EXPECT_EQ(haps[2].part, Span(Cycle(1), Cycle(3, 2)));
    EXPECT_EQ(haps[3].part, Span(Cycle(3, 2), Cycle(2)));
}

TEST(PatternsTest, Fastcat_RefusesNothing)
{
    EXPECT_THROW((void)fastcat({}), PatternError);
}

TEST(PatternsTest, Fastcat_SplitsIntoThirdsExactly)
{
    const auto haps =
        fastcat({note(1), note(2), note(3)}).queryAll(cycles(0, 1));

    ASSERT_EQ(haps.size(), 3U);
    EXPECT_EQ(haps[0].part, Span(Cycle(), Cycle(1, 3)));
    EXPECT_EQ(haps[1].part, Span(Cycle(1, 3), Cycle(2, 3)));
    EXPECT_EQ(haps[2].part, Span(Cycle(2, 3), Cycle(1)));
}

TEST(PatternsTest, Fastcat_SplitsIntoSeventhsExactly)
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

TEST(PatternsTest, Steady_IsAlwaysThereAndNeverBegins)
{
    const auto haps = steady(named(9)).queryAll(cycles(0, 3));

    ASSERT_EQ(haps.size(), 1U);
    EXPECT_EQ(haps[0].part, cycles(0, 3));
    EXPECT_FALSE(haps[0].whole.has_value());
    EXPECT_FALSE(haps[0].hasOnset());
    EXPECT_EQ(haps[0].value, named(9));
}

TEST(PatternsTest, Steady_SurvivesBeingStacked)
{
    const auto haps =
        stack({note(1), steady(named(9))}).queryAll(cycles(0, 1));

    ASSERT_EQ(haps.size(), 2U);
    EXPECT_TRUE(haps[0].hasOnset());
    EXPECT_FALSE(haps[1].hasOnset());
}

TEST(PatternsTest, Steady_StaysWholeLessThroughASequence)
{
    const auto haps =
        slowcat({steady(named(9)), note(1)}).queryAll(cycles(0, 2));

    ASSERT_EQ(haps.size(), 2U);
    EXPECT_FALSE(haps[0].whole.has_value());
    EXPECT_EQ(haps[0].part, cycles(0, 1));
    EXPECT_TRUE(haps[1].hasOnset());
}

namespace
{
    using antwika::pattern::Slice;
    using antwika::pattern::timecat;

    [[nodiscard]] std::vector<Slice> twoOne()
    {
        std::vector<Slice> slices;
        slices.push_back(Slice{.weight = Cycle(2), .part = note(1)});
        slices.push_back(Slice{.weight = Cycle(1), .part = note(2)});

        return slices;
    }
}

TEST(PatternsTest, Timecat_SharesTheCycleByWeight)
{
    const auto haps = timecat(twoOne()).queryAll(cycles(0, 1));

    ASSERT_EQ(haps.size(), 2U);
    EXPECT_EQ(haps[0].whole, Span(Cycle(0), Cycle(2, 3)));
    EXPECT_EQ(haps[0].part, *haps[0].whole);
    EXPECT_EQ(haps[1].whole, Span(Cycle(2, 3), Cycle(1)));
}

TEST(PatternsTest, Timecat_RepeatsEveryCycle)
{
    const auto haps = timecat(twoOne()).queryAll(cycles(2, 3));

    ASSERT_EQ(haps.size(), 2U);
    EXPECT_EQ(haps[0].whole, Span(Cycle(2), Cycle(2) + Cycle(2, 3)));
    EXPECT_TRUE(haps[0].hasOnset());
}

TEST(PatternsTest, Timecat_KeepsGoingBeforeCycleZero)
{
    const auto haps = timecat(twoOne()).queryAll(cycles(-1, 0));

    ASSERT_EQ(haps.size(), 2U);
    EXPECT_EQ(
        haps[0].whole, Span(Cycle(-1), Cycle(-1) + Cycle(2, 3)));
}

TEST(PatternsTest, Timecat_FragmentsWhatAWindowCuts)
{
    const auto haps = timecat(twoOne())
                          .queryAll(Span(Cycle(1, 3), Cycle(1, 2)));

    ASSERT_EQ(haps.size(), 1U);
    EXPECT_EQ(haps[0].part, Span(Cycle(1, 3), Cycle(1, 2)));
    EXPECT_EQ(haps[0].whole, Span(Cycle(0), Cycle(2, 3)));
    EXPECT_FALSE(haps[0].hasOnset());
}

TEST(PatternsTest, Timecat_MatchesFastcatOnEqualWeights)
{
    std::vector<Slice> slices;
    slices.push_back(Slice{.weight = Cycle(1), .part = note(1)});
    slices.push_back(Slice{.weight = Cycle(1), .part = note(2)});

    const auto shared = timecat(std::move(slices)).queryAll(cycles(0, 2));

    ASSERT_EQ(shared.size(), 4U);
    EXPECT_EQ(shared[0].part, Span(Cycle(), Cycle(1, 2)));
    EXPECT_EQ(
        shared, fastcat({note(1), note(2)}).queryAll(cycles(0, 2)));
}

TEST(PatternsTest, Timecat_TurnsAnInnerAlternationPerCycle)
{
    std::vector<Slice> slices;
    slices.push_back(
        Slice{
            .weight = Cycle(2),
            .part = slowcat({note(1), note(2)})});
    slices.push_back(Slice{.weight = Cycle(1), .part = note(3)});

    const auto pattern = timecat(std::move(slices));

    const auto first = pattern.queryAll(cycles(0, 1));
    const auto second = pattern.queryAll(cycles(1, 2));

    ASSERT_EQ(first.size(), 2U);
    ASSERT_EQ(second.size(), 2U);
    EXPECT_EQ(first[0].value, named(1));
    EXPECT_EQ(second[0].value, named(2));
}

TEST(PatternsTest, Timecat_CarriesAWholelessSignalThrough)
{
    std::vector<Slice> slices;
    slices.push_back(
        Slice{.weight = Cycle(1), .part = steady(named(9))});
    slices.push_back(Slice{.weight = Cycle(2), .part = note(1)});

    const auto haps =
        timecat(std::move(slices)).queryAll(cycles(0, 1));

    ASSERT_EQ(haps.size(), 2U);
    EXPECT_FALSE(haps[0].whole.has_value());
    EXPECT_EQ(haps[0].part, Span(Cycle(0), Cycle(1, 3)));
}

TEST(PatternsTest, Timecat_RefusesNothing)
{
    EXPECT_THROW((void)timecat({}), PatternError);
}

TEST(PatternsTest, Timecat_RefusesASliceOfNoWidth)
{
    std::vector<Slice> slices;
    slices.push_back(Slice{.weight = Cycle(0), .part = note(1)});

    try
    {
        (void)timecat(std::move(slices));
        FAIL() << "a slice of no width should have been refused";
    }
    catch (const PatternError &refused)
    {
        EXPECT_EQ(
            std::string(refused.what()),
            "antwika::pattern: a slice of no width at all cannot hold "
            "a pattern");
    }
}

TEST(PatternsTest, Timecat_RefusesASliceOfNegativeWidth)
{
    std::vector<Slice> slices;
    slices.push_back(Slice{.weight = Cycle(-1), .part = note(1)});
    slices.push_back(Slice{.weight = Cycle(2), .part = note(2)});

    EXPECT_THROW((void)timecat(std::move(slices)), PatternError);
}
