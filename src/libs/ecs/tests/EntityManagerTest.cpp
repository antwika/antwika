#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/log/mocks/MockLogger.hpp>

#include "EntityManager.hpp"
#include "antwika/ecs/EcsError.hpp"

using antwika::ecs::EcsError;
using antwika::ecs::Entity;
using antwika::ecs::kNullEntity;
using antwika::ecs::rawValue;
using antwika::ecs::detail::EntityManager;
using antwika::log::Level;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

TEST(EntityManagerTest, Create_ReturnsIncreasingValuesFromOne)
{
    NiceMock<MockLogger> logger;
    EntityManager manager(logger);

    const auto first = manager.create();
    const auto second = manager.create();

    EXPECT_EQ(rawValue(first), 1U);
    EXPECT_EQ(rawValue(second), 2U);
}

TEST(EntityManagerTest, Alive_IsTrueForANewEntity)
{
    NiceMock<MockLogger> logger;
    EntityManager manager(logger);

    const auto entity = manager.create();

    EXPECT_TRUE(manager.isAlive(entity));
}

TEST(EntityManagerTest, Destroy_LeavesTheIndexDeadAndUnreused)
{
    NiceMock<MockLogger> logger;
    EntityManager manager(logger);

    const auto first = manager.create();
    manager.destroy(first);
    const auto second = manager.create();

    EXPECT_FALSE(manager.isAlive(first));
    EXPECT_NE(rawValue(first), rawValue(second));
    EXPECT_EQ(rawValue(second), 2U);
}

TEST(EntityManagerTest, Destroy_ThrowsOnADeadEntity)
{
    NiceMock<MockLogger> logger;
    EntityManager manager(logger);

    const auto entity = manager.create();
    manager.destroy(entity);

    EXPECT_THROW(manager.destroy(entity), EcsError);
}

TEST(EntityManagerTest, Destroy_ThrowsOnAnUnknownEntity)
{
    NiceMock<MockLogger> logger;
    EntityManager manager(logger);

    EXPECT_THROW(manager.destroy(Entity{42}), EcsError);
}

TEST(EntityManagerTest, Alive_IsFalseForTheNullEntity)
{
    NiceMock<MockLogger> logger;
    EntityManager manager(logger);
    static_cast<void>(manager.create());

    EXPECT_FALSE(manager.isAlive(kNullEntity));
}

TEST(EntityManagerTest, Alive_IsFalseForTheValueJustPastTheLastCreated)
{
    NiceMock<MockLogger> logger;
    EntityManager manager(logger);
    const auto only = manager.create();

    EXPECT_FALSE(manager.isAlive(Entity{rawValue(only) + 1}));
}

TEST(EntityManagerTest, Create_LogsFatalAndThrowsWhenExhausted)
{
    MockLogger logger;
    EXPECT_CALL(
        logger,
        log(Level::Fatal, "EntityManager: entity index space exhausted"));

    EntityManager manager(logger, 1);
    static_cast<void>(manager.create());

    EXPECT_THROW(static_cast<void>(manager.create()), EcsError);
}

TEST(EntityManagerTest, Alive_StillAnswersAfterExhaustion)
{
    NiceMock<MockLogger> logger;
    EntityManager manager(logger, 1);
    const auto only = manager.create();

    EXPECT_THROW(static_cast<void>(manager.create()), EcsError);

    EXPECT_TRUE(manager.isAlive(only));
}
