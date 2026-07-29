#include <gtest/gtest.h>

#include <optional>
#include <vector>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/EventRecorder.hpp>
#include <antwika/event/ReplayRecorder.hpp>
#include <antwika/event/TimedEvent.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/MinimumLevelLogPolicy.hpp>
#include <antwika/log/NullAppender.hpp>
#include <antwika/log/PlainFormatter.hpp>
#include <antwika/replay/EngineLoopError.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/fakes/FakeClock.hpp>

#include <antwika/scheduler/Priority.hpp>

#include "antwika/task_worker/Events.hpp"
#include "antwika/task_worker/TaskRegistry.hpp"
#include "antwika/task_worker/TaskWorker.hpp"
#include "antwika/task_worker/Worker.hpp"

using antwika::ecs::ISystem;
using antwika::ecs::World;
using antwika::event::Event;
using antwika::event::EventRecorder;
using antwika::event::ReplayRecorder;
using antwika::event::TimedEvent;
using antwika::log::Level;
using antwika::log::MinimumLevelLogPolicy;
using antwika::log::NullAppender;
using antwika::log::PlainFormatter;
using antwika::replay::EngineLoopError;
using antwika::replay::ReplaySource;
using antwika::scheduler::kCriticalPriority;
using antwika::scheduler::kLowPriority;
using antwika::scheduler::kNormalPriority;
using antwika::task_worker::makeWorkerLabel;
using antwika::task_worker::TaskDependency;
using antwika::task_worker::TaskInfo;
using antwika::task_worker::TaskRegistry;
using antwika::task_worker::TaskStatus;
using antwika::task_worker::Worker;
using antwika::task_worker::WorkerStatus;
using antwika::time::fakes::FakeClock;

namespace
{
    using antwika::task_worker::events::kTaskSubmit;
    constexpr int kExpectedTicks = 6;
    constexpr antwika::time::Tick kMaxTicks = 10;
    constexpr std::uint32_t kWorkerCount = 2;

