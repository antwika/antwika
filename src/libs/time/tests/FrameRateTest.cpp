#include <gtest/gtest.h>

#include <chrono>

#include "antwika/time/FrameRate.hpp"

using antwika::time::FrameRate;
using antwika::time::formatFrameRate;
using antwika::time::kFrameSampleCount;
using antwika::time::formatFrameTime;

namespace
{

    constexpr float kTolerance = 0.01F;

    constexpr std::chrono::nanoseconds kSixtieth{16'666'666};

}

TEST(FrameRateTest, PerSecond_SaysNothingBeforeAFrameIsCounted)
{
    EXPECT_FALSE(FrameRate{}.perSecond().has_value());
}

TEST(FrameRateTest, PerSecond_CountsTheFramesOfASecond)
{
    FrameRate rate;

    rate.record(kSixtieth);

    ASSERT_TRUE(rate.perSecond().has_value());
    EXPECT_NEAR(*rate.perSecond(), 60.0F, kTolerance);
}

TEST(FrameRateTest, PerSecond_TakesItsCountOverTheFramesItWatches)
{
    FrameRate rate;

    for (std::size_t frame = 0; frame < kFrameSampleCount;
         ++frame)
    {
        rate.record(std::chrono::nanoseconds{100'000'000});
    }

    for (std::size_t frame = 0; frame < kFrameSampleCount;
         ++frame)
    {
        rate.record(std::chrono::nanoseconds{10'000'000});
    }

    ASSERT_TRUE(rate.perSecond().has_value());
    EXPECT_NEAR(*rate.perSecond(), 100.0F, kTolerance);
}

TEST(FrameRateTest, Record_LeavesOutAFrameThatTookNoTimeAtAll)
{
    FrameRate rate;

    rate.record(std::chrono::nanoseconds{0});
    rate.record(std::chrono::nanoseconds{-5});

    EXPECT_FALSE(rate.perSecond().has_value());
}

TEST(FrameRateTest, FormatFrameRate_WritesTheCountRoundedToAWholeFrame)
{
    EXPECT_EQ(formatFrameRate(59.6F), "60 fps");
    EXPECT_EQ(formatFrameRate(12.2F), "12 fps");
}

TEST(FrameRateTest, FormatFrameRate_WritesNoCountAsADash)
{
    EXPECT_EQ(formatFrameRate(std::nullopt), "- fps");
}

TEST(FrameRateTest, AverageFrameTime_SaysNothingBeforeAFrameIsCounted)
{
    EXPECT_FALSE(FrameRate{}.averageFrameTime().has_value());
}

TEST(FrameRateTest, AverageFrameTime_TakesTheSpansItWatchesTogether)
{
    FrameRate rate;

    rate.record(std::chrono::nanoseconds{4'000'000});
    rate.record(std::chrono::nanoseconds{8'000'000});

    ASSERT_TRUE(rate.averageFrameTime().has_value());
    EXPECT_EQ(*rate.averageFrameTime(), std::chrono::nanoseconds{6'000'000});
}

TEST(FrameRateTest, FormatFrameTime_WritesTheSpanInTenthsOfAMillisecond)
{
    EXPECT_EQ(formatFrameTime(std::chrono::nanoseconds{7'440'000}), "7.4 ms");
    EXPECT_EQ(
        formatFrameTime(std::chrono::nanoseconds{16'000'000}), "16.0 ms");
}

TEST(FrameRateTest, FormatFrameTime_WritesNoSpanAsADash)
{
    EXPECT_EQ(formatFrameTime(std::nullopt), "- ms");
}
