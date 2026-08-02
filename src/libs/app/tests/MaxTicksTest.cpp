#include <gtest/gtest.h>

#include <optional>

#include "antwika/app/MaxTicks.hpp"

using antwika::app::maxTicksOf;

namespace
{
    constexpr std::optional<antwika::time::Tick> kFallback{90000};
} // namespace

TEST(MaxTicksTest, AnAbsentFlagLeavesTheFallback)
{
    EXPECT_EQ(maxTicksOf(std::nullopt, kFallback), kFallback);
}

TEST(MaxTicksTest, AReadableValueBecomesTheCap)
{
    EXPECT_EQ(
        maxTicksOf("1200", kFallback),
        std::optional<antwika::time::Tick>{1200});
}

// Zero means no cap at all.
// That is what somebody at a real window asks for.
TEST(MaxTicksTest, ZeroMeansNoCapAtAll)
{
    EXPECT_EQ(maxTicksOf("0", kFallback), std::nullopt);
}

// Matching how these applications treat every other malformed flag.
TEST(MaxTicksTest, AnUnreadableValueLeavesTheFallback)
{
    EXPECT_EQ(maxTicksOf("soon", kFallback), kFallback);
    EXPECT_EQ(maxTicksOf("12x", kFallback), kFallback);
    EXPECT_EQ(
        maxTicksOf("99999999999999999999999", kFallback), kFallback);
}
