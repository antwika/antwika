#include "antwika/pattern/Hap.hpp"

#include <optional>

#include <gtest/gtest.h>

#include "antwika/pattern/Controls.hpp"
#include "antwika/pattern/Cycle.hpp"
#include "antwika/pattern/ParamId.hpp"
#include "antwika/pattern/ParamValue.hpp"
#include "antwika/pattern/Span.hpp"

using antwika::pattern::Controls;
using antwika::pattern::Cycle;
using antwika::pattern::Hap;
using antwika::pattern::Span;

namespace
{
    const Span kWholeCycle(Cycle(), Cycle(1));
} // namespace

// Triggering on a hap rather than an onset is the costly mistake here.
TEST(HapTest, BeginsWhenItsPartStartsWhereItsWholeDoes)
{
    const Hap whole{
        .whole = kWholeCycle, .part = kWholeCycle, .value = Controls{}};

    EXPECT_TRUE(whole.hasOnset());
}

// A window that cut this event in half produced it.
// The event began before the window did, so it must not fire again.
TEST(HapTest, DoesNotBeginWhenItIsTheTailOfAClippedEvent)
{
    const Hap tail{
        .whole = kWholeCycle,
        .part = Span(Cycle(1, 2), Cycle(1)),
        .value = Controls{}};

    EXPECT_FALSE(tail.hasOnset());
}

// A hap with no whole is a signal sampled over the window.
// It is read as a parameter and never triggered.
TEST(HapTest, AContinuousValueNeverBegins)
{
    const Hap signal{
        .whole = std::nullopt,
        .part = kWholeCycle,
        .value = Controls{}};

    EXPECT_FALSE(signal.hasOnset());
}

TEST(HapTest, ComparesOnBothSpansAndTheValue)
{
    const Hap event{
        .whole = kWholeCycle, .part = kWholeCycle, .value = Controls{}};

    EXPECT_EQ(event, event);

    EXPECT_NE(
        event,
        (Hap{
            .whole = std::nullopt,
            .part = kWholeCycle,
            .value = Controls{}}));

    EXPECT_NE(
        event,
        (Hap{
            .whole = kWholeCycle,
            .part = Span(Cycle(1, 2), Cycle(1)),
            .value = Controls{}}));
}

TEST(HapTest, ComparesOnWhatItCarriesAsWellAsWhenItIs)
{
    const Hap quiet{
        .whole = kWholeCycle, .part = kWholeCycle, .value = Controls{}};

    const Hap loud{
        .whole = kWholeCycle,
        .part = kWholeCycle,
        .value = Controls(antwika::pattern::ParamId{1},
                          antwika::pattern::ParamValue(1))};

    EXPECT_NE(quiet, loud);
}
