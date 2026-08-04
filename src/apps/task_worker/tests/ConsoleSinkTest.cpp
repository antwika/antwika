#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/console/ConsoleState.hpp>
#include <antwika/console/SnapshotFormat.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/testing/ScratchPath.hpp>

#include "antwika/task_worker/Events.hpp"
#include "antwika/task_worker/StateDump.hpp"
#include "antwika/task_worker/TaskRegistry.hpp"
#include "antwika/task_worker/TaskSubmissionError.hpp"
#include "antwika/task_worker/TaskWorker.hpp"
#include "antwika/task_worker/Worker.hpp"

using antwika::console::kConsoleAnimTicks;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::event::mocks::MockEventSink;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::log::mocks::MockLogger;
using antwika::replay::ReplaySource;
using antwika::task_worker::TaskRegistry;
using antwika::task_worker::TaskStatus;
using antwika::task_worker::Worker;
using antwika::task_worker::WorkerStatus;
using antwika::time::Tick;
using ::testing::NiceMock;
using ::testing::StartsWith;

namespace
{
    // The canvas the sheet drops down over, main.cpp's size.
    constexpr antwika::gfx::Size kCanvas{.width = 960, .height = 600};

    // The first tick on which the field reads.
    // The toggle goes down on tick 1 and each tick slides one step.
    constexpr Tick kOpenTick = 1 + kConsoleAnimTicks;

    [[nodiscard]] TickEvent keyAt(
        const InputEventCodec &codec,
        Tick tick,
        Key key,
        bool shift = false)
    {
        return TickEvent{
            .tick = tick,
            .event = codec.encode(KeyPressed{
                .key = key, .modifiers = {.shift = shift}})};
    }

    // The keys that type one command, one press per character.
    // Only what the two commands need: letters, underscore.
    // A run types by the Swedish board unless told otherwise.
    // So the underscore is shift over the American slash position.
    void typeText(
        std::vector<TickEvent> &events,
        const InputEventCodec &codec,
        Tick tick,
        std::string_view text)
    {
        for (const char character : text)
        {
            if (character == '_')
            {
                events.push_back(keyAt(codec, tick, Key::Slash, true));
                continue;
            }

            events.push_back(keyAt(
                codec,
                tick,
                static_cast<Key>(
                    static_cast<std::uint8_t>(Key::A)
                    + (character - 'a'))));
        }
    }

    [[nodiscard]] TickEvent stopAt(Tick tick)
    {
        return TickEvent{
            .tick = tick,
            .event = Event{.name = antwika::engine::events::kStop}};
    }

    [[nodiscard]] TickEvent submitAt(Tick tick, std::string payload)
    {
        return TickEvent{
            .tick = tick,
            .event = Event{
                .name = antwika::task_worker::events::kTaskSubmit,
                .payload = std::move(payload)}};
    }

    // Four tasks: one long one that runs at once, three that wait.
    // Delta outranks everything but depends on Beta.
    // Honouring its edge is visible in the order they run.
    void submitScript(std::vector<TickEvent> &events)
    {
        events.push_back(submitAt(
            1,
            R"({"id":1,"priority":200,"durationTicks":12,)"
            R"("label":"Alpha"})"));
        events.push_back(submitAt(
            1,
            R"({"id":2,"priority":1,"durationTicks":2,)"
            R"("label":"Beta"})"));
        events.push_back(submitAt(
            1,
            R"({"id":3,"priority":2,"durationTicks":2,)"
            R"("label":"Gamma"})"));
        events.push_back(submitAt(
            1,
            R"({"id":4,"priority":3,"durationTicks":2,)"
            R"("label":"Delta","dependsOnId":2})"));
    }

    // BootstrapTest's harness, with the console turned on.
    struct ConsoleHarness
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        InputEventCodec codec;
        TaskRegistry registry;
        antwika::console::ConsolePicture consoleOverlay{kCanvas};

