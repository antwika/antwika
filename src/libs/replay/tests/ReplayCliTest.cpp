#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <vector>

#include <unistd.h>

#include <antwika/cli/CommandLineError.hpp>
#include <antwika/testing/ScratchPath.hpp>

#include "antwika/replay/ReplayCli.hpp"
#include "antwika/replay/ReplayFormatError.hpp"

using antwika::cli::CommandLineError;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::replay::loadReplayFile;
using antwika::replay::openReplayFile;
using antwika::replay::ReplayFormatError;
using antwika::replay::saveReplayFile;

namespace
{
    std::vector<char *> toArgv(const std::vector<std::string> &args)
    {
        std::vector<char *> argv;
        argv.reserve(args.size());
        for (const auto &arg : args)
        {
            argv.push_back(const_cast<char *>(arg.c_str()));
        }
        return argv;
    }

    antwika::replay::ReplayCliOptions optionsFrom(
        const std::vector<std::string> &args)
    {
        auto argv = toArgv(args);
        return antwika::replay::replayCliOptionsFrom(
            antwika::cli::parseCommandLine(
                static_cast<int>(argv.size()),
                argv.data(),
                antwika::replay::replayCliFlags()));
    }
}

TEST(ReplayCliTest, Ctor_DefaultsToNoPathsAndNoHelp)
{
    const antwika::replay::ReplayCliOptions options;

    EXPECT_FALSE(options.recordPath.has_value());
    EXPECT_FALSE(options.replayPath.has_value());
    EXPECT_FALSE(options.helpRequested);
}

TEST(ReplayCliTest, OptionsFrom_ReturnsNoPathsWithoutFlags)
{
    const auto options = optionsFrom({"antwika_app"});

    EXPECT_FALSE(options.recordPath.has_value());
    EXPECT_FALSE(options.replayPath.has_value());
}

TEST(ReplayCliTest, OptionsFrom_ReadsBothRecordAndReplayPaths)
{
    const auto options = optionsFrom(
        {"antwika_app", "--record", "out.jsonl", "--replay", "in.jsonl"});

    ASSERT_TRUE(options.recordPath.has_value());
    EXPECT_EQ(*options.recordPath, "out.jsonl");
    ASSERT_TRUE(options.replayPath.has_value());
    EXPECT_EQ(*options.replayPath, "in.jsonl");
}

TEST(ReplayCliTest, OptionsFrom_RefusesAValuelessReplayFlag)
{
    EXPECT_THROW(
        (void)optionsFrom({"antwika_app", "--replay"}),
        CommandLineError);
}

TEST(ReplayCliTest, OptionsFrom_RefusesAValuelessRecordFlag)
{
    EXPECT_THROW(
        (void)optionsFrom({"antwika_app", "--record"}),
        CommandLineError);
}

TEST(ReplayCliTest, OptionsFrom_RefusesAMisspeltFlag)
{
    EXPECT_THROW(
        (void)optionsFrom({"antwika_app", "--replya", "demo.json"}),
        CommandLineError);
}

TEST(ReplayCliTest, OptionsFrom_ReportsThatHelpWasAsked)
{
    const auto options = optionsFrom({"antwika_app", "--help"});

    EXPECT_TRUE(options.helpRequested);
}

TEST(ReplayCliTest, ReplayCliFlags_AreTheTwoEveryAppTakes)
{
    const auto flags = antwika::replay::replayCliFlags();

    ASSERT_EQ(flags.size(), 2U);
    EXPECT_EQ(flags[0].name, "--record");
    EXPECT_EQ(flags[1].name, "--replay");
}

TEST(ReplayCliTest, LoadReplayFile_DecodesASavedDocument)
{
    antwika::testing::ScratchFile file("antwika_replay_cli_load_test.jsonl");
    {
        std::ofstream out(file.string());
        out << R"({"magic":"antwika-replay","version":1,"events":)"
            << R"([{"tick":0,"event":{"name":"life.step","payload":""}}])"
            << "}";
    }

    const auto events = loadReplayFile(file.string());

    EXPECT_EQ(
        events,
        (std::vector<TickEvent>{
            TickEvent{.tick = 0, .event = Event{.name = "life.step"}},
        }));
}

TEST(ReplayCliTest, LoadReplayFile_ThrowsOnAnUnparsableFile)
{
    antwika::testing::ScratchFile file(
        "antwika_replay_cli_load_malformed_test.jsonl");
    {
        std::ofstream out(file.string());
        out << "not a replay document";
    }

    EXPECT_THROW((void)loadReplayFile(file.string()), ReplayFormatError);
}

TEST(ReplayCliTest, SaveReplayFile_FiltersOutBuiltInTicks)
{
    antwika::testing::ScratchFile file("antwika_replay_cli_save_test.jsonl");
    std::vector<TickEvent> events{
        TickEvent{.tick = 0, .event = Event{.name = "engine.tick"}},
        TickEvent{
            .tick = 0,
            .event = Event{.name = "game.score_increment", .payload = "1"},
        },
        TickEvent{.tick = 1, .event = Event{.name = "engine.tick"}},
    };

    saveReplayFile(events, file.string());

    const auto reloaded = loadReplayFile(file.string());
    EXPECT_EQ(
        reloaded,
        (std::vector<TickEvent>{
            TickEvent{
                .tick = 0,
                .event =
                    Event{.name = "game.score_increment", .payload = "1"},
            },
        }));
}

TEST(ReplayCliTest, OpenReplayFile_GivesAnAppendableStream)
{
    const antwika::testing::ScratchFile file(
        "antwika_replay_cli_open_test.jsonl");

    {
        std::ofstream out = openReplayFile(file.string());
        EXPECT_TRUE(out.is_open());
    }

    EXPECT_TRUE(std::filesystem::exists(file.string()));
}

TEST(ReplayCliTest, OpenReplayFile_ThrowsOnAnUnusablePath)
{
    const std::string path =
        (std::filesystem::temp_directory_path() / "antwika-no-such-dir"
         / "out.jsonl")
            .string();

    EXPECT_THROW(
        std::ignore = openReplayFile(path), ReplayFormatError);
}

TEST(ReplayCliTest, LoadReplayFile_NamesAMissingFile)
{
    const antwika::testing::ScratchFile file(
        "antwika_replay_cli_load_absent_test.jsonl");

    try
    {
        (void)loadReplayFile(file.string());
        FAIL() << "loading an absent replay should have thrown";
    }
    catch (const ReplayFormatError &error)
    {
        const std::string message = error.what();
        EXPECT_NE(message.find("could not open"), std::string::npos)
            << message;
        EXPECT_NE(message.find(file.string()), std::string::npos)
            << message;
    }
}

TEST(ReplayCliTest, SaveReplayFile_NamesAnUnwritablePath)
{
    const std::string path =
        (std::filesystem::temp_directory_path() / "antwika-no-such-dir"
         / "out.jsonl")
            .string();

    try
    {
        saveReplayFile({}, path);
        FAIL() << "saving into an absent directory should have thrown";
    }
    catch (const ReplayFormatError &error)
    {
        const std::string message = error.what();
        EXPECT_NE(message.find("could not open"), std::string::npos)
            << message;
        EXPECT_NE(message.find(path), std::string::npos) << message;
    }
}
