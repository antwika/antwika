#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

#include <antwika/cli/CommandLine.hpp>
#include <antwika/replay/ReplayCli.hpp>

#include "antwika/game/SaveCli.hpp"

using antwika::cli::CommandLine;
using antwika::cli::FlagSpec;
using antwika::cli::parseCommandLine;
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

    // The whole table an app parses against.
    // The replay flags plus this app's own, parsed in one pass.
    // Parsing twice would make each pass refuse the other's flags.
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
} // namespace

TEST(SaveCliTest, OffersBothFlags)
{
    const auto flags = saveCliFlags();

    ASSERT_EQ(flags.size(), 2U);
    EXPECT_EQ(flags[0].name, "--save");
    EXPECT_EQ(flags[1].name, "--load");
    EXPECT_FALSE(flags[0].help.empty());
    EXPECT_FALSE(flags[1].help.empty());
}

TEST(SaveCliTest, ReadsBothPaths)
{
    const auto options = saveCliOptionsFrom(parse(
        {"antwika_game", "--save", "out.json", "--load", "in.json"}));

    ASSERT_TRUE(options.savePath.has_value());
    EXPECT_EQ(*options.savePath, "out.json");
    ASSERT_TRUE(options.loadPath.has_value());
    EXPECT_EQ(*options.loadPath, "in.json");
}

TEST(SaveCliTest, LeavesBothUnsetWhenNeitherIsGiven)
{
    const auto options =
        saveCliOptionsFrom(parse({"antwika_game", "--replay", "d.json"}));

    EXPECT_FALSE(options.savePath.has_value());
    EXPECT_FALSE(options.loadPath.has_value());
}
