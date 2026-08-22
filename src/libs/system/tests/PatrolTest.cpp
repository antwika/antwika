#include <gtest/gtest.h>

#include <cstdint>
#include <set>
#include <vector>

#include <antwika/component/Patrol.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/RosterIndex.hpp>
#include <antwika/component/Velocity.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/collision/Collision.hpp>

#include "antwika/gameplay/GameLoop.hpp"
#include "antwika/system/PatrolSystem.hpp"

using antwika::ecs::World;
using antwika::voxel::VoxelPosition;
using antwika::voxel::Voxels;
using antwika::ecs::Entity;
using antwika::gameplay::GameLoop;
using antwika::component::Patrol;
using antwika::system::PatrolSystem;
using antwika::gameplay::Phase;
using antwika::component::Position;
using antwika::component::RosterIndex;
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

    [[nodiscard]] Voxels floorOver(
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
        PatrolSystem system{solidVoxels, stopPositions};
        Entity entity{};

        void begin(const Position stoodPosition)
        {
            gameLoop.addSystem(Phase::Sending, system);
            entity = gameLoop.world().create();
            gameLoop.world().add<Position>(entity, stoodPosition);
            gameLoop.world().add<Velocity>(entity, Velocity{});
            gameLoop.world().add<Patrol>(entity, Patrol{});
            gameLoop.world().add<RosterIndex>(
                entity, RosterIndex{.index = 0});
            gameLoop.world().commit();
        }

        [[nodiscard]] Velocity sent() const
        {
            return gameLoop.world().get<Velocity>(entity);
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

    harness.solidVoxels = floorOver(2);
    harness.stopPositions = {{}};
    harness.begin(Position{.x = 0.0F, .y = 0.5F, .z = 0.0F});
    harness.gameLoop.run(0);

    EXPECT_NEAR(harness.sent().velocityX, 0.0F, kTolerance);
    EXPECT_NEAR(harness.sent().velocityZ, 0.0F, kTolerance);
}

TEST(PatrolTest, Update_SendsACharacterTowardItsFirstStop)
{
    PatrolHarness harness;

    harness.solidVoxels = floorOver(3);
    harness.stopPositions = {{groundAt(3, 0)}};
    harness.begin(Position{.x = 0.0F, .y = 0.5F, .z = 0.0F});
    harness.gameLoop.run(0);

    EXPECT_NEAR(harness.sent().velocityX, 1.0F, kTolerance);
    EXPECT_NEAR(harness.sent().velocityZ, 0.0F, kTolerance);
}

TEST(PatrolTest, Update_StrollsAtHalfAWalkersPace)
{
    PatrolHarness harness;

    harness.solidVoxels = floorOver(3);
    harness.stopPositions = {{groundAt(3, 0)}};
    harness.begin(Position{.x = 0.0F, .y = 0.5F, .z = 0.0F});
    harness.gameLoop.run(0);

    EXPECT_NEAR(
        harness.sent().speedMultiplier,
        kStrollSpeedFactor,
        kTolerance);
}

TEST(PatrolTest, Update_TurnsForTheNextStopOnceItArrives)
{
    PatrolHarness harness;

    harness.solidVoxels = floorOver(3);
    harness.stopPositions = {{groundAt(1, 0), groundAt(-1, 0)}};
    harness.begin(Position{.x = 1.0F, .y = 0.5F, .z = 0.0F});
    harness.gameLoop.run(0);
    harness.gameLoop.run(1);

    EXPECT_LT(harness.sent().velocityX, 0.0F);
}

TEST(PatrolTest, Update_WrapsBackToTheFirstStopAfterTheLast)
{
    PatrolHarness harness;

    harness.solidVoxels = floorOver(3);
    harness.stopPositions = {{groundAt(1, 0), groundAt(-1, 0)}};
    harness.begin(Position{.x = 1.0F, .y = 0.5F, .z = 0.0F});

    for (antwika::time::Tick tick = 0; tick < 4; ++tick)
    {
        harness.gameLoop.run(tick);
    }

    EXPECT_EQ(
        harness.gameLoop.world()
            .get<Patrol>(harness.entity)
            .nextStopIndex,
        0U);
}

TEST(PatrolTest, Update_ReplansOnceItsRouteRunsOut)
{
    PatrolHarness harness;

    harness.solidVoxels = floorOver(3);
    harness.stopPositions = {{groundAt(1, 0)}};
    harness.begin(Position{.x = 1.0F, .y = 0.5F, .z = 0.0F});
    harness.gameLoop.run(0);

    const auto first = harness.gameLoop.world()
                           .get<Patrol>(harness.entity)
                           .nextStopIndex;

    harness.gameLoop.run(1);

    EXPECT_EQ(
        harness.gameLoop.world()
            .get<Patrol>(harness.entity)
            .nextStopIndex,
        first);
}

TEST(PatrolTest, Update_LeavesACharacterStoodWhereItStandsOnNothing)
{
    PatrolHarness harness;

    harness.solidVoxels = floorOver(1);
    harness.stopPositions = {{groundAt(0, 0)}};
    harness.begin(Position{.x = 40.0F, .y = 0.5F, .z = 40.0F});
    harness.gameLoop.run(0);

    EXPECT_NEAR(harness.sent().velocityX, 0.0F, kTolerance);
    EXPECT_NEAR(harness.sent().velocityZ, 0.0F, kTolerance);
}

TEST(PatrolTest, Update_LeavesACharacterStoodWhereItsStopStandsOnNothing)
{
    PatrolHarness harness;

    harness.solidVoxels = floorOver(1);
    harness.stopPositions = {{groundAt(40, 40)}};
    harness.begin(Position{.x = 0.0F, .y = 0.5F, .z = 0.0F});
    harness.gameLoop.run(0);

    EXPECT_NEAR(harness.sent().velocityX, 0.0F, kTolerance);
    EXPECT_NEAR(harness.sent().velocityZ, 0.0F, kTolerance);
}

TEST(PatrolTest, Update_LeavesACharacterStoodWhereNoWalkReachesItsStop)
{
    PatrolHarness harness;

    harness.solidVoxels = floorOver(1);
    harness.solidVoxels.merge(voxelsOf({VoxelCell{.position = {.x = 8, .y = 0,
        .z = 8}}}));
    harness.stopPositions = {{groundAt(8, 8)}};
    harness.begin(Position{.x = 0.0F, .y = 0.5F, .z = 0.0F});
    harness.gameLoop.run(0);

    EXPECT_NEAR(harness.sent().velocityX, 0.0F, kTolerance);
    EXPECT_NEAR(harness.sent().velocityZ, 0.0F, kTolerance);
}

TEST(PatrolTest, Update_HoldsACharacterStillWhileItSpeaks)
{
    PatrolHarness harness;

    harness.solidVoxels = floorOver(3);
    harness.stopPositions = {{groundAt(3, 0)}};
    harness.begin(Position{.x = 0.0F, .y = 0.5F, .z = 0.0F});
    harness.system.setSpeaking(0U);
    harness.gameLoop.run(0);

    EXPECT_NEAR(harness.sent().velocityX, 0.0F, kTolerance);
}

TEST(PatrolTest, Update_LetsACharacterStrollAgainOnceItIsDone)
{
    PatrolHarness harness;

    harness.solidVoxels = floorOver(3);
    harness.stopPositions = {{groundAt(3, 0)}};
    harness.begin(Position{.x = 0.0F, .y = 0.5F, .z = 0.0F});
    harness.system.setSpeaking(0U);
    harness.gameLoop.run(0);
    harness.system.setSpeaking(std::nullopt);
    harness.gameLoop.run(1);

    EXPECT_NEAR(harness.sent().velocityX, 1.0F, kTolerance);
}

TEST(PatrolTest, Update_HoldsEveryCharacterStillWhileFrozen)
{
    PatrolHarness harness;

    harness.solidVoxels = floorOver(3);
    harness.stopPositions = {{groundAt(3, 0)}};
    harness.begin(Position{.x = 0.0F, .y = 0.5F, .z = 0.0F});
    harness.system.setFrozen(true);
    harness.gameLoop.run(0);

    EXPECT_NEAR(harness.sent().velocityX, 0.0F, kTolerance);
}

TEST(PatrolTest, Update_LeavesACharacterWithNoRosterEntryStandingStill)
{
    PatrolHarness harness;

    harness.solidVoxels = floorOver(3);
    harness.stopPositions = {};
    harness.begin(Position{.x = 0.0F, .y = 0.5F, .z = 0.0F});
    harness.gameLoop.run(0);

    EXPECT_NEAR(harness.sent().velocityX, 0.0F, kTolerance);
}

TEST(PatrolTest, Forget_LetsGoOfTheRoutesItPlanned)
{
    PatrolHarness harness;

    harness.solidVoxels = floorOver(3);
    harness.stopPositions = {{groundAt(3, 0)}};
    harness.begin(Position{.x = 0.0F, .y = 0.5F, .z = 0.0F});
    harness.gameLoop.run(0);
    harness.system.forget();
    harness.gameLoop.run(1);

    EXPECT_NEAR(harness.sent().velocityX, 1.0F, kTolerance);
}

TEST(PatrolTest, Update_StrollsEveryCharacterOfTheRoster)
{
    NiceMock<MockLogger> logger;
    Voxels solidVoxels = floorOver(3);
    std::vector<std::vector<VoxelPosition>> stopPositions{
        {groundAt(3, 0)}, {groundAt(-3, 0)}};
    World world(logger);
    GameLoop gameLoop(world);
    PatrolSystem system(solidVoxels, stopPositions);

    gameLoop.addSystem(Phase::Sending, system);

    std::vector<Entity> folkEntities;

    for (std::uint32_t index = 0; index < 2U; ++index)
    {
        const auto entity = gameLoop.world().create();

        gameLoop.world().add<Position>(
            entity, Position{.x = 0.0F, .y = 0.5F, .z = 0.0F});
        gameLoop.world().add<Velocity>(entity, Velocity{});
        gameLoop.world().add<Patrol>(entity, Patrol{});
        gameLoop.world().add<RosterIndex>(
            entity, RosterIndex{.index = index});
        folkEntities.push_back(entity);
    }

    gameLoop.world().commit();
    gameLoop.run(0);

    EXPECT_NEAR(
        gameLoop.world().get<Velocity>(folkEntities.at(0)).velocityX,
        1.0F,
        kTolerance);
    EXPECT_NEAR(
        gameLoop.world().get<Velocity>(folkEntities.at(1)).velocityX,
        -1.0F,
        kTolerance);
}
