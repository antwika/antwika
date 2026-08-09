#include <gtest/gtest.h>

#include <optional>

#include "antwika/pattern/Hap.hpp"
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
}

TEST(HapTest, HasOnset_IsTrueWhenPartMeetsWhole)
{
    const Hap whole{
        .whole = kWholeCycle, .part = kWholeCycle, .value = Controls{}};

    EXPECT_TRUE(whole.hasOnset());
}

TEST(HapTest, HasOnset_IsFalseForAClippedTail)
{
    const Hap tail{
        .whole = kWholeCycle,
        .part = Span(Cycle(1, 2), Cycle(1)),
        .value = Controls{}};

    EXPECT_FALSE(tail.hasOnset());
}

TEST(HapTest, HasOnset_IsFalseForAContinuousValue)
{
    const Hap signal{
        .whole = std::nullopt,
        .part = kWholeCycle,
        .value = Controls{}};

    EXPECT_FALSE(signal.hasOnset());
}

TEST(HapTest, OperatorEquals_ComparesBothSpansAndTheValue)
{
    const Hap event{
        .whole = kWholeCycle, .part = kWholeCycle, .value = Controls{}};

    const auto twin = event;
    EXPECT_EQ(event, twin);

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

TEST(HapTest, OperatorEquals_ComparesWhatAndWhen)
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
