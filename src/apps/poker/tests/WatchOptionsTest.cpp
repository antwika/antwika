#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <vector>

#include "antwika/poker/WatchOptions.hpp"

using antwika::poker::parseWatchOptions;
using antwika::poker::WatchOptions;
using namespace std::chrono_literals;

namespace
{
    // parseWatchOptions takes main()'s argv, so a test has to build one.
    [[nodiscard]] WatchOptions parse(std::vector<const char *> args)
    {
        std::vector<char *> argv;
        argv.reserve(args.size());
        for (const auto *arg : args)
        {
            argv.push_back(const_cast<char *>(arg));
        }
        return parseWatchOptions(
            static_cast<int>(argv.size()), argv.data());
    }
} // namespace

TEST(WatchOptionsTest, ParseWatchOptions_DefaultsToNoDelay)
{
    EXPECT_EQ(parse({"antwika_poker"}).tickDelay, 0ms);
}

TEST(WatchOptionsTest, ParseWatchOptions_ReadsTheDelay)
{
    EXPECT_EQ(
        parse({"antwika_poker", "--tick-delay-ms", "150"}).tickDelay, 150ms);
}

TEST(WatchOptionsTest, ParseWatchOptions_AcceptsAnExplicitZero)
{
    EXPECT_EQ(
        parse({"antwika_poker", "--tick-delay-ms", "0"}).tickDelay, 0ms);
}

TEST(WatchOptionsTest, ParseWatchOptions_IgnoresTheFlagWithNoValue)
{
    EXPECT_EQ(parse({"antwika_poker", "--tick-delay-ms"}).tickDelay, 0ms);
}

TEST(WatchOptionsTest, ParseWatchOptions_IgnoresAValueThatIsNotANumber)
{
    EXPECT_EQ(
        parse({"antwika_poker", "--tick-delay-ms", "soon"}).tickDelay, 0ms);
}

TEST(WatchOptionsTest, ParseWatchOptions_IgnoresATrailingNonNumber)
{
    EXPECT_EQ(
        parse({"antwika_poker", "--tick-delay-ms", "150x"}).tickDelay, 0ms);
}

TEST(WatchOptionsTest, ParseWatchOptions_IgnoresANegativeDelay)
{
    EXPECT_EQ(
        parse({"antwika_poker", "--tick-delay-ms", "-5"}).tickDelay, 0ms);
}

TEST(WatchOptionsTest, ParseWatchOptions_IgnoresFlagsItDoesNotKnow)
{
    // The replay flags are parsed elsewhere, over the same argv.
    EXPECT_EQ(
        parse({"antwika_poker", "--replay", "demo.json"}).tickDelay, 0ms);
}

TEST(WatchOptionsTest, ParseWatchOptions_ReadsTheDelayAmongOtherFlags)
{
    EXPECT_EQ(
        parse(
            {"antwika_poker",
             "--replay",
             "demo.json",
             "--tick-delay-ms",
             "80"})
            .tickDelay,
        80ms);
}
