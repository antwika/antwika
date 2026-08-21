#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>
#include <unistd.h>

#include <antwika/app/RunRecorded.hpp>
#include <antwika/cli/FlagSpec.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/replay/ReplayCli.hpp>
#include <antwika/replay/ReplayFormatError.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/testing/ScratchFile.hpp>

using antwika::app::RunContext;
using antwika::app::runRecorded;
using antwika::app::loadReplayEvents;
using antwika::event::TickEvent;
using antwika::replay::loadReplayFile;
using antwika::replay::ReplayFormatError;
using antwika::replay::saveReplayFile;

namespace
{
    const TickEvent kScripted{
        .tick = antwika::time::Tick{1},
        .event = {.name = "test.event", .payload = "payload"}};
}

TEST(RunRecordedTest, RunRecorded_RunsTheBodyAndSucceeds)
{
    std::array<char *, 1> argv{const_cast<char *>("antwika_test")};
    std::ostringstream errors;
    bool ran = false;

    const int exitCode = runRecorded(
        1,
        argv.data(),
        "antwika_test",
        [&ran](const RunContext &) { ran = true; },
        {},
        errors);

    EXPECT_TRUE(ran);
    EXPECT_EQ(exitCode, EXIT_SUCCESS);
    EXPECT_TRUE(errors.str().empty());
}

TEST(RunRecordedTest, RunRecorded_ReportsAFailureUnderTheName)
{
    std::array<char *, 1> argv{const_cast<char *>("antwika_test")};
    std::ostringstream errors;

    const int exitCode = runRecorded(
        1,
        argv.data(),
        "antwika_test",
        [](const RunContext &)
        { throw std::runtime_error("it went wrong"); },
        {},
        errors);

    EXPECT_EQ(exitCode, EXIT_FAILURE);
    EXPECT_EQ(errors.str(), "antwika_test: it went wrong\n");
}

TEST(RunRecordedTest, RunRecorded_AttachesNoRecorderWithoutRecord)
{
    std::array<char *, 1> argv{const_cast<char *>("antwika_test")};
    std::ostringstream errors;

    static_cast<void>(runRecorded(
        1,
        argv.data(),
        "antwika_test",
        [](const RunContext &run)
        { EXPECT_FALSE(run.replayRecorderSink.has_value()); },
        {},
        errors));
}

TEST(RunRecordedTest, RunRecorded_SavesWhatTheRecorderWasGiven)
{
    const antwika::testing::ScratchFile file("antwika-app-recorded.json");
    const std::string path = file.string();
    std::array<char *, 3> argv{
        const_cast<char *>("antwika_test"),
        const_cast<char *>("--record"),
        const_cast<char *>(path.c_str())};
    std::ostringstream errors;

    const int exitCode = runRecorded(
        3,
        argv.data(),
        "antwika_test",
        [](const RunContext &run)
        {
            ASSERT_TRUE(run.replayRecorderSink.has_value());
            run.replayRecorderSink->get().handle(kScripted);
        },
        {},
        errors);

    EXPECT_EQ(exitCode, EXIT_SUCCESS);
    EXPECT_EQ(loadReplayFile(file.string()), std::vector{kScripted});
}

TEST(RunRecordedTest, RunRecorded_SavesWhatAFailedRunGotTo)
{
    const antwika::testing::ScratchFile file("antwika-app-failed.json");
    const std::string path = file.string();
    std::array<char *, 3> argv{
        const_cast<char *>("antwika_test"),
        const_cast<char *>("--record"),
        const_cast<char *>(path.c_str())};
    std::ostringstream errors;

    const int exitCode = runRecorded(
        3,
        argv.data(),
        "antwika_test",
        [](const RunContext &run)
        {
            run.replayRecorderSink->get().handle(kScripted);
            throw std::runtime_error("it went wrong");
        },
        {},
        errors);

    EXPECT_EQ(exitCode, EXIT_FAILURE);
    EXPECT_EQ(loadReplayFile(file.string()), std::vector{kScripted});
}

