#include <gtest/gtest.h>

#include <memory>

#include "antwika/pattern/Pattern.hpp"
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

TEST(PatternTest, Ctor_RefusesToWrapNothing)
{
    EXPECT_THROW(Pattern(std::shared_ptr<const IPattern>{}), PatternError);
}

TEST(PatternTest, Query_WritesIntoASinkTheCallerOwns)
{
    HapBuffer buffer;

    pure(Controls{}).query(Span(Cycle(), Cycle(1)), buffer);

    EXPECT_EQ(buffer.haps().size(), 1U);
}

TEST(PatternTest, QueryAll_WritesIntoAVector)
{
    const auto haps = pure(Controls{}).queryAll(Span(Cycle(), Cycle(2)));

    EXPECT_EQ(haps.size(), 2U);
}

TEST(PatternTest, Copy_SharesOneImplementation)
{
    const auto original = pure(Controls{});
    const auto copy = original;

    const auto copied = copy.queryAll(Span(Cycle(), Cycle(1)));

    ASSERT_EQ(copied.size(), 1U);
    EXPECT_EQ(copied, original.queryAll(Span(Cycle(), Cycle(1))));
}
