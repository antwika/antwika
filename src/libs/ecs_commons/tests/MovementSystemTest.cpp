#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/ecs_commons/GridPosition.hpp"
#include "antwika/ecs_commons/MovementSystem.hpp"
#include "antwika/ecs_commons/Velocity.hpp"

using antwika::ecs::Entity;
using antwika::ecs::World;
using antwika::ecs_commons::GridPosition;
using antwika::ecs_commons::MovementSystem;
using antwika::ecs_commons::Velocity;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{
    Entity spawn(World &world, GridPosition at, Velocity velocity)
    {
        const auto entity = world.create();
        world.add<GridPosition>(entity, at);
        world.add<Velocity>(entity, velocity);
        return entity;
    }
} // namespace

TEST(MovementSystemTest, MovesAnEntityByItsVelocityEachTick)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity =
        spawn(world, GridPosition{.x = 0, .y = 0}, Velocity{.dx = 2, .dy = -1});
    world.commit();

    MovementSystem system;
    system.update(world, 0);
    world.commit();
    EXPECT_EQ(world.get<GridPosition>(entity), (GridPosition{.x = 2, .y = -1}));

    system.update(world, 1);
    world.commit();
    EXPECT_EQ(world.get<GridPosition>(entity), (GridPosition{.x = 4, .y = -2}));
}

TEST(MovementSystemTest, LeavesAnEntityWithoutAVelocityWhereItIs)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto still = world.create();
    world.add<GridPosition>(still, GridPosition{.x = 7, .y = 7});
    world.commit();

    MovementSystem system;
    system.update(world, 0);
    world.commit();

    EXPECT_EQ(world.get<GridPosition>(still), (GridPosition{.x = 7, .y = 7}));
}

TEST(MovementSystemTest, StagesEveryMoveSoNobodySeesAnotherEntityMidTick)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto left =
        spawn(world, GridPosition{.x = 0, .y = 0}, Velocity{.dx = 1, .dy = 0});
    const auto right =
        spawn(world, GridPosition{.x = 1, .y = 0}, Velocity{.dx = -1, .dy = 0});
    world.commit();

    MovementSystem system;
    system.update(world, 0);

    // Before the commit, both are still where they started.
    EXPECT_EQ(world.get<GridPosition>(left), (GridPosition{.x = 0, .y = 0}));
    EXPECT_EQ(world.get<GridPosition>(right), (GridPosition{.x = 1, .y = 0}));

    world.commit();

    // They passed through each other rather than one reacting first.
    EXPECT_EQ(world.get<GridPosition>(left), (GridPosition{.x = 1, .y = 0}));
    EXPECT_EQ(world.get<GridPosition>(right), (GridPosition{.x = 0, .y = 0}));
}

TEST(MovementSystemTest, VisitsEntitiesInTheSameOrderEveryRun)
{
    const auto order = []
    {
        NiceMock<MockLogger> logger;
        World world(logger);
        std::vector<Entity> spawned;
        for (int i = 0; i < 8; ++i)
        {
            spawned.push_back(
                spawn(
                    world,
                    GridPosition{.x = i, .y = 0},
                    Velocity{.dx = 1, .dy = 0}));
        }
        world.commit();

        MovementSystem system;
        system.update(world, 0);
        world.commit();

        std::vector<GridPosition> result;
        for (const auto entity : spawned)
        {
            result.push_back(world.get<GridPosition>(entity));
        }
        return result;
    };

    EXPECT_EQ(order(), order());
}
