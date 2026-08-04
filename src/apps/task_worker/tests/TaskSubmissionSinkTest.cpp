#include "antwika/task_worker/TaskSubmissionSink.hpp"

#include <optional>

#include <gtest/gtest.h>

#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/ecs_commons/Name.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/task_worker/Events.hpp"
#include "antwika/task_worker/JobQueue.hpp"
#include "antwika/task_worker/TaskDispatchSystem.hpp"
#include "antwika/task_worker/TaskRegistry.hpp"
#include "antwika/task_worker/TaskSubmissionError.hpp"
#include "antwika/task_worker/Worker.hpp"
#include "antwika/task_worker/WorkerLookup.hpp"

using antwika::ecs::SystemScheduler;
using antwika::ecs::World;
using antwika::ecs_commons::makeName;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::log::mocks::MockLogger;
using antwika::task_worker::JobQueue;
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
    JobQueue jobScheduler;
    SystemScheduler systemScheduler;
    static_cast<void>(systemScheduler.createPhase("dispatch"));
    TaskRegistry registry;
    TaskSubmissionSink sink(
        world, systemScheduler, jobScheduler, lookup, registry);

    sink.handle(TickEvent{
        .tick = 0,
        .event = Event{
            .name = kTaskSubmit,
            .payload = R"({"id":1,"priority":1,"durationTicks":3,)"
                       R"("label":"Alpha"})",
        },
    });

    EXPECT_EQ(jobScheduler.scheduler().pending(), 1U);

    lookup.refresh();
    const auto executed = jobScheduler.scheduler().run(0, lookup.idleCount());
    EXPECT_EQ(executed.size(), 1U);

    world.commit();
    EXPECT_EQ(
        world.get<Worker>(workerA),
        (Worker{WorkerStatus::Busy, 3, 1, makeName("Alpha")}));
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
    JobQueue jobScheduler;
    SystemScheduler systemScheduler;
    static_cast<void>(systemScheduler.createPhase("dispatch"));
    TaskRegistry registry;
    TaskSubmissionSink sink(
        world, systemScheduler, jobScheduler, lookup, registry);

    sink.handle(TickEvent{
        .tick = 0,
        .event = Event{
            .name = kTaskSubmit,
            .payload = R"({"id":1,"priority":1,"durationTicks":1,)"
                       R"("label":"First"})",
        },
    });
    sink.handle(TickEvent{
        .tick = 0,
        .event = Event{
            .name = kTaskSubmit,
            .payload = R"({"id":2,"priority":1,"durationTicks":1,)"
                       R"("label":"Second","dependsOnId":1})",
        },
    });

    EXPECT_EQ(jobScheduler.scheduler().pending(), 2U);
    EXPECT_EQ(
        registry.allTasks()[1].dependsOn,
        (std::optional<TaskDependency>{TaskDependency{1, "First"}}));

    lookup.refresh();
    const auto firstRun = jobScheduler.scheduler().run(0, lookup.idleCount());
    EXPECT_EQ(firstRun.size(), 1U);
    EXPECT_EQ(jobScheduler.scheduler().pending(), 1U);
}

TEST(TaskSubmissionSinkTest, PayloadThatIsNotValidJsonThrows)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto worker = world.create();
    world.add<Worker>(worker, Worker{});
    world.commit();

    WorkerLookup lookup(world, {worker});
    JobQueue jobScheduler;
    SystemScheduler systemScheduler;
    static_cast<void>(systemScheduler.createPhase("dispatch"));
    TaskRegistry registry;
    TaskSubmissionSink sink(
        world, systemScheduler, jobScheduler, lookup, registry);

    EXPECT_THROW(
        sink.handle(TickEvent{
            .tick = 0,
            .event = Event{
                .name = kTaskSubmit,
                .payload = "not json",
            },
        }),
        TaskSubmissionError);
}

TEST(TaskSubmissionSinkTest, PayloadMissingDurationTicksFieldThrows)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto worker = world.create();
    world.add<Worker>(worker, Worker{});
    world.commit();

    WorkerLookup lookup(world, {worker});
    JobQueue jobScheduler;
    SystemScheduler systemScheduler;
    static_cast<void>(systemScheduler.createPhase("dispatch"));
    TaskRegistry registry;
    TaskSubmissionSink sink(
        world, systemScheduler, jobScheduler, lookup, registry);

    EXPECT_THROW(
        sink.handle(TickEvent{
            .tick = 0,
            .event = Event{
                .name = kTaskSubmit,
                .payload = R"({"id":1,"priority":1,"label":"Alpha"})",
            },
        }),
        TaskSubmissionError);
}

