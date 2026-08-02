#include "antwika/pattern/Pattern.hpp"

#include <memory>

#include <gtest/gtest.h>

#include "antwika/pattern/Controls.hpp"
#include "antwika/pattern/Cycle.hpp"
#include "antwika/pattern/HapBuffer.hpp"
#include "antwika/pattern/PatternError.hpp"
#include "antwika/pattern/Patterns.hpp"
#include "antwika/pattern/Span.hpp"

using antwika::pattern::Controls;
using antwika::pattern::Cycle;
using antwika::pattern::HapBuffer;
using antwika::pattern::IPattern;
using antwika::pattern::Pattern;
using antwika::pattern::PatternError;
using antwika::pattern::pure;
using antwika::pattern::Span;

TEST(PatternTest, RefusesToWrapNothing)
{
    EXPECT_THROW(Pattern(std::shared_ptr<const IPattern>{}), PatternError);
}

TEST(PatternTest, QueriesIntoASinkTheCallerOwns)
{
    HapBuffer buffer;

    pure(Controls{}).query(Span(Cycle(), Cycle(1)), buffer);

    EXPECT_EQ(buffer.haps().size(), 1U);
}

TEST(PatternTest, QueriesIntoAVectorForConvenience)
{
    const auto haps = pure(Controls{}).queryAll(Span(Cycle(), Cycle(2)));

    EXPECT_EQ(haps.size(), 2U);
}

// A pattern is a value, so an expression owns everything inside it.
// Nothing has a lifetime rule written in a comment.
TEST(PatternTest, CopiesShareOneImplementation)
{
    const auto original = pure(Controls{});
    const auto copy = original;

    EXPECT_EQ(
        copy.queryAll(Span(Cycle(), Cycle(1))),
        original.queryAll(Span(Cycle(), Cycle(1))));
}
