#include <gtest/gtest.h>

#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "antwika/replay/ReplayCli.hpp"
#include "antwika/replay/ReplayFormatError.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
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

TEST(ReplayCliTest, ParseIgnoresATrailingReplayFlagMissingItsValue)
{
    std::vector<std::string> args{"antwika_app", "--replay"};
    auto argv = toArgv(args);

    const auto options = parseReplayCliOptions(
        static_cast<int>(argv.size()), argv.data());

    EXPECT_FALSE(options.replayPath.has_value());
}

TEST(ReplayCliTest, ParseIgnoresATrailingRecordFlagMissingItsValue)
{
    std::vector<std::string> args{"antwika_app", "--record"};
    auto argv = toArgv(args);

    const auto options = parseReplayCliOptions(
        static_cast<int>(argv.size()), argv.data());

    EXPECT_FALSE(options.recordPath.has_value());
}

TEST(ReplayCliTest, ParseIgnoresAnUnrecognizedFlag)
{
    std::vector<std::string> args{"antwika_app", "--unknown", "value"};
    auto argv = toArgv(args);

    const auto options = parseReplayCliOptions(
        static_cast<int>(argv.size()), argv.data());

    EXPECT_FALSE(options.recordPath.has_value());
    EXPECT_FALSE(options.replayPath.has_value());
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
    ScratchFile file("antwika_replay_cli_load_missing_test.json");

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

TEST(ReplayCliTest, SaveReplayFileFiltersOutExtraSelfGeneratedEventNames)
{
    ScratchFile file("antwika_replay_cli_save_extra_test.json");
    std::vector<TickEvent> events{
        TickEvent{.tick = 0, .event = Event{.name = "Running Antwika App"}},
        TickEvent{
            .tick = 0,
            .event = Event{.name = "game.score_increment", .payload = "1"},
        },
    };
    constexpr std::array<std::string_view, 1> extraNames{
        "Running Antwika App",
    };

    saveReplayFile(events, file.string(), extraNames);

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
