#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/mocks/MockSystem.hpp>
#include <antwika/ecs_commons/Name.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/simulation/EngineLoopError.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include <antwika/scheduler/Priority.hpp>

#include "DemoScript.hpp"
#include "antwika/task_worker/Events.hpp"
#include "antwika/task_worker/PoolSnapshot.hpp"
#include "antwika/task_worker/TaskRegistry.hpp"
#include "antwika/task_worker/TaskWorker.hpp"
#include "antwika/task_worker/Worker.hpp"

using antwika::ecs::ISystem;
using antwika::ecs::World;
using antwika::ecs::mocks::MockSystem;
using antwika::ecs_commons::makeName;
using antwika::event::Event;
using antwika::event::mocks::MockEventSink;
using antwika::event::TickEventRecorder;
using antwika::event::TickEvent;
using antwika::log::Level;
using antwika::simulation::EngineLoopError;
using antwika::replay::ReplaySource;
using antwika::scheduler::kCriticalPriority;
using antwika::scheduler::kLowPriority;
using antwika::scheduler::kNormalPriority;
using antwika::task_worker::TaskDependency;
using antwika::task_worker::TaskInfo;
using antwika::task_worker::TaskRegistry;
using antwika::task_worker::TaskStatus;
using antwika::task_worker::Worker;
using antwika::task_worker::WorkerStatus;
using antwika::log::mocks::MockLogger;
using antwika::task_worker::events::kTaskSubmit;
using antwika::task_worker::tests::demoScript;
using antwika::task_worker::tests::kMaxTicks;
using antwika::task_worker::tests::kWorkerCount;
using ::testing::_;
using ::testing::NiceMock;

namespace
{
    constexpr int kExpectedTicks = 6;

    class FakeSnapshotRecordingSystem final : public ISystem
    {
    public:
        explicit FakeSnapshotRecordingSystem(const TaskRegistry &registry)
            : registry(registry)
        {
        }

        FakeSnapshotRecordingSystem(
            const FakeSnapshotRecordingSystem &) = delete;
        FakeSnapshotRecordingSystem(FakeSnapshotRecordingSystem &&) = delete;

        FakeSnapshotRecordingSystem &operator=(
            const FakeSnapshotRecordingSystem &) = delete;
        FakeSnapshotRecordingSystem &operator=(
            FakeSnapshotRecordingSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override
        {
            snapshots.push_back(
                antwika::task_worker::snapshotOf(world, registry, tick));
        }

        std::vector<antwika::task_worker::PoolSnapshot> snapshots;

    private:
        const TaskRegistry &registry;
    };

    [[nodiscard]] std::vector<std::string> queueLabelsOf(
        const antwika::task_worker::PoolSnapshot &snapshot)
    {
        std::vector<std::string> labels;

        for (const auto &task : snapshot.queue)
        {
            labels.push_back(
                task.blocked ? task.label + " waits for "
                                   + task.waitingFor
                             : task.label);
        }

        return labels;
    }
}

TEST(BootstrapTest, Bootstrap_RunsScriptedTasksToCompletion)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;

    auto script = demoScript();
    ReplaySource inputSource(script);

    auto finalState = antwika::task_worker::bootstrap(
        antwika::task_worker::TaskWorkerWiring{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = inputSource,
            .workerCount = kWorkerCount,
            .maxTicks = kMaxTicks}).workers;

    ASSERT_EQ(finalState.size(), 2U);
    EXPECT_EQ(
        finalState[0],
        (Worker{WorkerStatus::Busy, 1, 5, makeName("Epsilon")}));
    EXPECT_EQ(
        finalState[1],
        (Worker{WorkerStatus::Busy, 2, 3, makeName("Gamma")}));
}

TEST(BootstrapTest, Bootstrap_RunsEveryObserverOncePerTick)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;

    auto script = demoScript();
    ReplaySource inputSource(script);
    NiceMock<MockSystem> countingSystem;
    EXPECT_CALL(countingSystem, update(_, _)).Times(kExpectedTicks);

    antwika::task_worker::bootstrap(
        antwika::task_worker::TaskWorkerWiring{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = inputSource,
            .workerCount = kWorkerCount,
            .observers = {countingSystem},
            .maxTicks = kMaxTicks});
}

TEST(BootstrapTest, Bootstrap_KeepsACallerSuppliedRegistryInSync)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;

    auto script = demoScript();
    ReplaySource inputSource(script);
    TaskRegistry registry;

    antwika::task_worker::bootstrap(
        antwika::task_worker::TaskWorkerWiring{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = inputSource,
            .workerCount = kWorkerCount,
            .registry = registry,
            .maxTicks = kMaxTicks});

    EXPECT_EQ(
        registry.allTasks(),
        (std::vector<TaskInfo>{
            TaskInfo{
                1, "Alpha", kNormalPriority, TaskStatus::Completed, 4, 0,
                std::nullopt},
            TaskInfo{
                2, "Beta", kNormalPriority, TaskStatus::Completed, 5, 0,
                std::nullopt},
            TaskInfo{
                3, "Gamma", kLowPriority, TaskStatus::Running, 2, 2,
                std::nullopt},
            TaskInfo{
                4, "Delta", kCriticalPriority, TaskStatus::Completed, 1, 0,
                std::nullopt},
            TaskInfo{
                5, "Epsilon", kNormalPriority, TaskStatus::Running, 1, 1,
                TaskDependency{4, "Delta"}}}));
}

