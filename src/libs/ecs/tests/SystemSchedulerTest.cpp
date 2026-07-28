#include "antwika/ecs/SystemScheduler.hpp"

#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/ecs/mocks/MockSystem.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/ecs/EcsError.hpp"

using antwika::ecs::EcsError;
using antwika::ecs::Entity;
using antwika::ecs::ISystem;
using antwika::ecs::SystemScheduler;
using antwika::ecs::World;
using antwika::ecs::mocks::MockSystem;
using antwika::log::mocks::MockLogger;
using antwika::time::Tick;
using ::testing::NiceMock;

namespace
{

    struct Position
    {
        int x{};

        bool operator==(const Position &) const = default;
    };

    class RecordingSystem final : public ISystem
    {
    public:
        RecordingSystem(std::vector<std::string> &log, std::string name)
            : log(log), name(std::move(name))
        {
        }

        void update(World &, Tick) override
        {
            log.push_back(name);
        }

    private:
        std::vector<std::string> &log;
        std::string name;
    };

    class SetPositionSystem final : public ISystem
    {
    public:
        SetPositionSystem(Entity entity, int x) : entity(entity), x(x)
        {
        }

        void update(World &world, Tick) override
        {
            world.set<Position>(entity, Position{x});
        }

    private:
        Entity entity;
        int x;
    };

    class RecordPositionSystem final : public ISystem
    {
    public:
        RecordPositionSystem(Entity entity, std::vector<int> &observed)
            : entity(entity), observed(observed)
        {
        }

        void update(World &world, Tick) override
        {
            observed.push_back(world.get<Position>(entity).x);
        }

    private:
        Entity entity;
        std::vector<int> &observed;
    };

} // namespace

TEST(SystemSchedulerTest, CreatePhaseReturnsSequentialIds)
{
    SystemScheduler scheduler;

    EXPECT_EQ(scheduler.createPhase("a"), 0U);
    EXPECT_EQ(scheduler.createPhase("b"), 1U);
}

TEST(SystemSchedulerTest, AddingASystemToAnUnknownPhaseThrows)
{
    SystemScheduler scheduler;
    NiceMock<MockSystem> system;

    EXPECT_THROW(scheduler.addSystem(0, system), EcsError);
}

TEST(SystemSchedulerTest, SystemsRunInPhaseCreationThenRegistrationOrder)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    SystemScheduler scheduler;
    std::vector<std::string> log;
    RecordingSystem a(log, "a");
    RecordingSystem b(log, "b");
    RecordingSystem c(log, "c");

    const auto phaseOne = scheduler.createPhase("first");
    const auto phaseTwo = scheduler.createPhase("second");
    // Registered out of phase order, to prove run order tracks phase
    // creation order, not the order addSystem() happened to be called.
    scheduler.addSystem(phaseTwo, b);
    scheduler.addSystem(phaseOne, a);
    scheduler.addSystem(phaseTwo, c);

    scheduler.run(world, 0);

    EXPECT_EQ(log, (std::vector<std::string>{"a", "b", "c"}));
}

TEST(SystemSchedulerTest, SystemsInTheSamePhaseNeverObserveASiblingsWrite)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.add<Position>(entity, Position{0});
    world.commit();

    SystemScheduler scheduler;
    std::vector<int> observed;
    SetPositionSystem setter(entity, 42);
    RecordPositionSystem reader(entity, observed);
    const auto phase = scheduler.createPhase("phase");
    scheduler.addSystem(phase, setter);
    scheduler.addSystem(phase, reader);

    scheduler.run(world, 0);

    EXPECT_EQ(observed, (std::vector<int>{0}));
    EXPECT_EQ(world.get<Position>(entity), (Position{42}));
}

TEST(SystemSchedulerTest, ALaterPhaseObservesAnEarlierPhasesWrites)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    world.add<Position>(entity, Position{0});
    world.commit();

    SystemScheduler scheduler;
    std::vector<int> observed;
    SetPositionSystem setter(entity, 42);
    RecordPositionSystem reader(entity, observed);
    const auto phaseOne = scheduler.createPhase("one");
    const auto phaseTwo = scheduler.createPhase("two");
    scheduler.addSystem(phaseOne, setter);
    scheduler.addSystem(phaseTwo, reader);

    scheduler.run(world, 0);

    EXPECT_EQ(observed, (std::vector<int>{42}));
}
