#include <gtest/gtest.h>

#include <antwika/component/Player.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/Velocity.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/collision/Collision.hpp>

#include "antwika/system/MoveIntentSystem.hpp"
#include "antwika/system/WalkSystem.hpp"
#include "antwika/gameplay/GameLoop.hpp"

using antwika::ecs::OpenPhase;
using antwika::ecs::World;
using antwika::voxel::Voxels;
using antwika::gameplay::Phase;
using antwika::collision::kWalkSpeed;
using antwika::voxel::VoxelCell;
using antwika::voxel::voxelsOf;
using antwika::component::Player;
using antwika::system::MoveIntentSystem;
using antwika::gameplay::GameLoop;
using antwika::component::Position;
using antwika::component::Velocity;
using antwika::system::WalkSystem;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{

    constexpr float kTolerance = 0.0001F;

    [[nodiscard]] Voxels getFilledOver(
        const std::int32_t reach)
    {
        Voxels voxels;

        for (auto x = -reach; x <= reach; ++x)
        {
            for (auto z = -reach; z <= reach; ++z)
            {
                voxels.merge(voxelsOf({VoxelCell{.position = {.x = x, .y = 0,
                    .z = z}}}));
            }
        }

        return voxels;
    }

}

TEST(WalkerTest, Update_SendsAPlayerTheWayThatIsHeld)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    GameLoop gameLoop(world);
    antwika::component::DirectionKeys wasdKeys;
    antwika::component::DirectionKeys arrowKeys;
    antwika::system::SimulationState simulationState;
    MoveIntentSystem sendingSystem(wasdKeys, arrowKeys, simulationState);

    gameLoop.addSystem(Phase::Sending, sendingSystem);

    const auto entity = gameLoop.getWorld().create();

    {
        const OpenPhase phase(gameLoop.getWorld());

        gameLoop.getWorld().add<Player>(entity, Player{});
        gameLoop.getWorld().add<Velocity>(entity, Velocity{});
    }

    wasdKeys.east = true;
    wasdKeys.north = true;
    gameLoop.run(0);

    const auto walkVelocity = gameLoop.getWorld().get<Velocity>(entity);

    EXPECT_NEAR(walkVelocity.velocityX, 1.0F, kTolerance);
    EXPECT_NEAR(walkVelocity.velocityZ, -1.0F, kTolerance);
}

TEST(WalkerTest, Update_LeavesACharacterWithNoPlayerTagUnsent)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    GameLoop gameLoop(world);
    antwika::component::DirectionKeys wasdKeys;
    antwika::component::DirectionKeys arrowKeys;
    antwika::system::SimulationState simulationState;
    MoveIntentSystem sendingSystem(wasdKeys, arrowKeys, simulationState);

    gameLoop.addSystem(Phase::Sending, sendingSystem);

    const auto entity = gameLoop.getWorld().create();

    {
        const OpenPhase phase(gameLoop.getWorld());

        gameLoop.getWorld().add<Velocity>(entity, Velocity{});
    }

    wasdKeys.east = true;
    gameLoop.run(0);

    EXPECT_NEAR(
        gameLoop.getWorld().get<Velocity>(entity).velocityX,
        0.0F,
        kTolerance);
}

TEST(WalkerTest, Update_WalksTwoWalkersOverThePileAtOnce)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    GameLoop gameLoop(world);
    auto filledVoxels = getFilledOver(2);
    WalkSystem steppingSystem(filledVoxels);

    gameLoop.addSystem(Phase::Walking, steppingSystem);

    const auto east = gameLoop.getWorld().create();
    const auto west = gameLoop.getWorld().create();

    {
        const OpenPhase phase(gameLoop.getWorld());

        gameLoop.getWorld().add<Position>(
            east, Position{.x = 0.5F, .y = 1.0F, .z = 0.5F});
        gameLoop.getWorld().add<Velocity>(
            east, Velocity{.velocityX = 1.0F});
        gameLoop.getWorld().add<Position>(
            west, Position{.x = 0.5F, .y = 1.0F, .z = 0.5F});
        gameLoop.getWorld().add<Velocity>(
            west, Velocity{.velocityX = -1.0F});
    }

    gameLoop.run(0);

    EXPECT_NEAR(
        gameLoop.getWorld().get<Position>(east).x,
        0.5F + kWalkSpeed,
        kTolerance);
    EXPECT_NEAR(
        gameLoop.getWorld().get<Position>(west).x,
        0.5F - kWalkSpeed,
        kTolerance);
}

TEST(WalkerTest, Update_WalksEveryWalkerOverThePileAsItStands)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    GameLoop gameLoop(world);
    auto filledVoxels = getFilledOver(2);
    WalkSystem steppingSystem(filledVoxels);

    gameLoop.addSystem(Phase::Walking, steppingSystem);

    const auto entity = gameLoop.getWorld().create();

    {
        const OpenPhase phase(gameLoop.getWorld());

        gameLoop.getWorld().add<Position>(
            entity, Position{.x = 0.5F, .y = 1.0F, .z = 0.5F});
        gameLoop.getWorld().add<Velocity>(
            entity, Velocity{.velocityX = 1.0F, .velocityZ = 0.0F});
    }

    gameLoop.run(0);

    EXPECT_NEAR(
        gameLoop.getWorld().get<Position>(entity).x,
        0.5F + kWalkSpeed,
        kTolerance);
}
