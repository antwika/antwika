#include "EntityManager.hpp"

#include <chrono>
#include <iostream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/log/Logger.hpp>
#include <antwika/log/MinimumLevelLogPolicy.hpp>
#include <antwika/log/PlainFormatter.hpp>
#include <antwika/log/StreamAppender.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/time/fakes/FakeClock.hpp>

#include "antwika/ecs/EcsError.hpp"

using antwika::ecs::EcsError;
using antwika::ecs::Entity;
using antwika::ecs::kNullEntity;
using antwika::ecs::rawValue;
using antwika::ecs::detail::EntityManager;
using antwika::log::Level;
using antwika::log::Logger;
using antwika::log::MinimumLevelLogPolicy;
using antwika::log::PlainFormatter;
using antwika::log::StreamAppender;
using antwika::log::mocks::MockLogger;
using antwika::time::fakes::FakeClock;
using ::testing::NiceMock;

namespace
{

    // Drives an EntityManager past its ceiling, logging via a real Logger.
    // The death test below checks the fatal message text captured this way.
    void createUntilExhausted()
    {
        PlainFormatter formatter;
        MinimumLevelLogPolicy policy(Level::Trace);
        FakeClock clock(std::chrono::system_clock::time_point{});
        StreamAppender appender(std::cerr);
        Logger logger(formatter, policy, clock, appender);

        EntityManager manager(logger, /*maxEntities=*/1);
        static_cast<void>(manager.create());
        static_cast<void>(manager.create());
    }

} // namespace

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

TEST(EntityManagerDeathTest, ExhaustingIndexSpaceLogsFatalAndTerminates)
{
    EXPECT_DEATH(createUntilExhausted(), "exhausted");
}