    // Same scenario as main.cpp's demoScript().
    // Gamma (Low) waits ticks 0-4: multi-tick distribution.
    // Delta (Critical, submitted tick 4) jumps ahead of Gamma.
    // Epsilon depends on Delta but can't run in Delta's run() call.
    // Epsilon runs the following tick: a cross-tick dependency.
    // See blog/006-... for the full scenario rationale.
    // Ends with engine.stop at tick 5, once every task has settled.
    std::vector<TimedEvent> demoScript()
    {
        return {
            TimedEvent{
                .tick = 0,
                .event = Event{
                    .name = kTaskSubmit,
                    .payload = R"({"id":1,"priority":1,)"
                               R"("durationTicks":4,"label":"Alpha"})"}},
            TimedEvent{
                .tick = 0,
                .event = Event{
                    .name = kTaskSubmit,
                    .payload = R"({"id":2,"priority":1,)"
                               R"("durationTicks":5,"label":"Beta"})"}},
            TimedEvent{
                .tick = 0,
                .event = Event{
                    .name = kTaskSubmit,
                    .payload = R"({"id":3,"priority":0,)"
                               R"("durationTicks":2,"label":"Gamma"})"}},
            TimedEvent{
                .tick = 4,
                .event = Event{
                    .name = kTaskSubmit,
                    .payload = R"({"id":4,"priority":3,)"
                               R"("durationTicks":1,"label":"Delta"})"}},
            TimedEvent{
                .tick = 4,
                .event = Event{
                    .name = kTaskSubmit,
                    .payload = R"({"id":5,"priority":1,)"
                               R"("durationTicks":1,"label":"Epsilon",)"
                               R"("dependsOnId":4})"}},
            TimedEvent{
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
} // namespace

TEST(BootstrapTest, Bootstrap_RunsScriptedTasksToCompletion)
{
    std::chrono::system_clock::time_point time{};
    FakeClock fakeClock(time);
    NullAppender appender;
    PlainFormatter formatter;
    MinimumLevelLogPolicy logPolicy(Level::Info);
    EventRecorder eventSink;

    auto script = demoScript();
    ReplaySource inputSource(script);

    auto finalState = antwika::task_worker::bootstrap(
        fakeClock,
        appender,
        formatter,
        logPolicy,
        eventSink,
        inputSource,
        kWorkerCount,
        {},
        nullptr,
        kMaxTicks);

    // At tick 5, Delta's and Beta's workers free simultaneously.
    // Epsilon (Normal) now outranks Gamma (Low) for the freed slot.
    // Epsilon claims the lower-index worker; Gamma gets the other.
    ASSERT_EQ(finalState.size(), 2U);
    EXPECT_EQ(
        finalState[0],
        (Worker{WorkerStatus::Busy, 1, 5, makeWorkerLabel("Epsilon")}));
    EXPECT_EQ(
        finalState[1],
        (Worker{WorkerStatus::Busy, 2, 3, makeWorkerLabel("Gamma")}));
}

TEST(BootstrapTest, Bootstrap_RunsEveryObserverOncePerTick)
{
    std::chrono::system_clock::time_point time{};
    FakeClock fakeClock(time);
    NullAppender appender;
    PlainFormatter formatter;
    MinimumLevelLogPolicy logPolicy(Level::Info);
    EventRecorder eventSink;

    auto script = demoScript();
    ReplaySource inputSource(script);
    CallCountingSystem countingSystem;

    antwika::task_worker::bootstrap(
        fakeClock,
        appender,
        formatter,
        logPolicy,
        eventSink,
        inputSource,
        kWorkerCount,
        {countingSystem},
        nullptr,
        kMaxTicks);

    EXPECT_EQ(countingSystem.calls, kExpectedTicks);
}

TEST(BootstrapTest, Bootstrap_KeepsACallerSuppliedRegistryInSync)
{
    std::chrono::system_clock::time_point time{};
    FakeClock fakeClock(time);
    NullAppender appender;
    PlainFormatter formatter;
    MinimumLevelLogPolicy logPolicy(Level::Info);
    EventRecorder eventSink;

    auto script = demoScript();
    ReplaySource inputSource(script);
    TaskRegistry registry;

    antwika::task_worker::bootstrap(
        fakeClock,
        appender,
        formatter,
        logPolicy,
        eventSink,
        inputSource,
        kWorkerCount,
        {},
        &registry,
        kMaxTicks);

    // By tick 5, Alpha/Beta/Delta have completed.
    // Gamma and Epsilon are still running, both just started.
    // Gamma's 2-tick duration leaves two; Epsilon's 1-tick leaves one.
    // See Bootstrap_RunsScriptedTasksToCompletion for the worker view.
    EXPECT_EQ(
        registry.allTasks(),
        (std::vector<TaskInfo>{
            TaskInfo{
                1, "Alpha", kNormalPriority, TaskStatus::Completed, 0,
                std::nullopt},
            TaskInfo{
                2, "Beta", kNormalPriority, TaskStatus::Completed, 0,
                std::nullopt},
            TaskInfo{
                3, "Gamma", kLowPriority, TaskStatus::Running, 2,
                std::nullopt},
            TaskInfo{
                4, "Delta", kCriticalPriority, TaskStatus::Completed, 0,
                std::nullopt},
            TaskInfo{
                5, "Epsilon", kNormalPriority, TaskStatus::Running, 1,
                TaskDependency{4, "Delta"}}}));
}

TEST(BootstrapTest, Bootstrap_WithNoScriptedInputAllWorkersStayIdle)
{
    std::chrono::system_clock::time_point time{};
    FakeClock fakeClock(time);
    NullAppender appender;
    PlainFormatter formatter;
    MinimumLevelLogPolicy logPolicy(Level::Info);
    EventRecorder eventSink;

    ReplaySource inputSource({
        TimedEvent{
            .tick = 2,
            .event = Event{.name = antwika::engine::events::kStop}},
    });

    auto finalState = antwika::task_worker::bootstrap(
        fakeClock,
        appender,
        formatter,
        logPolicy,
        eventSink,
        inputSource,
        2,
        {},
        nullptr,
        kMaxTicks);

    EXPECT_EQ(finalState[0], (Worker{WorkerStatus::Idle, 0}));
    EXPECT_EQ(finalState[1], (Worker{WorkerStatus::Idle, 0}));
}

// A caller wanting to persist a `--record` file has no pre-known script.
// It instead passes an optional replayRecorder.
// bootstrap() must register it so it observes every dispatched event.
TEST(BootstrapTest, Bootstrap_ForwardsDispatchedEventsToAReplayRecorder)
{
    std::chrono::system_clock::time_point time{};
    FakeClock fakeClock(time);
    NullAppender appender;
    PlainFormatter formatter;
    MinimumLevelLogPolicy logPolicy(Level::Info);
    EventRecorder eventSink;

    ReplaySource inputSource({
        TimedEvent{
            .tick = 0,
            .event = Event{
                .name = kTaskSubmit,
                .payload = R"({"id":1,"priority":1,)"
                           R"("durationTicks":4,"label":"Alpha"})"}},
        TimedEvent{
            .tick = 0,
            .event = Event{.name = antwika::engine::events::kStop}},
    });
    ReplayRecorder replayRecorder;

    antwika::task_worker::bootstrap(
        fakeClock,
        appender,
        formatter,
        logPolicy,
        eventSink,
        inputSource,
        kWorkerCount,
        {},
        nullptr,
        kMaxTicks,
        &replayRecorder);

    EXPECT_EQ(
        replayRecorder.getEvents(),
        (std::vector<TimedEvent>{
            TimedEvent{
                .tick = 0,
                .event = Event{.name = "Running Antwika TaskWorker"}},
            TimedEvent{
                .tick = 0,
                .event = Event{
                    .name = kTaskSubmit,
                    .payload = R"({"id":1,"priority":1,)"
                               R"("durationTicks":4,"label":"Alpha"})"}},
            TimedEvent{
                .tick = 0,
                .event = Event{.name = antwika::engine::events::kStop}},
            TimedEvent{
                .tick = 0,
                .event = Event{.name = antwika::engine::events::kTick}},
        }));
}

// Safety valve: a run that never dispatches engine.stop must fail loudly.
// It should not hang or silently truncate once maxTicks is reached.
TEST(BootstrapTest, Bootstrap_ThrowsWhenMaxTicksIsReachedWithoutAStopEvent)
{
    std::chrono::system_clock::time_point time{};
    FakeClock fakeClock(time);
    NullAppender appender;
    PlainFormatter formatter;
    MinimumLevelLogPolicy logPolicy(Level::Info);
    EventRecorder eventSink;

    ReplaySource inputSource({});

    EXPECT_THROW(
        antwika::task_worker::bootstrap(
            fakeClock,
            appender,
            formatter,
            logPolicy,
            eventSink,
            inputSource,
            kWorkerCount,
            {},
            nullptr,
            3),
        EngineLoopError);
}
