#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/console/ConsoleState.hpp>
#include <antwika/console/SnapshotFormat.hpp>
#include <antwika/console/conformance/ConsoleContractTest.hpp>
#include <antwika/console/conformance/ConsoleSnapshotRoundTripTest.hpp>
#include <antwika/console/testing/ConsoleScript.hpp>
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
using antwika::console::testing::keyAt;
using antwika::console::testing::kOpenTick;
using antwika::console::testing::stopAt;
using antwika::console::testing::typeText;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::event::mocks::MockEventSink;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::log::mocks::MockLogger;
using antwika::replay::ReplaySource;
using antwika::task_worker::TaskRegistry;
using antwika::task_worker::TaskStatus;
using antwika::task_worker::Worker;
using antwika::task_worker::WorkerStatus;
using antwika::time::Tick;
using ::testing::NiceMock;

namespace
{
    constexpr antwika::gfx::Size kCanvas{.width = 960, .height = 600};

    [[nodiscard]] TickEvent submitAt(Tick tick, std::string payload)
    {
        return TickEvent{
            .tick = tick,
            .event = Event{
                .name = antwika::task_worker::events::kTaskSubmit,
                .payload = std::move(payload)}};
    }

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

    struct ConsoleHarness final
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

    struct TaskWorkerConsoleTraits final
    {
        using Summary = antwika::task_worker::TaskWorkerSummary;

        static Summary run(
            std::vector<TickEvent> script,
            const std::string &dumpPath,
            const bool loadEnabled)
        {
            script.push_back(stopAt(kOpenTick + 1));

            ReplaySource source(std::move(script));
            ConsoleHarness harness;

            return harness.run(source, 40, dumpPath, loadEnabled);
        }

        static const std::vector<std::string> &console(
            const Summary &summary)
        {
            return summary.console;
        }

        static void expectUntouched(const Summary &summary)
        {
            EXPECT_EQ(summary.workers, (std::vector<Worker>{Worker{}}));
        }

        static std::string scratchPrefix()
        {
            return "antwika_task_worker_console.";
        }
    };
}

namespace antwika::console::conformance
{

    INSTANTIATE_TYPED_TEST_SUITE_P(
        TaskWorker, ConsoleContractTest, TaskWorkerConsoleTraits);

    INSTANTIATE_TYPED_TEST_SUITE_P(
        TaskWorker, ConsoleSnapshotRoundTripTest, TaskWorkerConsoleTraits);

}

TEST(ConsoleSinkTest, Run_IgnoresKeysWithoutAConsole)
{
    ConsoleHarness harness;
    std::vector<TickEvent> events{keyAt(harness.codec, 1, Key::Grave)};
    typeText(events, harness.codec, kOpenTick, "hello");
    events.push_back(keyAt(harness.codec, kOpenTick, Key::Enter));
    events.push_back(stopAt(kOpenTick + 1));
    ReplaySource source(std::move(events));

    const auto summary = antwika::task_worker::bootstrap(
        antwika::task_worker::TaskWorkerWiring{
            .logger = harness.logger,
            .eventSink = harness.eventSink,
            .inputSource = source,
            .workerCount = 1,
            .maxTicks = 40});

    EXPECT_TRUE(summary.console.empty());
}

TEST(ConsoleSinkTest, DumpState_WritesTheInstantAndSaysSo)
{
    const antwika::testing::ScratchFile file(
        "antwika_task_worker_console_dump.json");
    const auto path = file.path().string();

    ConsoleHarness harness;
    const auto summary = dumpARun(harness, path);

    const auto dumped = readDump(path);

    ASSERT_EQ(dumped.workers.size(), 1U);
    EXPECT_EQ(dumped.workers[0].status, WorkerStatus::Busy);
    EXPECT_EQ(dumped.workers[0].taskId, 1U);
    EXPECT_EQ(dumped.workers[0].label, "Alpha");
    EXPECT_EQ(dumped.workers[0].remainingTicks, 4U);

    ASSERT_EQ(dumped.tasks.size(), 4U);
    EXPECT_EQ(dumped.tasks[0].status, TaskStatus::Running);
    EXPECT_EQ(dumped.tasks[1].status, TaskStatus::Pending);
    EXPECT_EQ(dumped.tasks[2].status, TaskStatus::Pending);
    EXPECT_EQ(dumped.tasks[3].status, TaskStatus::Pending);
    EXPECT_EQ(
        dumped.tasks[3].dependsOn,
        (std::optional<antwika::task_worker::TaskDependency>{
            antwika::task_worker::TaskDependency{2, "Beta"}}));

    ASSERT_EQ(dumped.submissions.size(), 4U);
    EXPECT_EQ(dumped.submissions[0].label, "Alpha");

    EXPECT_EQ(
        summary.console,
        (std::vector<std::string>{
            "> dump_state", "dumped state to " + path}));
}

TEST(ConsoleSinkTest, LoadState_ContinuesAsTheDumpedRunDid)
{
    const antwika::testing::ScratchFile file(
        "antwika_task_worker_console_load.json");
    const auto path = file.path().string();

    ConsoleHarness dumping;
    const auto dumpedSummary = dumpARun(dumping, path);

    ConsoleHarness fresh;
    std::vector<TickEvent> events{keyAt(fresh.codec, 1, Key::Grave)};
    typeText(events, fresh.codec, kOpenTick, "load_state");
    events.push_back(keyAt(fresh.codec, kOpenTick, Key::Enter));
    events.push_back(stopAt(kOpenTick + 3));
    ReplaySource source(std::move(events));

    const auto summary = fresh.run(source, 40, path);

    EXPECT_EQ(summary.workers, dumpedSummary.workers);
    EXPECT_EQ(fresh.registry.allTasks(), dumping.registry.allTasks());
    EXPECT_EQ(
        fresh.registry.lastDispatch(),
        dumping.registry.lastDispatch());
}

TEST(ConsoleSinkTest, LoadState_KeepsPriorityAndDependencyOrder)
{
    const antwika::testing::ScratchFile file(
        "antwika_task_worker_console_order.json");
    const auto path = file.path().string();

    {
        ConsoleHarness dumping;
        static_cast<void>(dumpARun(dumping, path));
    }

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

TEST(ConsoleSinkTest, LoadState_LetsASubmissionDependOnAStarted)
{
    const antwika::testing::ScratchFile file(
        "antwika_task_worker_console_late_dep.json");
    const auto path = file.path().string();

    {
        ConsoleHarness dumping;
        static_cast<void>(dumpARun(dumping, path));
    }

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

TEST(ConsoleSinkTest, LoadState_StillRefusesAResubmittedId)
{
    const antwika::testing::ScratchFile file(
        "antwika_task_worker_console_dup.json");
    const auto path = file.path().string();

    {
        ConsoleHarness dumping;
        static_cast<void>(dumpARun(dumping, path));
    }

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
