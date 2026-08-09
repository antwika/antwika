#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <string_view>

#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/life/BoardSink.hpp"
#include "antwika/life/Board.hpp"
#include "antwika/life/BoardSinkError.hpp"
#include "antwika/life/Cell.hpp"
#include "antwika/life/Events.hpp"
#include "antwika/life/Grid.hpp"
#include "antwika/life/LifeSystem.hpp"

using antwika::ecs::SystemScheduler;
using antwika::ecs::World;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::life::BoardSink;
using antwika::life::BoardSinkError;
using antwika::life::Cell;
using antwika::life::Grid;
using antwika::life::LifeSystem;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{
    void registerLifeSystem(SystemScheduler &scheduler, LifeSystem &system)
    {
        const auto phase = scheduler.createPhase("life");
        scheduler.addSystem(phase, system);
    }
}

TEST(BoardSinkTest, Handle_StagesABlockThatSurvivesATick)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 4, 4);
    world.commit();

    SystemScheduler scheduler;
    LifeSystem system(grid);
    registerLifeSystem(scheduler, system);
    BoardSink sink(world, grid, scheduler);

    for (const std::string_view payload :
         {R"({"x":1,"y":1})",
          R"({"x":2,"y":1})",
          R"({"x":1,"y":2})",
          R"({"x":2,"y":2})"})
    {
        sink.handle(TickEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = std::string(payload),
            },
        });
    }

    EXPECT_FALSE(world.get<Cell>(grid.entityAt(1, 1)).alive);

    sink.handle(TickEvent{
        .tick = 0,
        .event = Event{.name = antwika::engine::events::kTick},
    });

    EXPECT_TRUE(world.get<Cell>(grid.entityAt(1, 1)).alive);
    EXPECT_TRUE(world.get<Cell>(grid.entityAt(2, 1)).alive);
    EXPECT_TRUE(world.get<Cell>(grid.entityAt(1, 2)).alive);
    EXPECT_TRUE(world.get<Cell>(grid.entityAt(2, 2)).alive);
}

TEST(BoardSinkTest, Handle_RunsLifeAndLetsAnIsolatedCellDie)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 3, 3);
    world.commit();

    SystemScheduler scheduler;
    LifeSystem system(grid);
    registerLifeSystem(scheduler, system);
    BoardSink sink(world, grid, scheduler);

    sink.handle(TickEvent{
        .tick = 0,
        .event = Event{
            .name = antwika::life::events::kToggleCell,
            .payload = R"({"x":1,"y":1})",
        },
    });
    sink.handle(TickEvent{
        .tick = 0,
        .event = Event{.name = antwika::engine::events::kTick},
    });

    EXPECT_FALSE(world.get<Cell>(grid.entityAt(1, 1)).alive);

    sink.handle(TickEvent{
        .tick = 1,
        .event = Event{.name = antwika::engine::events::kTick},
    });

    EXPECT_FALSE(world.get<Cell>(grid.entityAt(1, 1)).alive);
}

TEST(BoardSinkTest, Handle_ThrowsOnAPayloadThatIsNotJson)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 2, 2);
    world.commit();

    SystemScheduler scheduler;
    BoardSink sink(world, grid, scheduler);

    EXPECT_THROW(
        sink.handle(TickEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = "not json",
            },
        }),
        BoardSinkError);
}

TEST(BoardSinkTest, Handle_ThrowsOnAPayloadMissingX)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 2, 2);
    world.commit();

    SystemScheduler scheduler;
    BoardSink sink(world, grid, scheduler);

    EXPECT_THROW(
        sink.handle(TickEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = R"({"y":2})",
            },
        }),
        BoardSinkError);
}

TEST(BoardSinkTest, Handle_ThrowsOnAPayloadMissingY)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 2, 2);
    world.commit();

    SystemScheduler scheduler;
    BoardSink sink(world, grid, scheduler);

    EXPECT_THROW(
        sink.handle(TickEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = R"({"x":1})",
            },
        }),
        BoardSinkError);
}

TEST(BoardSinkTest, Handle_ThrowsOnANonNumericField)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 2, 2);
    world.commit();

    SystemScheduler scheduler;
    BoardSink sink(world, grid, scheduler);

    EXPECT_THROW(
        sink.handle(TickEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = R"({"x":"abc","y":2})",
            },
        }),
        BoardSinkError);
}

TEST(BoardSinkTest, Handle_ThrowsOnANegativeField)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 2, 2);
    world.commit();

    SystemScheduler scheduler;
    BoardSink sink(world, grid, scheduler);

    EXPECT_THROW(
        sink.handle(TickEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = R"({"x":-1,"y":2})",
            },
        }),
        BoardSinkError);
}

TEST(BoardSinkTest, Handle_ThrowsOnAPayloadOffTheBoard)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 2, 2);
    world.commit();

    SystemScheduler scheduler;
    BoardSink sink(world, grid, scheduler);

    EXPECT_THROW(
        sink.handle(TickEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = R"({"x":1000,"y":0})",
            },
        }),
        BoardSinkError);

    EXPECT_THROW(
        sink.handle(TickEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = R"({"x":0,"y":1000})",
            },
        }),
        BoardSinkError);
}

TEST(BoardSinkTest, Handle_ThrowsOnAnXPastUint32)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 2, 2);
    world.commit();

    SystemScheduler scheduler;
    BoardSink sink(world, grid, scheduler);

    EXPECT_THROW(
        sink.handle(TickEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::life::events::kToggleCell,
                .payload = R"({"x":4294967296,"y":2})",
            },
        }),
        BoardSinkError);
}

TEST(BoardSinkTest, Handle_IgnoresUnrelatedEvents)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, 2, 2);
    world.commit();

    SystemScheduler scheduler;
    BoardSink sink(world, grid, scheduler);
    const auto before = antwika::life::readBoard(world, grid);

    sink.handle(TickEvent{
        .tick = 0,
        .event = Event{.name = "some.other.event"},
    });

    EXPECT_EQ(antwika::life::readBoard(world, grid), before);
}
