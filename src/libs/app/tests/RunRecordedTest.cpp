#include <array>
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

#include <antwika/app/RunRecorded.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/replay/ReplayCli.hpp>
#include <antwika/replay/ReplayFormatError.hpp>
#include <antwika/time/Tick.hpp>

using antwika::app::RecordedRun;
using antwika::app::runRecorded;
using antwika::app::scriptedEvents;
using antwika::event::TickEvent;
using antwika::replay::loadReplayFile;
using antwika::replay::ReplayFormatError;
using antwika::replay::saveReplayFile;

namespace
{
    /**
     * @brief A file in the temporary directory, removed when it goes.
     */
    class TempFile final
    {
    public:
        explicit TempFile(const std::string &name)
            : path(std::filesystem::temp_directory_path() / name)
        {
        }

        TempFile(const TempFile &) = delete;
        TempFile(TempFile &&) = delete;

        TempFile &operator=(const TempFile &) = delete;
        TempFile &operator=(TempFile &&) = delete;

        ~TempFile()
        {
            std::error_code ignored;
            std::filesystem::remove(path, ignored);
        }

        [[nodiscard]] std::string name() const
        {
            return path.string();
        }

    private:
        std::filesystem::path path;
    };

    const TickEvent kScripted{
        .tick = antwika::time::Tick{1},
        .event = {.name = "test.event", .payload = "payload"}};
} // namespace

TEST(RunRecordedTest, RunsTheBodyAndSucceeds)
{
    std::array<char *, 1> argv{const_cast<char *>("antwika_test")};
    std::ostringstream errors;
    bool ran = false;

    const int exitCode = runRecorded(
        1,
        argv.data(),
        "antwika_test",
        [&ran](const RecordedRun &) { ran = true; },
        errors);

    EXPECT_TRUE(ran);
    EXPECT_EQ(exitCode, EXIT_SUCCESS);
    EXPECT_TRUE(errors.str().empty());
}

TEST(RunRecordedTest, ReportsAFailureUnderTheProgramsName)
{
    std::array<char *, 1> argv{const_cast<char *>("antwika_test")};
    std::ostringstream errors;

    const int exitCode = runRecorded(
        1,
        argv.data(),
        "antwika_test",
        [](const RecordedRun &)
        { throw std::runtime_error("it went wrong"); },
        errors);

    EXPECT_EQ(exitCode, EXIT_FAILURE);
    EXPECT_EQ(errors.str(), "antwika_test: it went wrong\n");
}

TEST(RunRecordedTest, AttachesNoRecorderWithoutRecord)
{
    std::array<char *, 1> argv{const_cast<char *>("antwika_test")};
    std::ostringstream errors;

    static_cast<void>(runRecorded(
        1,
        argv.data(),
        "antwika_test",
        [](const RecordedRun &run)
        { EXPECT_FALSE(run.replayRecorder.has_value()); },
        errors));
}

TEST(RunRecordedTest, SavesWhatTheRecorderWasGiven)
{
    const TempFile file("antwika-app-recorded.json");
    const std::string path = file.name();
    std::array<char *, 3> argv{
        const_cast<char *>("antwika_test"),
        const_cast<char *>("--record"),
        const_cast<char *>(path.c_str())};
    std::ostringstream errors;

    const int exitCode = runRecorded(
        3,
        argv.data(),
        "antwika_test",
        [](const RecordedRun &run)
        {
            ASSERT_TRUE(run.replayRecorder.has_value());
            run.replayRecorder->get().handle(kScripted);
        },
        errors);

    EXPECT_EQ(exitCode, EXIT_SUCCESS);
    EXPECT_EQ(loadReplayFile(file.name()), std::vector{kScripted});
}

TEST(RunRecordedTest, SavesWhatAFailedRunGotTo)
{
    const TempFile file("antwika-app-failed.json");
    const std::string path = file.name();
    std::array<char *, 3> argv{
        const_cast<char *>("antwika_test"),
        const_cast<char *>("--record"),
        const_cast<char *>(path.c_str())};
    std::ostringstream errors;

    const int exitCode = runRecorded(
        3,
        argv.data(),
        "antwika_test",
        [](const RecordedRun &run)
        {
            run.replayRecorder->get().handle(kScripted);
            throw std::runtime_error("it went wrong");
        },
        errors);

    EXPECT_EQ(exitCode, EXIT_FAILURE);
    EXPECT_EQ(loadReplayFile(file.name()), std::vector{kScripted});
}

TEST(RunRecordedTest, HandsTheBodyThePathToReplay)
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
        [](const RecordedRun &run)
        { EXPECT_EQ(run.options.replayPath, "demo.json"); },
        errors));
}

TEST(ScriptedEventsTest, StartsEmptyWithNoPathAndNoFallback)
{
    EXPECT_TRUE(scriptedEvents(std::nullopt).empty());
}

TEST(ScriptedEventsTest, LoadsTheFallbackWhenNoPathWasGiven)
{
    const TempFile file("antwika-app-fallback.json");
    saveReplayFile({kScripted}, file.name());

    EXPECT_EQ(
        scriptedEvents(std::nullopt, file.name()),
        std::vector{kScripted});
}

TEST(ScriptedEventsTest, LoadsThePathInPreferenceToTheFallback)
{
    const TempFile file("antwika-app-named.json");
    saveReplayFile({kScripted}, file.name());

    EXPECT_EQ(
        scriptedEvents(file.name(), "no-such-fallback.json"),
        std::vector{kScripted});
}

TEST(ScriptedEventsTest, ThrowsWhenTheNamedFileIsNotThere)
{
    EXPECT_THROW(
        static_cast<void>(scriptedEvents("no-such-replay.json")),
        ReplayFormatError);
}
