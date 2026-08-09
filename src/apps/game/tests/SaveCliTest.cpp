#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

#include <antwika/cli/CommandLine.hpp>
#include <antwika/cli/CommandLineError.hpp>
#include <antwika/replay/ReplayCli.hpp>

#include "antwika/game/SaveCli.hpp"

using antwika::cli::CommandLine;
using antwika::cli::FlagSpec;
using antwika::cli::parseCommandLine;
using antwika::game::requireRecordableStart;
using antwika::game::saveCliFlags;
using antwika::game::saveCliOptionsFrom;
using antwika::replay::replayCliFlags;

namespace
{
    std::vector<char *> toArgv(std::vector<std::string> &args)
    {
        std::vector<char *> argv;
        argv.reserve(args.size());
        for (auto &arg : args)
        {
            argv.push_back(arg.data());
        }

        return argv;
    }

    std::vector<FlagSpec> wholeTable()
    {
        std::vector<FlagSpec> flags(
            replayCliFlags().begin(), replayCliFlags().end());
        flags.insert(
            flags.end(), saveCliFlags().begin(), saveCliFlags().end());
        return flags;
    }

    CommandLine parse(std::vector<std::string> args)
    {
        auto argv = toArgv(args);
        const auto table = wholeTable();
        return parseCommandLine(
            static_cast<int>(argv.size()), argv.data(), table);
    }
}

TEST(SaveCliTest, SaveCliFlags_OffersBothFlags)
{
    const auto flags = saveCliFlags();

    ASSERT_EQ(flags.size(), 2U);
    EXPECT_EQ(flags[0].name, "--save");
    EXPECT_EQ(flags[1].name, "--load");
    EXPECT_FALSE(flags[0].help.empty());
    EXPECT_FALSE(flags[1].help.empty());
}

TEST(SaveCliTest, SaveCliOptionsFrom_ReadsBothPaths)
{
    const auto options = saveCliOptionsFrom(parse(
        {"antwika_game", "--save", "out.json", "--load", "in.json"}));

    ASSERT_TRUE(options.savePath.has_value());
    EXPECT_EQ(*options.savePath, "out.json");
    ASSERT_TRUE(options.loadPath.has_value());
    EXPECT_EQ(*options.loadPath, "in.json");
}

TEST(SaveCliTest, SaveCliOptionsFrom_LeavesBothUnsetWhenNeitherIsGiven)
{
    const auto options =
        saveCliOptionsFrom(parse({"antwika_game", "--replay", "d.json"}));

    EXPECT_FALSE(options.savePath.has_value());
    EXPECT_FALSE(options.loadPath.has_value());
}

TEST(SaveCliTest, RequireRecordableStart_RefusesALoadedRun)
{
    const auto options = saveCliOptionsFrom(parse(
        {"antwika_game", "--record", "out.jsonl", "--load", "in.json"}));

    EXPECT_THROW(
        requireRecordableStart(options, true),
        antwika::cli::CommandLineError);
}

TEST(SaveCliTest, RequireRecordableStart_LetsAnEmptyStart)
{
    const auto options = saveCliOptionsFrom(
        parse({"antwika_game", "--record", "out.jsonl"}));

    EXPECT_NO_THROW(requireRecordableStart(options, true));
}

TEST(SaveCliTest, RequireRecordableStart_LetsALoadUnrecorded)
{
    const auto options = saveCliOptionsFrom(
        parse({"antwika_game", "--load", "in.json"}));

    EXPECT_NO_THROW(requireRecordableStart(options, false));
}
