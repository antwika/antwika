#include <gtest/gtest.h>

#include <cstdint>
#include <set>
#include <vector>

#include <antwika/component/Patrol.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/CharacterIndex.hpp>
#include <antwika/component/Velocity.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/collision/Collision.hpp>

#include "antwika/gameplay/GameLoop.hpp"
#include "antwika/system/PatrolSystem.hpp"

using antwika::ecs::OpenPhase;
using antwika::ecs::World;
using antwika::voxel::VoxelPosition;
using antwika::voxel::Voxels;
using antwika::ecs::Entity;
using antwika::gameplay::GameLoop;
using antwika::component::Patrol;
using antwika::system::PatrolSystem;
using antwika::gameplay::Phase;
using antwika::component::Position;
using antwika::component::CharacterIndex;
using antwika::component::Velocity;
using antwika::voxel::VoxelCell;
using antwika::voxel::voxelsOf;
using antwika::component::kStrollSpeedFactor;
using antwika::voxel::kVoxelSide;
using antwika::log::mocks::MockLogger;
using testing::NiceMock;

namespace
{

    constexpr float kTolerance = 1e-4F;

    [[nodiscard]] Voxels getFloorOver(
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

    struct PatrolHarness final
    {
        NiceMock<MockLogger> logger{};
        Voxels solidVoxels{};
        std::vector<std::vector<VoxelPosition>> stopPositions{};
        World world{logger};
        GameLoop gameLoop{world};
        antwika::system::SimulationState simulationState{};
        PatrolSystem system{solidVoxels, stopPositions, simulationState};
        Entity entity{};

        void begin(const Position stoodPosition)
        {
            const OpenPhase phase(gameLoop.getWorld());

            gameLoop.addSystem(Phase::Sending, system);
            entity = gameLoop.getWorld().create();
            gameLoop.getWorld().add<Position>(entity, stoodPosition);
            gameLoop.getWorld().add<Velocity>(entity, Velocity{});
            gameLoop.getWorld().add<Patrol>(entity, Patrol{});
            gameLoop.getWorld().add<CharacterIndex>(
                entity, CharacterIndex{.index = 0});
        }

        [[nodiscard]] Velocity getSentVelocity() const
        {
            return gameLoop.getWorld().get<Velocity>(entity);
        }
    };

    [[nodiscard]] VoxelPosition groundAt(
        const std::int32_t x, const std::int32_t z)
    {
        return VoxelPosition{.x = x, .y = 0, .z = z};
    }

}

TEST(PatrolTest, Update_LeavesACharacterWithNoStopsStandingStill)
{
    PatrolHarness harness;

    harness.solidVoxels = getFloorOver(2);
    harness.stopPositions = {{}};
    harness.begin(Position{.x = 0.0F, .y = 0.5F, .z = 0.0F});
    harness.gameLoop.run(0);

    EXPECT_NEAR(harness.getSentVelocity().velocityX, 0.0F, kTolerance);
    EXPECT_NEAR(harness.getSentVelocity().velocityZ, 0.0F, kTolerance);
}

TEST(PatrolTest, Update_SendsACharacterTowardItsFirstStop)
{
    PatrolHarness harness;

    harness.solidVoxels = getFloorOver(3);
    harness.stopPositions = {{groundAt(3, 0)}};
    harness.begin(Position{.x = 0.0F, .y = 0.5F, .z = 0.0F});
    harness.gameLoop.run(0);

    EXPECT_NEAR(harness.getSentVelocity().velocityX, 1.0F, kTolerance);
    EXPECT_NEAR(harness.getSentVelocity().velocityZ, 0.0F, kTolerance);
}

TEST(PatrolTest, Update_StrollsAtHalfAWalkersPace)
{
    PatrolHarness harness;

    harness.solidVoxels = getFloorOver(3);
    harness.stopPositions = {{groundAt(3, 0)}};
    harness.begin(Position{.x = 0.0F, .y = 0.5F, .z = 0.0F});
    harness.gameLoop.run(0);

    EXPECT_NEAR(
        harness.getSentVelocity().speedMultiplier,
        kStrollSpeedFactor,
        kTolerance);
}

TEST(PatrolTest, Update_TurnsForTheNextStopOnceItArrives)
{
    PatrolHarness harness;

    harness.solidVoxels = getFloorOver(3);
    harness.stopPositions = {{groundAt(1, 0), groundAt(-1, 0)}};
    harness.begin(Position{.x = 1.0F, .y = 0.5F, .z = 0.0F});
    harness.gameLoop.run(0);
    harness.gameLoop.run(1);

    EXPECT_LT(harness.getSentVelocity().velocityX, 0.0F);
}

TEST(PatrolTest, Update_WrapsBackToTheFirstStopAfterTheLast)
{
    PatrolHarness harness;

    harness.solidVoxels = getFloorOver(3);
    harness.stopPositions = {{groundAt(1, 0), groundAt(-1, 0)}};
    harness.begin(Position{.x = 1.0F, .y = 0.5F, .z = 0.0F});

    for (antwika::time::Tick tick = 0; tick < 4; ++tick)
    {
        harness.gameLoop.run(tick);
    }

    EXPECT_EQ(
        harness.gameLoop.getWorld()
            .get<Patrol>(harness.entity)
            .nextStopIndex,
        0U);
}

TEST(PatrolTest, Update_ReplansOnceItsRouteRunsOut)
{
    PatrolHarness harness;

    harness.solidVoxels = getFloorOver(3);
    harness.stopPositions = {{groundAt(1, 0)}};
    harness.begin(Position{.x = 1.0F, .y = 0.5F, .z = 0.0F});
    harness.gameLoop.run(0);

    const auto first = harness.gameLoop.getWorld()
                           .get<Patrol>(harness.entity)
                           .nextStopIndex;

    harness.gameLoop.run(1);

    EXPECT_EQ(
        harness.gameLoop.getWorld()
            .get<Patrol>(harness.entity)
            .nextStopIndex,
        first);
}

TEST(PatrolTest, Update_LeavesACharacterStoodWhereItStandsOnNothing)
{
    PatrolHarness harness;

    harness.solidVoxels = getFloorOver(1);
    harness.stopPositions = {{groundAt(0, 0)}};
    harness.begin(Position{.x = 40.0F, .y = 0.5F, .z = 40.0F});
    harness.gameLoop.run(0);

    EXPECT_NEAR(harness.getSentVelocity().velocityX, 0.0F, kTolerance);
    EXPECT_NEAR(harness.getSentVelocity().velocityZ, 0.0F, kTolerance);
}

TEST(PatrolTest, Update_LeavesACharacterStoodWhereItsStopStandsOnNothing)
{
    PatrolHarness harness;

    harness.solidVoxels = getFloorOver(1);
    harness.stopPositions = {{groundAt(40, 40)}};
    harness.begin(Position{.x = 0.0F, .y = 0.5F, .z = 0.0F});
    harness.gameLoop.run(0);

    EXPECT_NEAR(harness.getSentVelocity().velocityX, 0.0F, kTolerance);
    EXPECT_NEAR(harness.getSentVelocity().velocityZ, 0.0F, kTolerance);
}

TEST(PatrolTest, Update_LeavesACharacterStoodWhereNoWalkReachesItsStop)
{
    PatrolHarness harness;

    harness.solidVoxels = getFloorOver(1);
    harness.solidVoxels.merge(voxelsOf({VoxelCell{.position = {.x = 8, .y = 0,
        .z = 8}}}));
    harness.stopPositions = {{groundAt(8, 8)}};
    harness.begin(Position{.x = 0.0F, .y = 0.5F, .z = 0.0F});
    harness.gameLoop.run(0);

    EXPECT_NEAR(harness.getSentVelocity().velocityX, 0.0F, kTolerance);
    EXPECT_NEAR(harness.getSentVelocity().velocityZ, 0.0F, kTolerance);
}

TEST(PatrolTest, Update_HoldsACharacterStillWhileItSpeaks)
{
    PatrolHarness harness;

    harness.solidVoxels = getFloorOver(3);
    harness.stopPositions = {{groundAt(3, 0)}};
    harness.begin(Position{.x = 0.0F, .y = 0.5F, .z = 0.0F});
    harness.simulationState.speaking = 0U;
    harness.gameLoop.run(0);

    EXPECT_NEAR(harness.getSentVelocity().velocityX, 0.0F, kTolerance);
}

TEST(PatrolTest, Update_LetsACharacterStrollAgainOnceItIsDone)
{
    PatrolHarness harness;

    harness.solidVoxels = getFloorOver(3);
    harness.stopPositions = {{groundAt(3, 0)}};
    harness.begin(Position{.x = 0.0F, .y = 0.5F, .z = 0.0F});
    harness.simulationState.speaking = 0U;
    harness.gameLoop.run(0);
    harness.simulationState.speaking = std::nullopt;
    harness.gameLoop.run(1);

    EXPECT_NEAR(harness.getSentVelocity().velocityX, 1.0F, kTolerance);
}

TEST(PatrolTest, Update_HoldsEveryCharacterStillWhilePaused)
{
    PatrolHarness harness;

    harness.solidVoxels = getFloorOver(3);
    harness.stopPositions = {{groundAt(3, 0)}};
    harness.begin(Position{.x = 0.0F, .y = 0.5F, .z = 0.0F});
    harness.simulationState.simulationPaused = true;
    harness.gameLoop.run(0);

    EXPECT_NEAR(harness.getSentVelocity().velocityX, 0.0F, kTolerance);
}

TEST(PatrolTest, Update_LeavesACharacterWithNoRosterEntryStandingStill)
{
    PatrolHarness harness;

    harness.solidVoxels = getFloorOver(3);
    harness.stopPositions = {};
    harness.begin(Position{.x = 0.0F, .y = 0.5F, .z = 0.0F});
    harness.gameLoop.run(0);

    EXPECT_NEAR(harness.getSentVelocity().velocityX, 0.0F, kTolerance);
}

TEST(PatrolTest, Forget_LetsGoOfTheRoutesItPlanned)
{
    PatrolHarness harness;

    harness.solidVoxels = getFloorOver(3);
    harness.stopPositions = {{groundAt(3, 0)}};
    harness.begin(Position{.x = 0.0F, .y = 0.5F, .z = 0.0F});
    harness.gameLoop.run(0);
    harness.system.forget();
    harness.gameLoop.run(1);

    EXPECT_NEAR(harness.getSentVelocity().velocityX, 1.0F, kTolerance);
}

TEST(PatrolTest, Update_StrollsEveryCharacterOfTheRoster)
{
    NiceMock<MockLogger> logger;
    Voxels solidVoxels = getFloorOver(3);
    std::vector<std::vector<VoxelPosition>> stopPositions{
        {groundAt(3, 0)}, {groundAt(-3, 0)}};
    World world(logger);
    GameLoop gameLoop(world);
    antwika::system::SimulationState simulationState;
    PatrolSystem system(solidVoxels, stopPositions, simulationState);

    gameLoop.addSystem(Phase::Sending, system);

    std::vector<Entity> folkEntities;

    {
        const OpenPhase phase(gameLoop.getWorld());

        for (std::uint32_t index = 0; index < 2U; ++index)
        {
            const auto entity = gameLoop.getWorld().create();

            gameLoop.getWorld().add<Position>(
                entity, Position{.x = 0.0F, .y = 0.5F, .z = 0.0F});
            gameLoop.getWorld().add<Velocity>(entity, Velocity{});
            gameLoop.getWorld().add<Patrol>(entity, Patrol{});
            gameLoop.getWorld().add<CharacterIndex>(
                entity, CharacterIndex{.index = index});
            folkEntities.push_back(entity);
        }
    }

    gameLoop.run(0);

    EXPECT_NEAR(
        gameLoop.getWorld().get<Velocity>(folkEntities.at(0)).velocityX,
        1.0F,
        kTolerance);
    EXPECT_NEAR(
        gameLoop.getWorld().get<Velocity>(folkEntities.at(1)).velocityX,
        -1.0F,
        kTolerance);
}
