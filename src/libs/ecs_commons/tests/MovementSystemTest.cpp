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

// A mover that leaves the world takes its components with it.
TEST(MovementSystemTest, DestroyingAMoverTakesItsComponentsWithIt)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto leaving =
        spawn(world, GridPosition{.x = 0, .y = 0}, Velocity{.dx = 1, .dy = 0});
    const auto staying =
        spawn(world, GridPosition{.x = 5, .y = 5}, Velocity{.dx = 0, .dy = 0});
    world.commit();

    world.destroy(leaving);
    world.commit();

    EXPECT_FALSE(world.alive(leaving));
    EXPECT_FALSE(world.has<GridPosition>(leaving));
    EXPECT_FALSE(world.has<Velocity>(leaving));

    EXPECT_TRUE(world.has<GridPosition>(staying));
    EXPECT_TRUE(world.has<Velocity>(staying));
}

// World visits every pool on a destroy, not just the ones in play.
// So a pool the leaving entity was never in has to be left alone.
// A scenery cell has a position and no velocity, which is that case.
TEST(MovementSystemTest, DestroyingAnEntityLeavesPoolsItWasNeverIn)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto mover =
        spawn(world, GridPosition{.x = 0, .y = 0}, Velocity{.dx = 1, .dy = 0});
    const auto scenery = world.create();
    world.add<GridPosition>(scenery, GridPosition{.x = 9, .y = 9});
    world.commit();

    ASSERT_FALSE(world.has<Velocity>(scenery));

    world.destroy(scenery);
    world.commit();

    EXPECT_FALSE(world.alive(scenery));
    EXPECT_FALSE(world.has<GridPosition>(scenery));

    // The velocity pool it was never in still holds the mover's.
    EXPECT_EQ(world.get<Velocity>(mover), (Velocity{.dx = 1, .dy = 0}));

    MovementSystem system;
    system.update(world, 0);
    world.commit();
    EXPECT_EQ(world.get<GridPosition>(mover), (GridPosition{.x = 1, .y = 0}));
}

// Taking the velocity off a mover parks it where it stands.
// That is the cheapest way to stop something without destroying it.
TEST(MovementSystemTest, RemovingAVelocityParksTheMoverWhereItIs)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity =
        spawn(world, GridPosition{.x = 3, .y = 4}, Velocity{.dx = 2, .dy = 2});
    world.commit();

    world.remove<Velocity>(entity);
    world.commit();

    EXPECT_FALSE(world.has<Velocity>(entity));
    EXPECT_TRUE(world.has<GridPosition>(entity));

    MovementSystem system;
    system.update(world, 0);
    world.commit();
    EXPECT_EQ(world.get<GridPosition>(entity), (GridPosition{.x = 3, .y = 4}));
}

// destroy() and add() both stage, and both run in the order staged.
// So an add() staged after a destroy() finds the entity already gone.
// It has to check again rather than trust its own earlier check.
// Otherwise it leaves a component with no entity to own it.
TEST(MovementSystemTest, AddingAfterADestroyInTheSameFrameInsertsNothing)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto mover =
        spawn(world, GridPosition{.x = 0, .y = 0}, Velocity{.dx = 1, .dy = 0});
    const auto leaving = world.create();
    world.add<GridPosition>(leaving, GridPosition{.x = 2, .y = 2});
    world.commit();

    world.destroy(leaving);
    world.add<Velocity>(leaving, Velocity{.dx = 4, .dy = 4});
    world.add<GridPosition>(leaving, GridPosition{.x = 8, .y = 8});
    world.commit();

    EXPECT_FALSE(world.alive(leaving));
    EXPECT_FALSE(world.has<GridPosition>(leaving));
    EXPECT_FALSE(world.has<Velocity>(leaving));

    // And nothing of the mover's was disturbed on the way past.
    EXPECT_EQ(world.get<Velocity>(mover), (Velocity{.dx = 1, .dy = 0}));
}