TEST(RunRecordedTest, RunRecorded_RefusesAPathBeforeTheSession)
{
    const std::string path =
        (std::filesystem::temp_directory_path() / "antwika-no-such-dir"
         / "out.jsonl")
            .string();
    std::array<char *, 3> argv{
        const_cast<char *>("antwika_test"),
        const_cast<char *>("--record"),
        const_cast<char *>(path.c_str())};
    std::ostringstream errors;

    bool ran = false;
    const int exitCode = runRecorded(
        3,
        argv.data(),
        "antwika_test",
        [&ran](const RunContext &) { ran = true; },
        {},
        errors);

    EXPECT_FALSE(ran);
    EXPECT_EQ(exitCode, EXIT_FAILURE);
    EXPECT_THAT(errors.str(), testing::HasSubstr("antwika_test: "));
    EXPECT_THAT(errors.str(), testing::HasSubstr("could not open"));
    EXPECT_THAT(errors.str(), testing::HasSubstr(path));
}

TEST(RunRecordedTest, RunRecorded_KeepsWhatAKilledRunGotTo)
{
    const antwika::testing::ScratchFile file("antwika-app-killed.jsonl");
    const std::string path = file.string();
    std::array<char *, 3> argv{
        const_cast<char *>("antwika_test"),
        const_cast<char *>("--record"),
        const_cast<char *>(path.c_str())};
    std::ostringstream errors;

    std::vector<TickEvent> midRunEvents;
    static_cast<void>(runRecorded(
        3,
        argv.data(),
        "antwika_test",
        [&midRunEvents, &path](const RunContext &run)
        {
            run.replayRecorderSink->get().handle(kScripted);
            run.replayRecorderSink->get().handle(
                TickEvent{
                    .tick = antwika::time::Tick{2},
                    .event = {.name = "test.event", .payload = "more"}});

            midRunEvents = loadReplayFile(path);
        },
        {},
        errors));

    ASSERT_EQ(midRunEvents.size(), 2U);
    EXPECT_EQ(midRunEvents.front(), kScripted);
}

TEST(RunRecordedTest, RunRecorded_HandsTheBodyThePathToReplay)
{
    std::array<char *, 3> argv{
        const_cast<char *>("antwika_test"),
        const_cast<char *>("--replay"),
        const_cast<char *>("demo.json")};
    std::ostringstream errors;

    static_cast<void>(runRecorded(
        3,
        argv.data(),
        "antwika_test",
        [](const RunContext &run)
        { EXPECT_EQ(run.options.replayPath, "demo.json"); },
        {},
        errors));
}

TEST(ScriptedEventsTest, ScriptedEvents_StartEmptyWithNoPath)
{
    EXPECT_TRUE(loadReplayEvents(std::nullopt).empty());
}

TEST(ScriptedEventsTest, ScriptedEvents_LoadTheFallbackWithNoPath)
{
    const antwika::testing::ScratchFile file("antwika-app-fallback.json");
    saveReplayFile({kScripted}, file.string());

    EXPECT_EQ(
        loadReplayEvents(std::nullopt, file.string()),
        std::vector{kScripted});
}

TEST(ScriptedEventsTest, ScriptedEvents_PreferThePathToTheFallback)
{
    const antwika::testing::ScratchFile file("antwika-app-named.json");
    saveReplayFile({kScripted}, file.string());

    EXPECT_EQ(
        loadReplayEvents(file.string(), "no-such-fallback.json"),
        std::vector{kScripted});
}

TEST(ScriptedEventsTest, ScriptedEvents_ThrowWhenTheFileIsNotThere)
{
    EXPECT_THROW(
        static_cast<void>(loadReplayEvents("no-such-replay.json")),
        ReplayFormatError);
}

TEST(RunRecordedTest, RunRecorded_DiscardsWhatReachesTheSink)
{
    std::array<char *, 1> argv{const_cast<char *>("antwika_test")};
    std::ostringstream errors;

    static_cast<void>(runRecorded(
        1,
        argv.data(),
        "antwika_test",
        [](const RunContext &run)
        {
            run.eventSink.handle(kScripted.event);
            run.eventSink.handle({});
        },
        {},
        errors));

    EXPECT_TRUE(errors.str().empty());
}

TEST(RunRecordedTest, RunRecorded_LetsANonExceptionThrough)
{
    std::array<char *, 1> argv{const_cast<char *>("antwika_test")};
    std::ostringstream errors;

    EXPECT_THROW(
        static_cast<void>(runRecorded(
            1,
            argv.data(),
            "antwika_test",
            [](const RunContext &) { throw 42; },
            {},
            errors)),
        int);

    EXPECT_TRUE(errors.str().empty());
}

