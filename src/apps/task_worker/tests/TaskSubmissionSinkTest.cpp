#include "antwika/task_worker/TaskSubmissionSink.hpp"

#include <optional>

#include <gtest/gtest.h>

#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/scheduler/Scheduler.hpp>

#include "antwika/task_worker/Events.hpp"
#include "antwika/task_worker/TaskDispatchSystem.hpp"
#include "antwika/task_worker/TaskRegistry.hpp"
#include "antwika/task_worker/TaskSubmissionError.hpp"
#include "antwika/task_worker/Worker.hpp"
#include "antwika/task_worker/WorkerLookup.hpp"

using antwika::ecs::SystemScheduler;
using antwika::ecs::World;
using antwika::event::Event;
using antwika::event::TimedEvent;
using antwika::log::mocks::MockLogger;
using antwika::scheduler::Scheduler;
using antwika::task_worker::makeWorkerLabel;
using antwika::task_worker::TaskDependency;
using antwika::task_worker::TaskDispatchSystem;
using antwika::task_worker::TaskRegistry;
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
    TaskRegistry registry;
    TaskSubmissionSink sink(
        world, systemScheduler, jobScheduler, lookup, registry);

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
    EXPECT_EQ(
        world.get<Worker>(workerA),
        (Worker{WorkerStatus::Busy, 3, 1, makeWorkerLabel("Alpha")}));
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
    TaskRegistry registry;
    TaskSubmissionSink sink(
        world, systemScheduler, jobScheduler, lookup, registry);

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
    EXPECT_EQ(
        registry.allTasks()[1].dependsOn,
        (std::optional<TaskDependency>{TaskDependency{1, "First"}}));

    lookup.refresh();
    const auto firstRun = jobScheduler.run(0, lookup.idleCount());
    EXPECT_EQ(firstRun.size(), 1U);
    EXPECT_EQ(jobScheduler.pending(), 1U);
}

TEST(TaskSubmissionSinkTest, PayloadWithTooFewFieldsThrows)
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
    TaskRegistry registry;
    TaskSubmissionSink sink(
        world, systemScheduler, jobScheduler, lookup, registry);

    EXPECT_THROW(
        sink.handle(TimedEvent{
            .tick = 0,
            .event = Event{
                .name = kTaskSubmit,
                .payload = "1,1,1",
            },
        }),
        TaskSubmissionError);
}

TEST(TaskSubmissionSinkTest, NonNumericFieldThrows)
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
    TaskRegistry registry;
    TaskSubmissionSink sink(
        world, systemScheduler, jobScheduler, lookup, registry);

    EXPECT_THROW(
        sink.handle(TimedEvent{
            .tick = 0,
            .event = Event{
                .name = kTaskSubmit,
                .payload = "1,oops,3,Alpha",
            },
        }),
        TaskSubmissionError);
}

TEST(TaskSubmissionSinkTest, TrailingGarbageAfterANumberThrows)
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
    TaskRegistry registry;
    TaskSubmissionSink sink(
        world, systemScheduler, jobScheduler, lookup, registry);

    EXPECT_THROW(
        sink.handle(TimedEvent{
            .tick = 0,
            .event = Event{
                .name = kTaskSubmit,
                .payload = "1,1,3x,Alpha",
            },
        }),
        TaskSubmissionError);
}

TEST(TaskSubmissionSinkTest, ZeroDurationTicksThrows)
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
    TaskRegistry registry;
    TaskSubmissionSink sink(
        world, systemScheduler, jobScheduler, lookup, registry);

    EXPECT_THROW(
        sink.handle(TimedEvent{
            .tick = 0,
            .event = Event{
                .name = kTaskSubmit,
                .payload = "1,1,0,Alpha",
            },
        }),
        TaskSubmissionError);
}

TEST(TaskSubmissionSinkTest, PriorityAboveUInt8RangeThrows)
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
    TaskRegistry registry;
    TaskSubmissionSink sink(
        world, systemScheduler, jobScheduler, lookup, registry);

    EXPECT_THROW(
        sink.handle(TimedEvent{
            .tick = 0,
            .event = Event{
                .name = kTaskSubmit,
                .payload = "1,256,3,Alpha",
            },
        }),
        TaskSubmissionError);
}

TEST(TaskSubmissionSinkTest, DuplicateTaskIdThrows)
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
    TaskRegistry registry;
    TaskSubmissionSink sink(
        world, systemScheduler, jobScheduler, lookup, registry);

    sink.handle(TimedEvent{
        .tick = 0,
        .event = Event{
            .name = kTaskSubmit,
            .payload = "1,1,3,Alpha",
        },
    });

    EXPECT_THROW(
        sink.handle(TimedEvent{
            .tick = 0,
            .event = Event{
                .name = kTaskSubmit,
                .payload = "1,1,3,AlphaAgain",
            },
        }),
        TaskSubmissionError);
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
    TaskRegistry registry;
    TaskSubmissionSink sink(
        world, systemScheduler, jobScheduler, lookup, registry);

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
    TaskRegistry registry;
    TaskDispatchSystem dispatchSystem(jobScheduler, lookup, registry);
    const auto phase = systemScheduler.createPhase("dispatch");
    systemScheduler.addSystem(phase, dispatchSystem);
    TaskSubmissionSink sink(
        world, systemScheduler, jobScheduler, lookup, registry);

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
    EXPECT_EQ(
        world.get<Worker>(worker),
        (Worker{WorkerStatus::Busy, 3, 1, makeWorkerLabel("Alpha")}));
}