        antwika::task_worker::TaskWorkerSummary run(
            ReplaySource &source,
            Tick maxTicks,
            const std::string &dumpPath,
            bool loadEnabled = true)
        {
            return antwika::task_worker::bootstrap(
                antwika::task_worker::TaskWorkerWiring{
                    .logger = logger,
                    .eventSink = eventSink,
                    .inputSource = source,
                    .workerCount = 1,
                    .registry = registry,
                    .maxTicks = maxTicks,
                    .consoleOverlay = consoleOverlay,
                    .consoleLoadEnabled = loadEnabled,
                    .stateDumpPath = dumpPath});
        }
    };

    // One session runs the script and dumps itself mid-run.
    // It stops three ticks later.
    // Every load test starts from its file.
    [[nodiscard]] antwika::task_worker::TaskWorkerSummary dumpARun(
        ConsoleHarness &harness, const std::string &path)
    {
        std::vector<TickEvent> events;
        submitScript(events);
        events.push_back(keyAt(harness.codec, 2, Key::Grave));
        typeText(
            events, harness.codec, 2 + kConsoleAnimTicks,
            "dump_state");
        events.push_back(
            keyAt(harness.codec, 2 + kConsoleAnimTicks, Key::Enter));
        events.push_back(stopAt(5 + kConsoleAnimTicks));

        ReplaySource source(std::move(events));

        return harness.run(source, 40, path);
    }

    [[nodiscard]] antwika::task_worker::StateDump readDump(
        const std::string &path)
    {
        const antwika::console::SnapshotFormat format(
            {.magic = antwika::task_worker::kStateDumpMagic,
             .version = antwika::task_worker::kStateDumpVersion},
            "antwika task worker state dump document",
            antwika::task_worker::standardStateDumpMigrations);

        return antwika::task_worker::stateDumpFromJson(
            format.read(path).state);
    }

    [[nodiscard]] TaskStatus statusOf(
        const TaskRegistry &registry, std::uint64_t taskId)
    {
        for (const auto &task : registry.allTasks())
        {
            if (task.taskId == taskId)
            {
                return task.status;
            }
        }

        ADD_FAILURE() << "no task with id " << taskId;
        return TaskStatus::Pending;
    }
} // namespace

TEST(ConsoleSinkTest, AnUnknownCommandIsEchoedAndRefused)
{
    ConsoleHarness harness;
    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};
    typeText(events, harness.codec, kOpenTick, "hello");
    events.push_back(keyAt(harness.codec, kOpenTick, Key::Enter));
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    const auto summary = harness.run(source, 40, "unused.json");

    EXPECT_EQ(
        summary.console,
        (std::vector<std::string>{
            "> hello", "unknown command: hello"}));
}

TEST(ConsoleSinkTest, TypingBeforeFullyOpenReachesNoField)
{
    ConsoleHarness harness;
    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};

    // Half way along the slide, none of this may land.
    typeText(events, harness.codec, 3, "hello");
    events.push_back(keyAt(harness.codec, 3, Key::Enter));

    // Fully open, the field is still empty, so Enter says nothing.
    events.push_back(keyAt(harness.codec, kOpenTick, Key::Enter));
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    const auto summary = harness.run(source, 40, "unused.json");

    EXPECT_TRUE(summary.console.empty());
}

TEST(ConsoleSinkTest, ARunWiredWithoutAConsoleIgnoresTheKeys)
{
    ConsoleHarness harness;
    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};
    typeText(events, harness.codec, kOpenTick, "hello");
    events.push_back(keyAt(harness.codec, kOpenTick, Key::Enter));
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    // The same script, wired with no picture to describe into.
    const auto summary = antwika::task_worker::bootstrap(
        antwika::task_worker::TaskWorkerWiring{
            .logger = harness.logger,
            .eventSink = harness.eventSink,
            .inputSource = source,
            .workerCount = 1,
            .maxTicks = 40});

    EXPECT_TRUE(summary.console.empty());
}

