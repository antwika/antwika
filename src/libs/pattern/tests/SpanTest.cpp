#include <gtest/gtest.h>

#include <cstddef>

#include "antwika/pattern/Span.hpp"
#include "antwika/pattern/Cycle.hpp"
#include "antwika/pattern/PatternError.hpp"

using antwika::pattern::Cycle;
using antwika::pattern::PatternError;
using antwika::pattern::Span;

TEST(SpanTest, Ctor_HoldsTheTwoEndsItWasGiven)
{
    const Span first(Cycle(), Cycle(1));

    EXPECT_EQ(first.begin(), Cycle());
    EXPECT_EQ(first.end(), Cycle(1));
    EXPECT_EQ(first.length(), Cycle(1));
}

TEST(SpanTest, Ctor_RefusesAStretchOfNoTime)
{
    EXPECT_THROW(Span(Cycle(1), Cycle(1)), PatternError);
    EXPECT_THROW(Span(Cycle(1), Cycle()), PatternError);
}

TEST(SpanTest, Intersect_FindsWhatTwoStretchesShare)
{
    const Span first(Cycle(), Cycle(1));
    const Span second(Cycle(1, 2), Cycle(2));

    const auto shared = first.intersect(second);

    ASSERT_TRUE(shared.has_value());
    EXPECT_EQ(*shared, Span(Cycle(1, 2), Cycle(1)));
}

TEST(SpanTest, Intersect_GivesNothingForTouchingSpans)
{
    const Span first(Cycle(), Cycle(1));
    const Span second(Cycle(1), Cycle(2));

    EXPECT_FALSE(first.intersect(second).has_value());
}

TEST(SpanTest, Intersect_GivesNothingForSpansApart)
{
    const Span first(Cycle(), Cycle(1));
    const Span far(Cycle(4), Cycle(5));

    EXPECT_FALSE(first.intersect(far).has_value());
}

TEST(SpanTest, SpanCycles_SplitsOnePiecePerCycle)
{
    const Span across(Cycle(1, 2), Cycle(5, 2));

    const auto pieces = across.spanCycles();

    ASSERT_EQ(pieces.size(), 3U);
    EXPECT_EQ(pieces[0], Span(Cycle(1, 2), Cycle(1)));
    EXPECT_EQ(pieces[1], Span(Cycle(1), Cycle(2)));
    EXPECT_EQ(pieces[2], Span(Cycle(2), Cycle(5, 2)));
}

TEST(SpanTest, SpanCycles_LeavesOneCycleUnchanged)
{
    const Span whole(Cycle(), Cycle(1));

    const auto pieces = whole.spanCycles();

    ASSERT_EQ(pieces.size(), 1U);
    EXPECT_EQ(pieces[0], whole);
}

TEST(SpanTest, SpanCycles_CoversExactlyWhatWasSplit)
{
    const Span across(Cycle(-3, 2), Cycle(3, 2));

    const auto pieces = across.spanCycles();

    ASSERT_EQ(pieces.size(), 4U);
    EXPECT_EQ(pieces.front().begin(), across.begin());
    EXPECT_EQ(pieces.back().end(), across.end());

    for (std::size_t piece = 1; piece < pieces.size(); ++piece)
    {
        EXPECT_EQ(pieces[piece].begin(), pieces[piece - 1].end());
    }
}

TEST(SpanTest, OperatorEquals_ComparesOnBothEnds)
{
    const Span first(Cycle(), Cycle(1));

    EXPECT_EQ(first, Span(Cycle(), Cycle(1)));
    EXPECT_NE(first, Span(Cycle(), Cycle(2)));
    EXPECT_NE(first, Span(Cycle(1, 2), Cycle(1)));
}