TEST(RunRecordedTest, RunRecorded_AcceptsACallersOwnFlag)
{
    std::array<char *, 3> argv{
        const_cast<char *>("antwika_test"),
        const_cast<char *>("--tick-delay-ms"),
        const_cast<char *>("40")};
    std::ostringstream errors;
    constexpr std::array extra{antwika::cli::FlagSpec{
        .name = "--tick-delay-ms",
        .valueName = "<n>",
        .help = "Hold each frame."}};

    std::string output;
    const int exitCode = runRecorded(
        3,
        argv.data(),
        "antwika_test",
        [&output](const RunContext &run)
        { output = run.commandLine.value("--tick-delay-ms").value_or(""); },
        extra,
        errors);

    EXPECT_EQ(exitCode, EXIT_SUCCESS);
    EXPECT_EQ(output, "40");
    EXPECT_TRUE(errors.str().empty());
}

TEST(RunRecordedTest, RunRecorded_AnswersHelpWithoutTheBody)
{
    std::array<char *, 2> argv{
        const_cast<char *>("antwika_test"),
        const_cast<char *>("--help")};
    std::ostringstream errors;
    std::ostringstream help;
    constexpr std::array extra{antwika::cli::FlagSpec{
        .name = "--tick-delay-ms",
        .valueName = "<n>",
        .help = "Hold each frame."}};

    bool ran = false;
    const int exitCode = runRecorded(
        2,
        argv.data(),
        "antwika_test",
        [&ran](const RunContext &) { ran = true; },
        extra,
        errors,
        help);

    EXPECT_FALSE(ran);
    EXPECT_EQ(exitCode, EXIT_SUCCESS);
    EXPECT_TRUE(errors.str().empty());
    EXPECT_FALSE(help.str().empty());
}

TEST(RunRecordedTest, RunRecorded_ListsTheReplayFlagsAndTheCallersOwn)
{
    std::array<char *, 2> argv{
        const_cast<char *>("antwika_test"),
        const_cast<char *>("--help")};
    std::ostringstream errors;
    std::ostringstream help;
    constexpr std::array extra{antwika::cli::FlagSpec{
        .name = "--tick-delay-ms",
        .valueName = "<n>",
        .help = "Hold each frame."}};

    static_cast<void>(runRecorded(
        2,
        argv.data(),
        "antwika_test",
        [](const RunContext &) {},
        extra,
        errors,
        help));

    EXPECT_THAT(help.str(), testing::HasSubstr("Usage: antwika_test"));
    EXPECT_THAT(help.str(), testing::HasSubstr("--record <path>"));
    EXPECT_THAT(help.str(), testing::HasSubstr("--replay <path>"));
    EXPECT_THAT(help.str(), testing::HasSubstr("--tick-delay-ms <n>"));
    EXPECT_THAT(help.str(), testing::HasSubstr("Hold each frame."));
    EXPECT_THAT(help.str(), testing::HasSubstr("--help"));
}

TEST(RunRecordedTest, RunRecorded_WritesNoRecordingForHelp)
{
    const antwika::testing::ScratchFile file("antwika-app-help.json");
    const std::string path = file.string();
    std::array<char *, 4> argv{
        const_cast<char *>("antwika_test"),
        const_cast<char *>("--record"),
        const_cast<char *>(path.c_str()),
        const_cast<char *>("--help")};
    std::ostringstream errors;
    std::ostringstream help;

    const int exitCode = runRecorded(
        4,
        argv.data(),
        "antwika_test",
        [](const RunContext &) { FAIL() << "the body should not run"; },
        {},
        errors,
        help);

    EXPECT_EQ(exitCode, EXIT_SUCCESS);
    EXPECT_FALSE(std::filesystem::exists(path));
}

TEST(RunRecordedTest, RunRecorded_ReportsAnUnknownFlag)
{
    std::array<char *, 2> argv{
        const_cast<char *>("antwika_test"),
        const_cast<char *>("--tick-delay-ms")};
    std::ostringstream errors;

    bool ran = false;
    const int exitCode = runRecorded(
        2,
        argv.data(),
        "antwika_test",
        [&ran](const RunContext &) { ran = true; },
        {},
        errors);

    EXPECT_EQ(exitCode, EXIT_FAILURE);
    EXPECT_FALSE(ran);
    EXPECT_THAT(
        errors.str(), testing::HasSubstr("antwika_test: "));
    EXPECT_THAT(errors.str(), testing::HasSubstr("--tick-delay-ms"));
}