TEST(ConsoleSinkTest, DumpStateWritesTheInstantAndSaysSo)
{
    const antwika::testing::ScratchFile file(
        "antwika_task_worker_console_dump.json");
    const auto path = file.path().string();

    ConsoleHarness harness;
    const auto summary = dumpARun(harness, path);

    const auto dumped = readDump(path);

    // Alpha was claimed on tick 1 and counted down every tick since.
    ASSERT_EQ(dumped.workers.size(), 1U);
    EXPECT_EQ(dumped.workers[0].status, WorkerStatus::Busy);
    EXPECT_EQ(dumped.workers[0].taskId, 1U);
    EXPECT_EQ(dumped.workers[0].label, "Alpha");
    EXPECT_EQ(dumped.workers[0].remainingTicks, 4U);

    // The rest of the script still waits, its edge included.
    ASSERT_EQ(dumped.tasks.size(), 4U);
    EXPECT_EQ(dumped.tasks[0].status, TaskStatus::Running);
    EXPECT_EQ(dumped.tasks[1].status, TaskStatus::Pending);
    EXPECT_EQ(dumped.tasks[2].status, TaskStatus::Pending);
    EXPECT_EQ(dumped.tasks[3].status, TaskStatus::Pending);
    EXPECT_EQ(
        dumped.tasks[3].dependsOn,
        (std::optional<antwika::task_worker::TaskDependency>{
            antwika::task_worker::TaskDependency{2, "Beta"}}));

    // The submissions ride along, without any JobId.
    ASSERT_EQ(dumped.submissions.size(), 4U);
    EXPECT_EQ(dumped.submissions[0].label, "Alpha");

    EXPECT_EQ(
        summary.console,
        (std::vector<std::string>{
            "> dump_state", "dumped state to " + path}));
}

TEST(ConsoleSinkTest, LoadStateContinuesExactlyAsTheDumpedRunDid)
{
    const antwika::testing::ScratchFile file(
        "antwika_task_worker_console_load.json");
    const auto path = file.path().string();

    // The dumping session goes on for three ticks after the dump.
    ConsoleHarness dumping;
    const auto dumpedSummary = dumpARun(dumping, path);

    // A fresh session loads it and runs three ticks after the load.
    ConsoleHarness fresh;
    std::vector<TickEvent> events{keyAt(fresh.codec, 1, Key::Grave)};
    typeText(events, fresh.codec, kOpenTick, "load_state");
    events.push_back(keyAt(fresh.codec, kOpenTick, Key::Enter));
    events.push_back(stopAt(kOpenTick + 3));
    ReplaySource source(std::move(events));

    const auto summary = fresh.run(source, 40, path);

    // Same ticks after the same instant, so the same pool.
    EXPECT_EQ(summary.workers, dumpedSummary.workers);
    EXPECT_EQ(fresh.registry.allTasks(), dumping.registry.allTasks());
    EXPECT_EQ(
        fresh.registry.lastDispatch(),
        dumping.registry.lastDispatch());

    // The dump's own exchange, then what loading it said.
    auto expected = dumpedSummary.console;
    expected.push_back("loaded state from " + path);
    EXPECT_EQ(summary.console, expected);
}

TEST(ConsoleSinkTest, RenumberedTasksKeepPriorityAndDependencyOrder)
{
    const antwika::testing::ScratchFile file(
        "antwika_task_worker_console_order.json");
    const auto path = file.path().string();

    {
        ConsoleHarness dumping;
        static_cast<void>(dumpARun(dumping, path));
    }

    // Loaded, Alpha still holds the worker for four more ticks.
    // Then Gamma must outrank Beta.
    // And Delta, the highest of the three, must still wait for Beta.
    // Its edge rode the renumbered JobIds, or it would go first.
    ConsoleHarness fresh;
    std::vector<TickEvent> events{keyAt(fresh.codec, 1, Key::Grave)};
    typeText(events, fresh.codec, kOpenTick, "load_state");
    events.push_back(keyAt(fresh.codec, kOpenTick, Key::Enter));
    events.push_back(stopAt(kOpenTick + 6));
    ReplaySource source(std::move(events));

    const auto summary = fresh.run(source, 40, path);

    EXPECT_EQ(statusOf(fresh.registry, 1), TaskStatus::Completed);
    EXPECT_EQ(statusOf(fresh.registry, 3), TaskStatus::Completed);
    EXPECT_EQ(statusOf(fresh.registry, 2), TaskStatus::Running);
    EXPECT_EQ(statusOf(fresh.registry, 4), TaskStatus::Pending);
    ASSERT_EQ(summary.workers.size(), 1U);
    EXPECT_EQ(summary.workers[0].taskId, 2U);
}

