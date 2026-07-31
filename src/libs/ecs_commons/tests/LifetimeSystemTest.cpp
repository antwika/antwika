#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/ecs_commons/Lifetime.hpp"
#include "antwika/ecs_commons/LifetimeSystem.hpp"

using antwika::ecs::Entity;
using antwika::ecs::World;
using antwika::ecs_commons::Lifetime;
using antwika::ecs_commons::LifetimeSystem;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{
    Entity spawn(World &world, antwika::time::Tick remaining)
    {
        const auto entity = world.create();
        world.add<Lifetime>(entity, Lifetime{.remaining = remaining});
        return entity;
    }
} // namespace

TEST(LifetimeSystemTest, CountsDownByOneTickPerUpdate)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = spawn(world, 3);
    world.commit();

    LifetimeSystem system;
    system.update(world, 0);
    world.commit();

    EXPECT_TRUE(world.alive(entity));
    EXPECT_EQ(world.get<Lifetime>(entity), (Lifetime{.remaining = 2}));
}

TEST(LifetimeSystemTest, SurvivesExactlyAsManyTicksAsItHasLeft)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = spawn(world, 3);
    world.commit();

    LifetimeSystem system;
    for (antwika::time::Tick tick = 0; tick < 2; ++tick)
    {
        system.update(world, tick);
        world.commit();
        EXPECT_TRUE(world.alive(entity));
    }

    system.update(world, 2);
    world.commit();

    EXPECT_FALSE(world.alive(entity));
}

TEST(LifetimeSystemTest, ExpiresOnTheFirstUpdateWithNothingLeft)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto zero = spawn(world, 0);
    const auto one = spawn(world, 1);
    world.commit();

    LifetimeSystem system;
    system.update(world, 0);
    world.commit();

    EXPECT_FALSE(world.alive(zero));
    EXPECT_FALSE(world.alive(one));
}

TEST(LifetimeSystemTest, StagesTheDestructionSoTheLastTickIsStillReadable)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = spawn(world, 1);
    world.commit();

    LifetimeSystem system;
    system.update(world, 0);

    EXPECT_TRUE(world.alive(entity));
    EXPECT_EQ(world.get<Lifetime>(entity), (Lifetime{.remaining = 1}));

    world.commit();

    EXPECT_FALSE(world.alive(entity));
}

TEST(LifetimeSystemTest, LeavesEntitiesWithoutALifetimeAlone)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto immortal = world.create();
    world.commit();

    LifetimeSystem system;
    system.update(world, 0);
    world.commit();

    EXPECT_TRUE(world.alive(immortal));
}