TEST(TaskSubmissionSinkTest, PayloadMissingLabelFieldThrows)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto worker = world.create();
    world.add<Worker>(worker, Worker{});
    world.commit();

    WorkerLookup lookup(world, {worker});
    JobQueue jobScheduler;
    SystemScheduler systemScheduler;
    static_cast<void>(systemScheduler.createPhase("dispatch"));
    TaskRegistry registry;
    TaskSubmissionSink sink(
        world, systemScheduler, jobScheduler, lookup, registry);

    EXPECT_THROW(
        sink.handle(TickEvent{
            .tick = 0,
            .event = Event{
                .name = kTaskSubmit,
                .payload = R"({"id":1,"priority":1,"durationTicks":3})",
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
    JobQueue jobScheduler;
    SystemScheduler systemScheduler;
    static_cast<void>(systemScheduler.createPhase("dispatch"));
    TaskRegistry registry;
    TaskSubmissionSink sink(
        world, systemScheduler, jobScheduler, lookup, registry);

    EXPECT_THROW(
        sink.handle(TickEvent{
            .tick = 0,
            .event = Event{
                .name = kTaskSubmit,
                .payload = R"({"id":1,"priority":"oops",)"
                           R"("durationTicks":3,"label":"Alpha"})",
            },
        }),
        TaskSubmissionError);
}

TEST(TaskSubmissionSinkTest, PayloadWithNonStringLabelThrows)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto worker = world.create();
    world.add<Worker>(worker, Worker{});
    world.commit();

    WorkerLookup lookup(world, {worker});
    JobQueue jobScheduler;
    SystemScheduler systemScheduler;
    static_cast<void>(systemScheduler.createPhase("dispatch"));
    TaskRegistry registry;
    TaskSubmissionSink sink(
        world, systemScheduler, jobScheduler, lookup, registry);

    EXPECT_THROW(
        sink.handle(TickEvent{
            .tick = 0,
            .event = Event{
                .name = kTaskSubmit,
                .payload = R"({"id":1,"priority":1,)"
                           R"("durationTicks":3,"label":123})",
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
    JobQueue jobScheduler;
    SystemScheduler systemScheduler;
    static_cast<void>(systemScheduler.createPhase("dispatch"));
    TaskRegistry registry;
    TaskSubmissionSink sink(
        world, systemScheduler, jobScheduler, lookup, registry);

    EXPECT_THROW(
        sink.handle(TickEvent{
            .tick = 0,
            .event = Event{
                .name = kTaskSubmit,
                .payload = R"({"id":1,"priority":1,"durationTicks":0,)"
                           R"("label":"Alpha"})",
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
    JobQueue jobScheduler;
    SystemScheduler systemScheduler;
    static_cast<void>(systemScheduler.createPhase("dispatch"));
    TaskRegistry registry;
    TaskSubmissionSink sink(
        world, systemScheduler, jobScheduler, lookup, registry);

    EXPECT_THROW(
        sink.handle(TickEvent{
            .tick = 0,
            .event = Event{
                .name = kTaskSubmit,
                .payload = R"({"id":1,"priority":256,)"
                           R"("durationTicks":3,"label":"Alpha"})",
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
    JobQueue jobScheduler;
    SystemScheduler systemScheduler;
    static_cast<void>(systemScheduler.createPhase("dispatch"));
    TaskRegistry registry;
    TaskSubmissionSink sink(
        world, systemScheduler, jobScheduler, lookup, registry);

    sink.handle(TickEvent{
        .tick = 0,
        .event = Event{
            .name = kTaskSubmit,
            .payload = R"({"id":1,"priority":1,"durationTicks":3,)"
                       R"("label":"Alpha"})",
        },
    });

    EXPECT_THROW(
        sink.handle(TickEvent{
            .tick = 0,
            .event = Event{
                .name = kTaskSubmit,
                .payload = R"({"id":1,"priority":1,"durationTicks":3,)"
                           R"("label":"AlphaAgain"})",
            },
        }),
        TaskSubmissionError);
}

// Unlike the earlier CSV wire format, JSON needs no comma escaping.
// A label containing a literal comma is just string content.
// It never gets misparsed as an extra dependsOnId field.
TEST(
    TaskSubmissionSinkTest,
    LabelContainingACommaIsNoLongerMisparsedAsADependsOnId)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto worker = world.create();
    world.add<Worker>(worker, Worker{});
    world.commit();

    WorkerLookup lookup(world, {worker});
    JobQueue jobScheduler;
    SystemScheduler systemScheduler;
    static_cast<void>(systemScheduler.createPhase("dispatch"));
    TaskRegistry registry;
    TaskSubmissionSink sink(
        world, systemScheduler, jobScheduler, lookup, registry);

    sink.handle(TickEvent{
        .tick = 0,
        .event = Event{
            .name = kTaskSubmit,
            .payload = R"({"id":1,"priority":1,"durationTicks":5,)"
                       R"("label":"First"})",
        },
    });
    sink.handle(TickEvent{
        .tick = 0,
        .event = Event{
            .name = kTaskSubmit,
            .payload = R"({"id":2,"priority":1,"durationTicks":3,)"
                       R"("label":"Se,cond","dependsOnId":1})",
        },
    });

    EXPECT_EQ(
        registry.allTasks()[1].dependsOn,
        (std::optional<TaskDependency>{TaskDependency{1, "First"}}));
    EXPECT_EQ(registry.allTasks()[1].label, "Se,cond");
}

TEST(TaskSubmissionSinkTest, PayloadWithNonNumericDependsOnIdThrows)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto worker = world.create();
    world.add<Worker>(worker, Worker{});
    world.commit();

    WorkerLookup lookup(world, {worker});
    JobQueue jobScheduler;
    SystemScheduler systemScheduler;
    static_cast<void>(systemScheduler.createPhase("dispatch"));
    TaskRegistry registry;
    TaskSubmissionSink sink(
        world, systemScheduler, jobScheduler, lookup, registry);

    sink.handle(TickEvent{
        .tick = 0,
        .event = Event{
            .name = kTaskSubmit,
            .payload = R"({"id":1,"priority":1,"durationTicks":1,)"
                       R"("label":"First"})",
        },
    });

    EXPECT_THROW(
        sink.handle(TickEvent{
            .tick = 0,
            .event = Event{
                .name = kTaskSubmit,
                .payload = R"({"id":2,"priority":1,"durationTicks":1,)"
                           R"("label":"Second","dependsOnId":"bad"})",
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
    JobQueue jobScheduler;
    SystemScheduler systemScheduler;
    static_cast<void>(systemScheduler.createPhase("dispatch"));
    TaskRegistry registry;
    TaskSubmissionSink sink(
        world, systemScheduler, jobScheduler, lookup, registry);

    EXPECT_THROW(
        sink.handle(TickEvent{
            .tick = 0,
            .event = Event{
                .name = kTaskSubmit,
                .payload = R"({"id":1,"priority":1,"durationTicks":1,)"
                           R"("label":"Orphan","dependsOnId":999})",
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
    JobQueue jobScheduler;
    SystemScheduler systemScheduler;
    TaskRegistry registry;
    TaskDispatchSystem dispatchSystem(jobScheduler, lookup, registry);
    const auto phase = systemScheduler.createPhase("dispatch");
    systemScheduler.addSystem(phase, dispatchSystem);
    TaskSubmissionSink sink(
        world, systemScheduler, jobScheduler, lookup, registry);

    sink.handle(TickEvent{
        .tick = 0,
        .event = Event{
            .name = kTaskSubmit,
            .payload = R"({"id":1,"priority":1,"durationTicks":3,)"
                       R"("label":"Alpha"})",
        },
    });
    sink.handle(TickEvent{
        .tick = 0,
        .event = Event{.name = antwika::engine::events::kTick},
    });

    EXPECT_TRUE(jobScheduler.scheduler().empty());
    EXPECT_EQ(
        world.get<Worker>(worker),
        (Worker{WorkerStatus::Busy, 3, 1, makeName("Alpha")}));
}
