#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "antwika/replay/CommandLineError.hpp"
#include "antwika/replay/ReplayCli.hpp"
#include "antwika/replay/ReplayFormatError.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::replay::CommandLineError;
using antwika::replay::loadReplayFile;
using antwika::replay::parseReplayCliOptions;
using antwika::replay::ReplayFormatError;
using antwika::replay::saveReplayFile;

namespace
{
    // Removes its backing file on scope exit.
    // That way a failing assertion never leaves stray temp files behind.
    class ScratchFile
    {
    public:
        explicit ScratchFile(std::string_view name)
            : path(std::filesystem::temp_directory_path() / name)
        {
        }

        ~ScratchFile()
        {
            // The error_code overload, not the throwing one.
            // A destructor is implicitly noexcept.
            // A throwing removal would take the whole binary down.
            std::error_code ignored;
            std::filesystem::remove(path, ignored);
        }

        ScratchFile(const ScratchFile &) = delete;
        ScratchFile(ScratchFile &&) = delete;
        ScratchFile &operator=(const ScratchFile &) = delete;
        ScratchFile &operator=(ScratchFile &&) = delete;

        [[nodiscard]] std::string string() const
        {
            return path.string();
        }

    private:
        std::filesystem::path path;
    };

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
} // namespace

TEST(ReplayCliTest, ParseReturnsNoPathsWhenNoFlagsAreGiven)
{
    std::vector<std::string> args{"antwika_app"};
    auto argv = toArgv(args);

    const auto options = parseReplayCliOptions(
        static_cast<int>(argv.size()), argv.data());

    EXPECT_FALSE(options.recordPath.has_value());
    EXPECT_FALSE(options.replayPath.has_value());
}

TEST(ReplayCliTest, ParseReadsBothRecordAndReplayPaths)
{
    std::vector<std::string> args{
        "antwika_app", "--record", "out.json", "--replay", "in.json"};
    auto argv = toArgv(args);

    const auto options = parseReplayCliOptions(
        static_cast<int>(argv.size()), argv.data());

    ASSERT_TRUE(options.recordPath.has_value());
    EXPECT_EQ(*options.recordPath, "out.json");
    ASSERT_TRUE(options.replayPath.has_value());
    EXPECT_EQ(*options.replayPath, "in.json");
}

TEST(ReplayCliTest, ParseRefusesATrailingReplayFlagMissingItsValue)
{
    std::vector<std::string> args{"antwika_app", "--replay"};
    auto argv = toArgv(args);

    EXPECT_THROW(
        (void)parseReplayCliOptions(
            static_cast<int>(argv.size()), argv.data()),
        CommandLineError);
}

TEST(ReplayCliTest, ParseRefusesATrailingRecordFlagMissingItsValue)
{
    std::vector<std::string> args{"antwika_app", "--record"};
    auto argv = toArgv(args);

    EXPECT_THROW(
        (void)parseReplayCliOptions(
            static_cast<int>(argv.size()), argv.data()),
        CommandLineError);
}

// The defect this parser exists for.
// `--replya demo.json` used to start an empty session in silence.
TEST(ReplayCliTest, ParseRefusesAMisspeltFlag)
{
    std::vector<std::string> args{"antwika_app", "--replya", "demo.json"};
    auto argv = toArgv(args);

    EXPECT_THROW(
        (void)parseReplayCliOptions(
            static_cast<int>(argv.size()), argv.data()),
        CommandLineError);
}

TEST(ReplayCliTest, ParseReportsThatHelpWasAskedFor)
{
    std::vector<std::string> args{"antwika_app", "--help"};
    auto argv = toArgv(args);

    const auto options = parseReplayCliOptions(
        static_cast<int>(argv.size()), argv.data());

    EXPECT_TRUE(options.helpRequested);
}

TEST(ReplayCliTest, ReplayCliFlagsAreTheTwoEveryAppTakes)
{
    const auto flags = antwika::replay::replayCliFlags();

    ASSERT_EQ(flags.size(), 2U);
    EXPECT_EQ(flags[0].name, "--record");
    EXPECT_EQ(flags[1].name, "--replay");
}

TEST(ReplayCliTest, LoadReplayFileDecodesAPreviouslySavedDocument)
{
    ScratchFile file("antwika_replay_cli_load_test.json");
    {
        std::ofstream out(file.string());
        out << R"({"magic":"antwika-replay","version":1,"events":)"
            << R"([{"tick":0,"event":{"name":"engine.tick","payload":""}}])"
            << "}";
    }

    const auto events = loadReplayFile(file.string());

    EXPECT_EQ(
        events,
        (std::vector<TickEvent>{
            TickEvent{.tick = 0, .event = Event{.name = "engine.tick"}},
        }));
}

TEST(ReplayCliTest, LoadReplayFileThrowsWhenFileCannotBeParsed)
{
    ScratchFile file("antwika_replay_cli_load_malformed_test.json");
    {
        std::ofstream out(file.string());
        out << "not a replay document";
    }

    EXPECT_THROW((void)loadReplayFile(file.string()), ReplayFormatError);
}

TEST(ReplayCliTest, SaveReplayFileFiltersOutBuiltInTicks)
{
    ScratchFile file("antwika_replay_cli_save_test.json");
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

// An absent file and a malformed one are not the same failure.
// They used to produce the same message.
TEST(ReplayCliTest, LoadReplayFileSaysAMissingFileCouldNotBeOpened)
{
    const ScratchFile file("antwika_replay_cli_load_absent_test.json");

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

// The failure this whole check exists for.
// A --record run writes once, at the end, and used to lose it in silence.
TEST(ReplayCliTest, SaveReplayFileSaysAnUnwritablePathCouldNotBeOpened)
{
    const std::string path =
        (std::filesystem::temp_directory_path() / "antwika-no-such-dir"
         / "out.json")
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

// Opening is not writing: a full disk fails only once bytes are flushed.
// ReplayOutputTest is what covers that throw, on any machine.
// This one confirms it against a real device where there is one.
// It may skip freely: skipping it costs no coverage any more.
TEST(ReplayCliTest, SaveReplayFileThrowsWhenTheBytesCannotBeWritten)
{
    if (!std::filesystem::exists("/dev/full"))
    {
        GTEST_SKIP() << "no /dev/full to fill";
    }

    std::vector<TickEvent> events{
        TickEvent{
            .tick = 0,
            .event = Event{.name = "game.score_increment", .payload = "1"},
        },
    };

    try
    {
        saveReplayFile(events, "/dev/full");
        FAIL() << "writing to a full device should have thrown";
    }
    catch (const ReplayFormatError &error)
    {
        const std::string message = error.what();
        EXPECT_NE(message.find("could not write"), std::string::npos)
            << message;
    }
}
