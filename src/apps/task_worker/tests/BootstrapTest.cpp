#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <antwika/ecs/ISystem.hpp>
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

#include "antwika/task_worker/Events.hpp"
#include "antwika/task_worker/PoolSnapshot.hpp"
#include "antwika/task_worker/TaskRegistry.hpp"
#include "antwika/task_worker/TaskWorker.hpp"
#include "antwika/task_worker/Worker.hpp"

using antwika::ecs::ISystem;
using antwika::ecs::World;
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
using ::testing::NiceMock;

namespace
{
    using antwika::task_worker::events::kTaskSubmit;
    constexpr int kExpectedTicks = 6;
    constexpr antwika::time::Tick kMaxTicks = 10;
    constexpr std::uint32_t kWorkerCount = 2;

    // Same scenario as replays/demo.jsonl.
    // Gamma (Low) waits ticks 0-4: multi-tick distribution.
    // Delta (Critical, submitted tick 4) jumps ahead of Gamma.
    // Epsilon depends on Delta but can't run in Delta's run() call.
    // Epsilon runs the following tick: a cross-tick dependency.
    // See blog/006-... for the full scenario rationale.
    // Ends with engine.stop at tick 5, once every task has settled.
    std::vector<TickEvent> demoScript()
    {
        return {
            TickEvent{
                .tick = 0,
                .event = Event{
                    .name = kTaskSubmit,
                    .payload = R"({"id":1,"priority":1,)"
                               R"("durationTicks":4,"label":"Alpha"})"}},
            TickEvent{
                .tick = 0,
                .event = Event{
                    .name = kTaskSubmit,
                    .payload = R"({"id":2,"priority":1,)"
                               R"("durationTicks":5,"label":"Beta"})"}},
            TickEvent{
                .tick = 0,
                .event = Event{
                    .name = kTaskSubmit,
                    .payload = R"({"id":3,"priority":0,)"
                               R"("durationTicks":2,"label":"Gamma"})"}},
            TickEvent{
                .tick = 4,
                .event = Event{
                    .name = kTaskSubmit,
                    .payload = R"({"id":4,"priority":3,)"
                               R"("durationTicks":1,"label":"Delta"})"}},
            TickEvent{
                .tick = 4,
                .event = Event{
                    .name = kTaskSubmit,
                    .payload = R"({"id":5,"priority":1,)"
                               R"("durationTicks":1,"label":"Epsilon",)"
                               R"("dependsOnId":4})"}},
            TickEvent{
                .tick = 5,
                .event = Event{.name = antwika::engine::events::kStop}},
        };
    }

    class CallCountingSystem final : public ISystem
    {
    public:
        void update(World &, antwika::time::Tick) override
        {
            ++calls;
        }

        int calls = 0;
    };

    // Takes the picture the window would draw, every tick.
    // Through the same snapshotOf() the RenderSystem draws from.
    // Registered as an observer, so it sees what a frame sees.
    class SnapshotRecordingSystem final : public ISystem
    {
    public:
        explicit SnapshotRecordingSystem(const TaskRegistry &registry)
            : registry(registry)
        {
        }

        SnapshotRecordingSystem(const SnapshotRecordingSystem &) = delete;
        SnapshotRecordingSystem(SnapshotRecordingSystem &&) = delete;

        SnapshotRecordingSystem &operator=(
            const SnapshotRecordingSystem &) = delete;
        SnapshotRecordingSystem &operator=(
            SnapshotRecordingSystem &&) = delete;

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
} // namespace

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
            .maxTicks = kMaxTicks});

    // At tick 5, Delta's and Beta's workers free simultaneously.
    // Epsilon (Normal) now outranks Gamma (Low) for the freed slot.
    // Epsilon claims the lower-index worker; Gamma gets the other.
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
    CallCountingSystem countingSystem;

    antwika::task_worker::bootstrap(
        antwika::task_worker::TaskWorkerWiring{
            .logger = logger,
            .eventSink = eventSink,
            .inputSource = inputSource,
            .workerCount = kWorkerCount,
            .observers = {countingSystem},
            .maxTicks = kMaxTicks});

    EXPECT_EQ(countingSystem.calls, kExpectedTicks);
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

    // By tick 5, Alpha/Beta/Delta have completed.
    // Gamma and Epsilon are still running, both just started.
    // Gamma's 2-tick duration leaves two; Epsilon's 1-tick leaves one.
    // See Bootstrap_RunsScriptedTasksToCompletion for the worker view.
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

// What the window draws is a projection of the run, not a second one.
// This is where the two are held against each other.
// The queue is drawn in the order the scheduler pulls from.
// The budget drawn each tick is the one it was actually run with.
//
// Gamma is low priority and waits from tick 0 to tick 5, on screen.
// Delta arrives at tick 4 and takes the only free worker.
// So the budget that tick is 1, and Gamma stays where it is.
// Epsilon arrives with it and is drawn as waiting for Delta.
// A blocked task is not a candidate for a worker at all.
// Which is why it can jump Gamma at tick 5, on priority.
TEST(BootstrapTest, Bootstrap_DrawsTheQueueTheSchedulerWillActuallyPull)
{
    NiceMock<MockLogger> logger;
    NiceMock<MockEventSink> eventSink;

    auto script = demoScript();
    ReplaySource inputSource(script);
    TaskRegistry registry;
    SnapshotRecordingSystem frames(registry);

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
            .maxTicks = kMaxTicks});

    EXPECT_EQ(finalState[0], (Worker{WorkerStatus::Idle, 0}));
    EXPECT_EQ(finalState[1], (Worker{WorkerStatus::Idle, 0}));
}

// A caller wanting to persist a `--record` file has no pre-known script.
// It instead passes an optional replayRecorder.
// bootstrap() must register it so it observes every dispatched event.
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

// Safety valve: a run that never dispatches engine.stop must fail loudly.
// It should not hang or silently truncate once maxTicks is reached.
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
