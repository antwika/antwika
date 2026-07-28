#include <gtest/gtest.h>

#include <optional>
#include <vector>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/EventRecorder.hpp>
#include <antwika/event/TimedEvent.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/MinimumLevelLogPolicy.hpp>
#include <antwika/log/NullAppender.hpp>
#include <antwika/log/PlainFormatter.hpp>
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
using antwika::event::TimedEvent;
using antwika::log::Level;
using antwika::log::MinimumLevelLogPolicy;
using antwika::log::NullAppender;
using antwika::log::PlainFormatter;
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
    constexpr antwika::time::Tick kTotalTicks = 6;
    constexpr std::uint32_t kWorkerCount = 2;

    // Same scenario as main.cpp's demoScript().
    // Gamma (Low) waits ticks 0-4: multi-tick distribution.
    // Delta (Critical, submitted tick 4) jumps ahead of Gamma.
    // Epsilon depends on Delta but can't run in Delta's run() call.
    // Epsilon runs the following tick: a cross-tick dependency.
    // See blog/006-... for the full scenario rationale.
    std::vector<TimedEvent> demoScript()
    {
        return {
            TimedEvent{
                .tick = 0,
                .event = Event{
                    .name = kTaskSubmit, .payload = "1,1,4,Alpha"}},
            TimedEvent{
                .tick = 0,
                .event = Event{
                    .name = kTaskSubmit, .payload = "2,1,5,Beta"}},
            TimedEvent{
                .tick = 0,
                .event = Event{
                    .name = kTaskSubmit, .payload = "3,0,1,Gamma"}},
            TimedEvent{
                .tick = 4,
                .event = Event{
                    .name = kTaskSubmit, .payload = "4,3,1,Delta"}},
            TimedEvent{
                .tick = 4,
                .event = Event{
                    .name = kTaskSubmit,
                    .payload = "5,1,1,Epsilon,4"}},
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
        kTotalTicks,
        kWorkerCount);

    // At tick 5, Delta's and Beta's workers free simultaneously.
    // Epsilon (Normal) now outranks Gamma (Low) for the freed slot.
    // Epsilon claims the lower-index worker; Gamma gets the other.
    ASSERT_EQ(finalState.size(), 2U);
    EXPECT_EQ(
        finalState[0],
        (Worker{WorkerStatus::Busy, 1, 5, makeWorkerLabel("Epsilon")}));
    EXPECT_EQ(
        finalState[1],
        (Worker{WorkerStatus::Busy, 1, 3, makeWorkerLabel("Gamma")}));
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
        kTotalTicks,
        kWorkerCount,
        {countingSystem});

    EXPECT_EQ(countingSystem.calls, static_cast<int>(kTotalTicks));
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
        kTotalTicks,
        kWorkerCount,
        {},
        &registry);

    // By tick 5, Alpha/Beta/Delta have completed.
    // Gamma and Epsilon are still running with one tick left each.
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
                3, "Gamma", kLowPriority, TaskStatus::Running, 1,
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

    ReplaySource inputSource({});

    auto finalState = antwika::task_worker::bootstrap(
        fakeClock,
        appender,
        formatter,
        logPolicy,
        eventSink,
        inputSource,
        3,
        2);

    EXPECT_EQ(finalState[0], (Worker{WorkerStatus::Idle, 0}));
    EXPECT_EQ(finalState[1], (Worker{WorkerStatus::Idle, 0}));
}
