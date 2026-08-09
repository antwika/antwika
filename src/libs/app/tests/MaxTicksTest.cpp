#include <gtest/gtest.h>

#include <optional>
#include <string_view>

#include "antwika/app/MaxTicks.hpp"

using antwika::app::maxTicksOf;

namespace
{
    constexpr std::optional<antwika::time::Tick> kFallback{90000};
}

TEST(MaxTicksTest, MaxTicksOf_LeavesTheFallbackWhenAbsent)
{
    EXPECT_EQ(maxTicksOf(std::nullopt, kFallback), kFallback);
}

TEST(MaxTicksTest, MaxTicksOf_TakesAReadableValueAsTheCap)
{
    EXPECT_EQ(
        maxTicksOf("1200", kFallback),
        std::optional<antwika::time::Tick>{1200});
}

TEST(MaxTicksTest, MaxTicksOf_ReadsNoFurtherThanTheValueEnds)
{
    constexpr std::string_view longer{"1200"};

    EXPECT_EQ(
        maxTicksOf(longer.substr(0, 2), kFallback),
        std::optional<antwika::time::Tick>{12});
}

TEST(MaxTicksTest, MaxTicksOf_TakesZeroAsNoCap)
{
    EXPECT_EQ(maxTicksOf("0", kFallback), std::nullopt);
}

TEST(MaxTicksTest, MaxTicksOf_LeavesTheFallbackWhenUnreadable)
{
    EXPECT_EQ(maxTicksOf("soon", kFallback), kFallback);
    EXPECT_EQ(maxTicksOf("12x", kFallback), kFallback);
    EXPECT_EQ(
        maxTicksOf("99999999999999999999999", kFallback), kFallback);
}