TEST(ConsoleSinkTest, ASubmissionAfterALoadMayDependOnAStartedTask)
{
    const antwika::testing::ScratchFile file(
        "antwika_task_worker_console_late_dep.json");
    const auto path = file.path().string();

    {
        ConsoleHarness dumping;
        static_cast<void>(dumpARun(dumping, path));
    }

    // Echo depends on Alpha, which the dump holds as Running.
    // Its edge is already satisfied.
    // So Echo's priority wins the worker the moment Alpha lets go.
    ConsoleHarness fresh;
    std::vector<TickEvent> events{keyAt(fresh.codec, 1, Key::Grave)};
    typeText(events, fresh.codec, kOpenTick, "load_state");
    events.push_back(keyAt(fresh.codec, kOpenTick, Key::Enter));
    events.push_back(submitAt(
        kOpenTick + 1,
        R"({"id":5,"priority":250,"durationTicks":2,)"
        R"("label":"Echo","dependsOnId":1})"));
    events.push_back(stopAt(kOpenTick + 6));
    ReplaySource source(std::move(events));

    static_cast<void>(fresh.run(source, 40, path));

    EXPECT_EQ(statusOf(fresh.registry, 1), TaskStatus::Completed);
    EXPECT_EQ(statusOf(fresh.registry, 5), TaskStatus::Completed);
    EXPECT_EQ(statusOf(fresh.registry, 3), TaskStatus::Running);
}

TEST(ConsoleSinkTest, AResubmittedIdIsStillRefusedAfterALoad)
{
    const antwika::testing::ScratchFile file(
        "antwika_task_worker_console_dup.json");
    const auto path = file.path().string();

    {
        ConsoleHarness dumping;
        static_cast<void>(dumpARun(dumping, path));
    }

    // The loaded submissions carry every dumped id.
    ConsoleHarness fresh;
    std::vector<TickEvent> events{keyAt(fresh.codec, 1, Key::Grave)};
    typeText(events, fresh.codec, kOpenTick, "load_state");
    events.push_back(keyAt(fresh.codec, kOpenTick, Key::Enter));
    events.push_back(submitAt(
        kOpenTick + 1,
        R"({"id":2,"priority":1,"durationTicks":2,)"
        R"("label":"BetaAgain"})"));
    events.push_back(stopAt(kOpenTick + 2));
    ReplaySource source(std::move(events));

    EXPECT_THROW(
        static_cast<void>(fresh.run(source, 40, path)),
        antwika::task_worker::TaskSubmissionError);
}

TEST(ConsoleSinkTest, LoadStateIsRefusedWhileRecordingOrReplaying)
{
    ConsoleHarness harness;
    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};
    typeText(events, harness.codec, kOpenTick, "load_state");
    events.push_back(keyAt(harness.codec, kOpenTick, Key::Enter));
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    const auto summary =
        harness.run(source, 40, "unused.json", false);

    EXPECT_EQ(
        summary.console,
        (std::vector<std::string>{
            "> load_state",
            "load_state: not available while recording or replaying"}));
    EXPECT_EQ(summary.workers, (std::vector<Worker>{Worker{}}));
}

TEST(ConsoleSinkTest, LoadStateAnswersAFileThatIsNotThere)
{
    const antwika::testing::ScratchFile file(
        "antwika_task_worker_console_absent.json");

    ConsoleHarness harness;
    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};
    typeText(events, harness.codec, kOpenTick, "load_state");
    events.push_back(keyAt(harness.codec, kOpenTick, Key::Enter));
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    const auto summary =
        harness.run(source, 40, file.path().string());

    ASSERT_EQ(summary.console.size(), 2U);
    EXPECT_EQ(summary.console[0], "> load_state");
    EXPECT_THAT(summary.console[1], StartsWith("could not load: "));
}
