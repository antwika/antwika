#include "antwika/ecs/SystemScheduler.hpp"

#include <cstdint>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/ecs/Entity.hpp"
#include "antwika/ecs/ISystem.hpp"
#include "antwika/ecs/World.hpp"

using antwika::ecs::Entity;
using antwika::ecs::ISystem;
using antwika::ecs::rawValue;
using antwika::ecs::SystemScheduler;
using antwika::ecs::World;
using antwika::log::mocks::MockLogger;
using antwika::time::Tick;
using ::testing::NiceMock;

namespace
{

    struct Position
    {
        int x{};
        int y{};

        bool operator==(const Position &) const = default;
    };

    struct Velocity
    {
        int dx{};
        int dy{};

        bool operator==(const Velocity &) const = default;
    };

    class MoveSystem final : public ISystem
    {
    public:
        void update(World &world, Tick) override
        {
            for (const auto entity : world.view<Position, Velocity>())
            {
                const auto position = world.get<Position>(entity);
                const auto velocity = world.get<Velocity>(entity);
                world.set<Position>(
                    entity,
                    Position{
                        position.x + velocity.dx,
                        position.y + velocity.dy});
            }
        }
    };

    std::vector<Position> runSimulation(std::uint64_t tickCount)
    {
        NiceMock<MockLogger> logger;
        World world(logger);

        std::vector<Entity> entities;
        for (int i = 0; i < 5; ++i)
        {
            const auto entity = world.create();
            world.add<Position>(entity, Position{i, 0});
            world.add<Velocity>(entity, Velocity{1, i % 3});
            entities.push_back(entity);
        }
        world.commit();

        SystemScheduler scheduler;
        MoveSystem mover;
        const auto phase = scheduler.createPhase("simulate");
        scheduler.addSystem(phase, mover);

        for (std::uint64_t tick = 0; tick < tickCount; ++tick)
        {
            scheduler.run(world, tick);
        }

        std::vector<Position> positions;
        for (const auto entity : entities)
        {
            positions.push_back(world.get<Position>(entity));
        }
        return positions;
    }

    std::vector<Entity> buildChurnedViewOrder()
    {
        NiceMock<MockLogger> logger;
        World world(logger);

        std::vector<Entity> created;
        for (int i = 0; i < 6; ++i)
        {
            const auto entity = world.create();
            world.add<Position>(entity, Position{i, 0});
            created.push_back(entity);
        }
        world.commit();

        world.destroy(created[1]);
        world.destroy(created[4]);
        const auto extra = world.create();
        world.add<Position>(extra, Position{99, 99});
        world.commit();

        std::vector<Entity> order;
        for (const auto entity : world.view<Position>())
        {
            order.push_back(entity);
        }
        return order;
    }

    std::vector<Entity> buildReusedHandles()
    {
        NiceMock<MockLogger> logger;
        World world(logger);

        std::vector<Entity> handed;
        for (int round = 0; round < 4; ++round)
        {
            const auto first = world.create();
            const auto second = world.create();
            world.add<Position>(first, Position{round, 0});
            world.add<Position>(second, Position{round, 1});
            world.commit();

            handed.push_back(first);
            handed.push_back(second);

            world.destroy(first);
            world.destroy(second);
            world.commit();
        }
        return handed;
    }

} // namespace

TEST(EcsDeterminismTest, RunningTheSameSimulationTwiceProducesIdenticalState)
{
    const auto first = runSimulation(20);
    const auto second = runSimulation(20);

    EXPECT_EQ(first, second);
}

TEST(EcsDeterminismTest,
     ViewIterationOrderIsIdenticalAcrossRunsWithTheSameChurn)
{
    EXPECT_EQ(buildChurnedViewOrder(), buildChurnedViewOrder());
}

TEST(EcsDeterminismTest, IndexReuseHandsOutTheSameValuesForTheSameChurn)
{
    // Reuse has to be a function of the create/destroy sequence.
    // A replay reproduces nothing else about which index was free.
    const auto first = buildReusedHandles();

    ASSERT_NE(rawValue(first.front()), rawValue(first.back()));
    EXPECT_EQ(first, buildReusedHandles());
}
