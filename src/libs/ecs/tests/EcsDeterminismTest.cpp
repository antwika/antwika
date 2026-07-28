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
