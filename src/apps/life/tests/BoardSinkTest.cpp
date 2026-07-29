#include "antwika/life/BoardSink.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <string_view>

#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/life/Board.hpp"
#include "antwika/life/BoardSinkError.hpp"
#include "antwika/life/Cell.hpp"
#include "antwika/life/Events.hpp"
#include "antwika/life/Grid.hpp"
#include "antwika/life/LifeSystem.hpp"

using antwika::ecs::SystemScheduler;
using antwika::ecs::World;
using antwika::event::Event;
using antwika::event::TimedEvent;
using antwika::life::BoardSink;
using antwika::life::BoardSinkError;
using antwika::life::Cell;
using antwika::life::Grid;
using antwika::life::LifeSystem;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{
    // BoardSink only drives an already-configured scheduler.
    // Registering LifeSystem into a phase is bootstrap()'s job, not this.
    // Every test that exercises kTick wires that up the same way here.
    void registerLifeSystem(SystemScheduler &scheduler, LifeSystem &system)
    {
        const auto phase = scheduler.createPhase("life");
        scheduler.addSystem(phase, system);
    }
} // namespace

TEST(BoardSinkTest, ToggleCellEventsStageA2x2BlockThatSurvivesTheNextTick)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 4, 4);
    world.commit();

    SystemScheduler scheduler;
    LifeSystem system(grid);
    registerLifeSystem(scheduler, system);
    BoardSink sink(world, grid, scheduler);

    for (const std::string_view payload : {"1,1", "2,1", "1,2", "2,2"})
    {
        sink.handle(TimedEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = std::string(payload),
            },
        });
    }

    // Staged into the back buffer only -- not yet visible before a commit.
    EXPECT_FALSE(world.get<Cell>(grid.entityAt(1, 1)).alive);

    sink.handle(TimedEvent{
        .tick = 0,
        .event = Event{.name = antwika::engine::events::kTick},
    });

    // A 2x2 block is a still life, so it survives a generation unchanged.
    // That confirms the toggle took effect.
    // Nothing else could have made these four cells alive.
    EXPECT_TRUE(world.get<Cell>(grid.entityAt(1, 1)).alive);
    EXPECT_TRUE(world.get<Cell>(grid.entityAt(2, 1)).alive);
    EXPECT_TRUE(world.get<Cell>(grid.entityAt(1, 2)).alive);
    EXPECT_TRUE(world.get<Cell>(grid.entityAt(2, 2)).alive);
}

TEST(BoardSinkTest, TickEventRunsLifeSystemLettingAnIsolatedCellDie)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 3, 3);
    world.commit();

    SystemScheduler scheduler;
    LifeSystem system(grid);
    registerLifeSystem(scheduler, system);
    BoardSink sink(world, grid, scheduler);

    sink.handle(TimedEvent{
        .tick = 0,
        .event = Event{
            .name = antwika::life::events::kToggleCell,
            .payload = "1,1",
        },
    });
    sink.handle(TimedEvent{
        .tick = 0,
        .event = Event{.name = antwika::engine::events::kTick},
    });

    // The toggled cell has zero live neighbors, so it dies immediately.
    // This proves kTick both commits the toggle and runs a generation.
    EXPECT_FALSE(world.get<Cell>(grid.entityAt(1, 1)).alive);

    sink.handle(TimedEvent{
        .tick = 1,
        .event = Event{.name = antwika::engine::events::kTick},
    });

    EXPECT_FALSE(world.get<Cell>(grid.entityAt(1, 1)).alive);
}

TEST(BoardSinkTest, ToggleCellPayloadMissingCommaThrows)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 2, 2);
    world.commit();

    SystemScheduler scheduler;
    BoardSink sink(world, grid, scheduler);

    EXPECT_THROW(
        sink.handle(TimedEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = "42",
            },
        }),
        BoardSinkError);
}

TEST(BoardSinkTest, ToggleCellPayloadWithNonNumericFieldThrows)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 2, 2);
    world.commit();

    SystemScheduler scheduler;
    BoardSink sink(world, grid, scheduler);

    EXPECT_THROW(
        sink.handle(TimedEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = "abc,def",
            },
        }),
        BoardSinkError);
}

TEST(BoardSinkTest, ToggleCellPayloadWithNegativeFieldThrows)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 2, 2);
    world.commit();

    SystemScheduler scheduler;
    BoardSink sink(world, grid, scheduler);

    EXPECT_THROW(
        sink.handle(TimedEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = "-1,2",
            },
        }),
        BoardSinkError);
}

TEST(BoardSinkTest, ToggleCellPayloadWithTrailingGarbageThrows)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 2, 2);
    world.commit();

    SystemScheduler scheduler;
    BoardSink sink(world, grid, scheduler);

    EXPECT_THROW(
        sink.handle(TimedEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = "1x,2",
            },
        }),
        BoardSinkError);
}

TEST(BoardSinkTest, IgnoresUnrelatedEvents)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 2, 2);
    world.commit();

    SystemScheduler scheduler;
    BoardSink sink(world, grid, scheduler);
    const auto before = antwika::life::readBoard(world, grid);

    sink.handle(TimedEvent{
        .tick = 0,
        .event = Event{.name = "some.other.event"},
    });

    EXPECT_EQ(antwika::life::readBoard(world, grid), before);
}
