#include "antwika/pattern/HapBuffer.hpp"

#include <gtest/gtest.h>

#include "antwika/pattern/Controls.hpp"
#include "antwika/pattern/Cycle.hpp"
#include "antwika/pattern/Hap.hpp"
#include "antwika/pattern/Span.hpp"

using antwika::pattern::Controls;
using antwika::pattern::Cycle;
using antwika::pattern::Hap;
using antwika::pattern::HapBuffer;
using antwika::pattern::Span;

namespace
{
    [[nodiscard]] Hap eventAt(Cycle begin, Cycle end)
    {
        const Span span(begin, end);

        return Hap{.whole = span, .part = span, .value = Controls{}};
    }
} // namespace

TEST(HapBufferTest, StartsEmpty)
{
    const HapBuffer buffer;

    EXPECT_TRUE(buffer.haps().empty());
}

TEST(HapBufferTest, KeepsWhatItWasHandedInOrder)
{
    HapBuffer buffer;

    buffer.accept(eventAt(Cycle(), Cycle(1)));
    buffer.accept(eventAt(Cycle(1), Cycle(2)));

    ASSERT_EQ(buffer.haps().size(), 2U);
    EXPECT_EQ(buffer.haps()[0], eventAt(Cycle(), Cycle(1)));
    EXPECT_EQ(buffer.haps()[1], eventAt(Cycle(1), Cycle(2)));
}

// A sequencer reserves once and clears each tick.
// Its hot path allocates nothing.
TEST(HapBufferTest, ForgetsWhatItHeldWithoutGivingUpItsSpace)
{
    HapBuffer buffer;
    buffer.reserve(16);

    buffer.accept(eventAt(Cycle(), Cycle(1)));
    buffer.clear();

    EXPECT_TRUE(buffer.haps().empty());
    EXPECT_GE(buffer.haps().capacity(), 16U);
}
