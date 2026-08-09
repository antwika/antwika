#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include <antwika/ecs/fakes/FakeRecordPositionSystem.hpp>
#include <antwika/ecs/fakes/FakeSetPositionSystem.hpp>
#include <antwika/ecs/mocks/MockSystem.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/ecs/SystemScheduler.hpp"
#include "antwika/ecs/EcsError.hpp"

using antwika::ecs::EcsError;
using antwika::ecs::Entity;
using antwika::ecs::ISystem;
using antwika::ecs::SystemScheduler;
using antwika::ecs::World;
using antwika::ecs::fakes::FakeRecordPositionSystem;
using antwika::ecs::fakes::FakeSetPositionSystem;
using antwika::ecs::mocks::MockSystem;
using antwika::log::mocks::MockLogger;
using antwika::time::Tick;
using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;

namespace
{

    struct Position final
    {
        int x{};

        bool operator==(const Position &) const = default;
    };

}

TEST(SystemSchedulerTest, CreatePhase_ReturnsSequentialIds)
{
    SystemScheduler scheduler;

    EXPECT_EQ(scheduler.createPhase("a"), 0U);
    EXPECT_EQ(scheduler.createPhase("b"), 1U);
}

TEST(SystemSchedulerTest, AddSystem_ThrowsOnAnUnknownPhase)
{
    SystemScheduler scheduler;
    NiceMock<MockSystem> system;

    EXPECT_THROW(scheduler.addSystem(0, system), EcsError);
}

TEST(SystemSchedulerTest, Run_TakesPhaseThenRegistrationOrder)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    SystemScheduler scheduler;
    NiceMock<MockSystem> a;
    NiceMock<MockSystem> b;
    NiceMock<MockSystem> c;

    {
        InSequence order;

        EXPECT_CALL(a, update(_, _));
        EXPECT_CALL(b, update(_, _));
        EXPECT_CALL(c, update(_, _));
    }

    const auto phaseOne = scheduler.createPhase("first");
    const auto phaseTwo = scheduler.createPhase("second");
    scheduler.addSystem(phaseTwo, b);
    scheduler.addSystem(phaseOne, a);
    scheduler.addSystem(phaseTwo, c);

    scheduler.run(world, 0);
}

TEST(SystemSchedulerTest, Run_HandsEverySystemTheTickItWasCalledWith)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    SystemScheduler scheduler;
    NiceMock<MockSystem> first;
    NiceMock<MockSystem> second;

    EXPECT_CALL(first, update(_, Tick{7}));
    EXPECT_CALL(second, update(_, Tick{7}));

    const auto phaseOne = scheduler.createPhase("one");
    const auto phaseTwo = scheduler.createPhase("two");
    scheduler.addSystem(phaseOne, first);
    scheduler.addSystem(phaseTwo, second);

    scheduler.run(world, 7);
}

TEST(SystemSchedulerTest, Run_HidesASiblingsWriteInTheSamePhase)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.add<Position>(entity, Position{0});
    world.commit();

    SystemScheduler scheduler;
    std::vector<int> observed;
    FakeSetPositionSystem<Position> setter(entity, 42);
    FakeRecordPositionSystem<Position> reader(entity, observed);
    const auto phase = scheduler.createPhase("phase");
    scheduler.addSystem(phase, setter);
    scheduler.addSystem(phase, reader);

    scheduler.run(world, 0);

    EXPECT_EQ(observed, (std::vector<int>{0}));
    EXPECT_EQ(world.get<Position>(entity), (Position{42}));
}

TEST(SystemSchedulerTest, Run_ShowsAnEarlierPhasesWrites)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.add<Position>(entity, Position{0});
    world.commit();

    SystemScheduler scheduler;
    std::vector<int> observed;
    FakeSetPositionSystem<Position> setter(entity, 42);
    FakeRecordPositionSystem<Position> reader(entity, observed);
    const auto phaseOne = scheduler.createPhase("one");
    const auto phaseTwo = scheduler.createPhase("two");
    scheduler.addSystem(phaseOne, setter);
    scheduler.addSystem(phaseTwo, reader);

    scheduler.run(world, 0);

    EXPECT_EQ(observed, (std::vector<int>{42}));
}

TEST(SystemSchedulerTest, Run_LeavesAnEntityDestroyable)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.add<Position>(entity, Position{0});
    world.commit();

    SystemScheduler scheduler;
    FakeSetPositionSystem<Position> setter(entity, 1);
    const auto phase = scheduler.createPhase("phase");
    scheduler.addSystem(phase, setter);
    scheduler.run(world, 0);

    world.destroy(entity);
    world.commit();

    EXPECT_FALSE(world.alive(entity));
}
