#include <gtest/gtest.h>

#include <optional>
#include <string_view>

#include "antwika/app/ParseMaxTicks.hpp"

using antwika::app::getParseMaxTicks;

namespace
{
    constexpr std::optional<antwika::time::Tick> kFallbackTick{90000};
}

TEST(ParseMaxTicksTest, ParseMaxTicks_LeavesTheFallbackWhenAbsent)
{
    EXPECT_EQ(getParseMaxTicks(std::nullopt, kFallbackTick), kFallbackTick);
}

TEST(ParseMaxTicksTest, ParseMaxTicks_TakesAReadableValueAsTheCap)
{
    EXPECT_EQ(
        getParseMaxTicks("1200", kFallbackTick),
        std::optional<antwika::time::Tick>{1200});
}

TEST(ParseMaxTicksTest, ParseMaxTicks_ReadsNoFurtherThanTheValueEnds)
{
    constexpr std::string_view longer{"1200"};

    EXPECT_EQ(
        getParseMaxTicks(longer.substr(0, 2), kFallbackTick),
        std::optional<antwika::time::Tick>{12});
}

TEST(ParseMaxTicksTest, ParseMaxTicks_TakesZeroAsNoCap)
{
    EXPECT_EQ(getParseMaxTicks("0", kFallbackTick), std::nullopt);
}

TEST(ParseMaxTicksTest, ParseMaxTicks_LeavesTheFallbackWhenUnreadable)
{
    EXPECT_EQ(getParseMaxTicks("soon", kFallbackTick), kFallbackTick);
    EXPECT_EQ(getParseMaxTicks("12x", kFallbackTick), kFallbackTick);
    EXPECT_EQ(
        getParseMaxTicks("99999999999999999999999", kFallbackTick), kFallbackTick);
}
