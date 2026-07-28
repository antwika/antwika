#include "antwika/task_worker/TaskSubmissionSink.hpp"

#include <gtest/gtest.h>

#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/scheduler/Scheduler.hpp>

#include "antwika/task_worker/Events.hpp"
#include "antwika/task_worker/TaskDispatchSystem.hpp"
#include "antwika/task_worker/TaskSubmissionError.hpp"
#include "antwika/task_worker/Worker.hpp"
#include "antwika/task_worker/WorkerLookup.hpp"

using antwika::ecs::SystemScheduler;
using antwika::ecs::World;
using antwika::event::Event;
using antwika::event::TimedEvent;
using antwika::log::mocks::MockLogger;
using antwika::scheduler::Scheduler;
using antwika::task_worker::TaskDispatchSystem;
using antwika::task_worker::TaskSubmissionError;
using antwika::task_worker::TaskSubmissionSink;
using antwika::task_worker::Worker;
using antwika::task_worker::WorkerLookup;
using antwika::task_worker::WorkerStatus;
using ::testing::NiceMock;

namespace
{
    using antwika::task_worker::events::kTaskSubmit;
} // namespace

TEST(TaskSubmissionSinkTest, ParsesAPayloadIntoAScheduledTaskAtItsPriority)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto workerA = world.create();
    world.add<Worker>(workerA, Worker{});
    const auto workerB = world.create();
    world.add<Worker>(workerB, Worker{});
    world.commit();

    WorkerLookup lookup(world, {workerA, workerB});
    Scheduler jobScheduler;
    SystemScheduler systemScheduler;
    static_cast<void>(systemScheduler.createPhase("dispatch"));
    TaskSubmissionSink sink(world, systemScheduler, jobScheduler, lookup);

    sink.handle(TimedEvent{
        .tick = 0,
        .event = Event{
            .name = kTaskSubmit,
            .payload = "1,1,3,Alpha",
        },
    });

    EXPECT_EQ(jobScheduler.pending(), 1U);

    lookup.refresh();
    const auto executed = jobScheduler.run(0, lookup.idleCount());
    EXPECT_EQ(executed.size(), 1U);

    world.commit();
    EXPECT_EQ(world.get<Worker>(workerA), (Worker{WorkerStatus::Busy, 3}));
}

TEST(
    TaskSubmissionSinkTest,
    DependsOnIdResolvesToTheRightJobIdAndBlocksUntilItRuns)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto worker = world.create();
    world.add<Worker>(worker, Worker{});
    world.commit();

    WorkerLookup lookup(world, {worker});
    Scheduler jobScheduler;
    SystemScheduler systemScheduler;
    static_cast<void>(systemScheduler.createPhase("dispatch"));
    TaskSubmissionSink sink(world, systemScheduler, jobScheduler, lookup);

    sink.handle(TimedEvent{
        .tick = 0,
        .event = Event{
            .name = kTaskSubmit,
            .payload = "1,1,1,First",
        },
    });
    sink.handle(TimedEvent{
        .tick = 0,
        .event = Event{
            .name = kTaskSubmit,
            .payload = "2,1,1,Second,1",
        },
    });

    EXPECT_EQ(jobScheduler.pending(), 2U);

    lookup.refresh();
    const auto firstRun = jobScheduler.run(0, lookup.idleCount());
    EXPECT_EQ(firstRun.size(), 1U);
    EXPECT_EQ(jobScheduler.pending(), 1U);
}

TEST(TaskSubmissionSinkTest, UnresolvableDependsOnIdThrows)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto worker = world.create();
    world.add<Worker>(worker, Worker{});
    world.commit();

    WorkerLookup lookup(world, {worker});
    Scheduler jobScheduler;
    SystemScheduler systemScheduler;
    static_cast<void>(systemScheduler.createPhase("dispatch"));
    TaskSubmissionSink sink(world, systemScheduler, jobScheduler, lookup);

    EXPECT_THROW(
        sink.handle(TimedEvent{
            .tick = 0,
            .event = Event{
                .name = kTaskSubmit,
                .payload = "1,1,1,Orphan,999",
            },
        }),
        TaskSubmissionError);
}

TEST(TaskSubmissionSinkTest, TickEventCommitsAndRunsSystemScheduler)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto worker = world.create();
    world.add<Worker>(worker, Worker{});
    world.commit();

    WorkerLookup lookup(world, {worker});
    Scheduler jobScheduler;
    SystemScheduler systemScheduler;
    TaskDispatchSystem dispatchSystem(jobScheduler, lookup);
    const auto phase = systemScheduler.createPhase("dispatch");
    systemScheduler.addSystem(phase, dispatchSystem);
    TaskSubmissionSink sink(world, systemScheduler, jobScheduler, lookup);

    sink.handle(TimedEvent{
        .tick = 0,
        .event = Event{
            .name = kTaskSubmit,
            .payload = "1,1,3,Alpha",
        },
    });
    sink.handle(TimedEvent{
        .tick = 0,
        .event = Event{.name = antwika::engine::events::kTick},
    });

    EXPECT_TRUE(jobScheduler.empty());
    EXPECT_EQ(world.get<Worker>(worker), (Worker{WorkerStatus::Busy, 3}));
}
