#include <gtest/gtest.h>

#include <chrono>
#include <vector>

#include <antwika/cli/CommandLine.hpp>
#include <antwika/cli/CommandLineError.hpp>
#include <antwika/cli/FlagSpec.hpp>
#include <antwika/replay/ReplayCli.hpp>

#include "antwika/poker/WatchOptions.hpp"

using antwika::cli::CommandLineError;
using antwika::poker::watchFlags;
using antwika::poker::watchOptionsFrom;
using antwika::poker::WatchOptions;
using namespace std::chrono_literals;

namespace
{
    // The app's flags are parsed with the replay ones, in one pass.
    // So a test builds the table its main() hands to runRecorded().
    [[nodiscard]] WatchOptions parse(std::vector<const char *> args)
    {
        std::vector<char *> argv;
        argv.reserve(args.size());
        for (const auto *arg : args)
        {
            argv.push_back(const_cast<char *>(arg));
        }

        std::vector<antwika::cli::FlagSpec> table(
            antwika::replay::replayCliFlags().begin(),
            antwika::replay::replayCliFlags().end());
        table.insert(
            table.end(), watchFlags().begin(), watchFlags().end());

        return watchOptionsFrom(antwika::cli::parseCommandLine(
            static_cast<int>(argv.size()), argv.data(), table));
    }
} // namespace

TEST(WatchOptionsTest, WatchOptionsFrom_DefaultsToASecondPerAction)
{
    // One tick is one poker action, so this is the pace of the game.
    EXPECT_EQ(parse({"antwika_poker"}).tickDelay, 1000ms);
}

TEST(WatchOptionsTest, WatchOptionsFrom_HoldsNoFinalFrameByDefault)
{
    // The default pacing applies to the terminal run too.
    // Holding there would hang under a backend reporting no close.
    EXPECT_FALSE(parse({"antwika_poker"}).holdFinalFrame);
}

TEST(WatchOptionsTest, WatchOptionsFrom_HoldsTheFinalFrameWhenAsked)
{
    EXPECT_TRUE(
        parse({"antwika_poker", "--tick-delay-ms", "150"}).holdFinalFrame);
}

TEST(WatchOptionsTest, WatchOptionsFrom_HoldsNoFinalFrameForAnExplicitZero)
{
    EXPECT_FALSE(
        parse({"antwika_poker", "--tick-delay-ms", "0"}).holdFinalFrame);
}

TEST(WatchOptionsTest, WatchOptionsFrom_ReadsTheDelay)
{
    EXPECT_EQ(
        parse({"antwika_poker", "--tick-delay-ms", "150"}).tickDelay, 150ms);
}

TEST(WatchOptionsTest, WatchOptionsFrom_AcceptsAnExplicitZero)
{
    EXPECT_EQ(
        parse({"antwika_poker", "--tick-delay-ms", "0"}).tickDelay, 0ms);
}

TEST(WatchOptionsTest, WatchOptionsFrom_RefusesTheFlagWithNoValue)
{
    // Ignored before the one parse.
    // A flag with no value is now the command line being wrong.
    EXPECT_THROW(
        (void)parse({"antwika_poker", "--tick-delay-ms"}),
        CommandLineError);
}

TEST(WatchOptionsTest, WatchOptionsFrom_IgnoresAValueThatIsNotANumber)
{
    EXPECT_EQ(
        parse({"antwika_poker", "--tick-delay-ms", "soon"}).tickDelay,
        1000ms);
}

TEST(WatchOptionsTest, WatchOptionsFrom_IgnoresATrailingNonNumber)
{
    EXPECT_EQ(
        parse({"antwika_poker", "--tick-delay-ms", "150x"}).tickDelay,
        1000ms);
}

TEST(WatchOptionsTest, WatchOptionsFrom_IgnoresANegativeDelay)
{
    EXPECT_EQ(
        parse({"antwika_poker", "--tick-delay-ms", "-5"}).tickDelay, 1000ms);
}

TEST(WatchOptionsTest, WatchOptionsFrom_LeavesTheReplayFlagsAlone)
{
    EXPECT_EQ(
        parse({"antwika_poker", "--replay", "demo.json"}).tickDelay,
        1000ms);
}

TEST(WatchOptionsTest, WatchOptionsFrom_ReadsTheDelayAmongOtherFlags)
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

TEST(WatchOptionsTest, WatchFlags_DocumentsTheDelayFlag)
{
    ASSERT_EQ(watchFlags().size(), 1U);
    EXPECT_EQ(watchFlags().front().name, "--tick-delay-ms");
    EXPECT_FALSE(watchFlags().front().valueName.empty());
    EXPECT_FALSE(watchFlags().front().help.empty());
}
