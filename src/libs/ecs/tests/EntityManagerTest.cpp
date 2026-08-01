#include "EntityManager.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/ecs/EcsError.hpp"

using antwika::ecs::EcsError;
using antwika::ecs::Entity;
using antwika::ecs::kNullEntity;
using antwika::ecs::rawValue;
using antwika::ecs::detail::EntityManager;
using antwika::log::Level;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

TEST(EntityManagerTest, CreateReturnsIncreasingValuesStartingAtOne)
{
    NiceMock<MockLogger> logger;
    EntityManager manager(logger);

    const auto first = manager.create();
    const auto second = manager.create();

    EXPECT_EQ(rawValue(first), 1U);
    EXPECT_EQ(rawValue(second), 2U);
}

TEST(EntityManagerTest, NewEntityIsAlive)
{
    NiceMock<MockLogger> logger;
    EntityManager manager(logger);

    const auto entity = manager.create();

    EXPECT_TRUE(manager.alive(entity));
}

TEST(EntityManagerTest, DestroyedEntityIsNotAliveAndIndexNeverReused)
{
    NiceMock<MockLogger> logger;
    EntityManager manager(logger);

    const auto first = manager.create();
    manager.destroy(first);
    const auto second = manager.create();

    EXPECT_FALSE(manager.alive(first));
    EXPECT_NE(rawValue(first), rawValue(second));
    EXPECT_EQ(rawValue(second), 2U);
}

TEST(EntityManagerTest, DestroyingADeadEntityThrows)
{
    NiceMock<MockLogger> logger;
    EntityManager manager(logger);

    const auto entity = manager.create();
    manager.destroy(entity);

    EXPECT_THROW(manager.destroy(entity), EcsError);
}

TEST(EntityManagerTest, DestroyingAnUnknownEntityThrows)
{
    NiceMock<MockLogger> logger;
    EntityManager manager(logger);

    EXPECT_THROW(manager.destroy(Entity{42}), EcsError);
}

TEST(EntityManagerTest, KNullEntityIsNeverAlive)
{
    NiceMock<MockLogger> logger;
    EntityManager manager(logger);
    static_cast<void>(manager.create());

    EXPECT_FALSE(manager.alive(kNullEntity));
}

TEST(EntityManagerTest, ExhaustingIndexSpaceLogsFatalAndThrows)
{
    MockLogger logger;
    EXPECT_CALL(
        logger,
        log(Level::Fatal, "EntityManager: entity index space exhausted"));

    EntityManager manager(logger, /*maxEntities=*/1);
    static_cast<void>(manager.create());

    EXPECT_THROW(static_cast<void>(manager.create()), EcsError);
}

TEST(EntityManagerTest, ExhaustionLeavesTheManagerUsableForQueries)
{
    NiceMock<MockLogger> logger;
    EntityManager manager(logger, /*maxEntities=*/1);
    const auto only = manager.create();

    EXPECT_THROW(static_cast<void>(manager.create()), EcsError);

    // Throwing rather than exiting leaves the caller a live object.
    EXPECT_TRUE(manager.alive(only));
}