TEST(BootstrapTest, Bootstrap_DrawsTheQueueTheSchedulerWillActuallyPull)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;

    auto script = demoScript();
    ReplaySource inputSource(script);
    TaskRegistry registry;
    FakeSnapshotRecordingSystem frames(registry);

    antwika::task_worker::bootstrap(
        antwika::task_worker::TaskWorkerWiring{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = inputSource,
            .workerCount = kWorkerCount,
            .observers = {frames},
            .registry = registry,
            .maxTicks = kMaxTicks});

    ASSERT_EQ(frames.snapshots.size(), static_cast<std::size_t>(6));

    EXPECT_EQ(
        queueLabelsOf(frames.snapshots[0]),
        (std::vector<std::string>{"Gamma"}));
    EXPECT_EQ(
        queueLabelsOf(frames.snapshots[3]),
        (std::vector<std::string>{"Gamma"}));
    EXPECT_EQ(
        queueLabelsOf(frames.snapshots[4]),
        (std::vector<std::string>{"Gamma", "Epsilon waits for Delta"}));
    EXPECT_TRUE(frames.snapshots[5].queue.empty());

    EXPECT_EQ(
        frames.snapshots[0].dispatch,
        (antwika::task_worker::DispatchInfo{2, 2}));
    EXPECT_EQ(
        frames.snapshots[3].dispatch,
        (antwika::task_worker::DispatchInfo{0, 0}));
    EXPECT_EQ(
        frames.snapshots[4].dispatch,
        (antwika::task_worker::DispatchInfo{1, 1}));
    EXPECT_EQ(
        frames.snapshots[5].dispatch,
        (antwika::task_worker::DispatchInfo{2, 2}));

    EXPECT_EQ(
        frames.snapshots[5].workers[0].label, std::string{"Epsilon"});
    EXPECT_EQ(frames.snapshots[5].workers[1].label, std::string{"Gamma"});
}

TEST(BootstrapTest, Bootstrap_WithNoScriptedInputAllWorkersStayIdle)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;

    ReplaySource inputSource({
        TickEvent{
            .tick = 2,
            .event = Event{.name = antwika::engine::events::kStop}},
    });

    auto finalState = antwika::task_worker::bootstrap(
        antwika::task_worker::TaskWorkerWiring{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = inputSource,
            .workerCount = 2,
            .maxTicks = kMaxTicks}).workers;

    EXPECT_EQ(finalState[0], (Worker{WorkerStatus::Idle, 0}));
    EXPECT_EQ(finalState[1], (Worker{WorkerStatus::Idle, 0}));
}

TEST(BootstrapTest, Bootstrap_ForwardsDispatchedEventsToATickEventRecorder)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;

    ReplaySource inputSource({
        TickEvent{
            .tick = 0,
            .event = Event{
                .name = kTaskSubmit,
                .payload = R"({"id":1,"priority":1,)"
                           R"("durationTicks":4,"label":"Alpha"})"}},
        TickEvent{
            .tick = 0,
            .event = Event{.name = antwika::engine::events::kStop}},
    });
    TickEventRecorder replayRecorder;

    antwika::task_worker::bootstrap(
        antwika::task_worker::TaskWorkerWiring{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = inputSource,
            .workerCount = kWorkerCount,
            .maxTicks = kMaxTicks,
            .replayRecorder = replayRecorder});

    EXPECT_EQ(
        replayRecorder.getEvents(),
        (std::vector<TickEvent>{
            TickEvent{
                .tick = 0,
                .event = Event{
                    .name = kTaskSubmit,
                    .payload = R"({"id":1,"priority":1,)"
                               R"("durationTicks":4,"label":"Alpha"})"}},
            TickEvent{
                .tick = 0,
                .event = Event{.name = antwika::engine::events::kStop}},
            TickEvent{
                .tick = 0,
                .event = Event{.name = antwika::engine::events::kTick}},
        }));
}

TEST(BootstrapTest, Bootstrap_ThrowsWhenMaxTicksIsReachedWithoutAStopEvent)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;

    ReplaySource inputSource({});

    EXPECT_THROW(
        antwika::task_worker::bootstrap(
            antwika::task_worker::TaskWorkerWiring{
                .logger = logger,
                .eventSink = eventSink,
                .inputSource = inputSource,
                .workerCount = kWorkerCount,
                .maxTicks = 3}),
        EngineLoopError);
}
