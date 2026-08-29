#include <gtest/gtest.h>

#include <cmath>
#include <set>
#include <string>
#include <vector>

#include <antwika/camera/FlyCamera.hpp>
#include <antwika/character/Character.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/gameplay/CheckpointState.hpp>
#include <antwika/component/AnimationState.hpp>
#include <antwika/component/CheckpointReport.hpp>
#include <antwika/component/ExitReport.hpp>
#include <antwika/component/Orientation.hpp>
#include <antwika/component/Pad.hpp>
#include <antwika/component/Player.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/Velocity.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/collision/Collision.hpp>

#include "antwika/gameplay/Game.hpp"
#include "antwika/gameplay/PadReports.hpp"

using antwika::ecs::OpenPhase;
using antwika::ecs::World;
using antwika::component::DirectionKeys;
using antwika::system::SimulationState;
using antwika::voxel::VoxelPosition;
using antwika::voxel::Voxels;
using antwika::component::AnimationState;
using antwika::gameplay::Game;
using antwika::gameplay::CheckpointState;
using antwika::component::Orientation;
using antwika::log::mocks::MockLogger;
using antwika::voxel::Kind;
using antwika::voxel::VoxelCell;
using antwika::component::Player;
using antwika::component::Position;
using antwika::component::Velocity;
using ::testing::NiceMock;
using antwika::voxel::voxelsOf;

namespace
{

    constexpr float kTolerance = 0.0001F;

    [[nodiscard]] Voxels getFloorOver(
        const std::int32_t reach)
    {
        Voxels voxels;

        for (auto x = -reach; x <= reach; ++x)
        {
            for (auto z = -reach; z <= reach; ++z)
            {
                voxels.merge(voxelsOf({VoxelCell{.position = {.x = x, .y = 0,
                    .z = z}, .material = {.kind = Kind::Normal}}}));
            }
        }

        return voxels;
    }

    class GameTest : public ::testing::Test
    {
    protected:
        void stand(const float x = 0.0F, const float z = 0.0F)
        {
            const auto entity = game.getWorld().create();

            {
                const OpenPhase phase(game.getWorld());

                game.setPlayer(entity);
                game.getWorld().add<Player>(entity, Player{});
                game.getWorld().add<Velocity>(entity, Velocity{});
                game.getWorld().add<Position>(
                    entity,
                    Position{
                        .x = x,
                        .y = antwika::voxel::kVoxelSide,
                        .z = z});
                game.getWorld().add<AnimationState>(
                    entity, AnimationState{.direction = 3});
            }
        }

        void layPad(
            const VoxelPosition position,
            const antwika::component::PadKind kind)
        {
            const auto entity = game.getWorld().create();

            const OpenPhase phase(game.getWorld());

            game.getWorld().add<antwika::component::Pad>(
                entity,
                antwika::component::Pad{
                    .position = position,
                    .kind = static_cast<std::uint8_t>(kind)});
        }

        NiceMock<MockLogger> logger;
        Voxels solids = getFloorOver(6);
        std::vector<std::vector<VoxelPosition>> patrolPositions;
        World world{logger};
        antwika::map::Map laidMap;
        Game game{logger, world, laidMap, solids, patrolPositions};
    };

}

TEST_F(GameTest, Game_StandsAnEyeTheViewIsTurnedBy)
{
    const auto &constGame = game;

    EXPECT_TRUE(constGame.getWorld().isAlive(game.getEye()));
    EXPECT_EQ(game.getWorld().get<Orientation>(game.getEye()).yaw, 0.0F);
}

TEST_F(GameTest, SetPlayer_NamesWhoTheGameIsPlayedAs)
{
    stand();

    EXPECT_TRUE(game.getWorld().isAlive(game.getPlayer()));
    EXPECT_NE(game.getPlayer(), game.getEye());
}

TEST_F(GameTest, PlayerAt_GivesWhereThePlayerStands)
{
    stand(2.0F, 3.0F);

    const auto playerPoint = game.playerAt();

    EXPECT_NEAR(playerPoint.x, 2.0F, kTolerance);
    EXPECT_NEAR(playerPoint.z, 3.0F, kTolerance);
}

TEST_F(GameTest, Run_WalksThePlayerTheWayTheKeysAsk)
{
    stand();
    game.setSimulation(SimulationState{});
    game.setArrowKeys(DirectionKeys{.east = true});
    game.run(1);

    EXPECT_GT(game.playerAt().x, 0.0F);
}

TEST_F(GameTest, SetWalkerHeld_HoldsTheWalkerStill)
{
    stand();
    game.setSimulation(SimulationState{.walkerHeld = true});
    game.setArrowKeys(DirectionKeys{.east = true});
    game.run(1);

    EXPECT_NEAR(game.playerAt().x, 0.0F, kTolerance);
}

TEST_F(GameTest, SetRunning_SendsTheWalkerFurtherInOneStep)
{
    stand();
    game.setSimulation(SimulationState{});
    game.setWasdKeys(DirectionKeys{.east = true});
    game.run(1);

    const auto walkedX = game.playerAt().x;

    game.setSimulation(SimulationState{.running = true});
    game.run(2);

    EXPECT_GT(game.playerAt().x - walkedX, walkedX);
}

TEST_F(GameTest, Checkpoint_StartsEmptyAndIsKeptAsItIsSet)
{
    EXPECT_FALSE(game.getCheckpoint().onPosition.has_value());

    auto heldCheckpoint = antwika::gameplay::CheckpointState{};

    heldCheckpoint.onPosition =
        antwika::voxel::VoxelPosition{.x = 2, .y = 0, .z = 2};
    game.setCheckpoint(heldCheckpoint);

    const auto &constGame = game;

    EXPECT_EQ(
        constGame.getCheckpoint().onPosition,
        (antwika::voxel::VoxelPosition{.x = 2, .y = 0, .z = 2}));
}

TEST_F(GameTest, CameraTransform_StartsAtTheDefaultAndZoomAtTheDefaultZoom)
{
    const auto &constGame = game;

    EXPECT_EQ(
        constGame.getCameraTransform().yaw,
        antwika::camera::getDefaultTransform().yaw);
    EXPECT_EQ(game.getZoom(), antwika::camera::kDefaultZoom);
}

TEST_F(GameTest, AimAt_PutsThePlayedCameraOnAPlaceAtOnce)
{
    game.aimAt(
        antwika::gfx::Mat4(1.0F),
        antwika::gfx::Vec3{4.0F, 0.0F, 5.0F});

    EXPECT_NEAR(game.getCameraTarget().x, 4.0F, kTolerance);
    EXPECT_NEAR(game.getCameraTransform().position.x, 4.0F, kTolerance);
}

TEST_F(GameTest, Follow_ClosesOnlyAShareOfTheWayToThePlace)
{
    game.follow(
        antwika::gfx::Mat4(1.0F),
        antwika::gfx::Vec3{10.0F, 0.0F, 0.0F});

    EXPECT_NEAR(
        game.getCameraTarget().x,
        10.0F * antwika::gameplay::kCameraFollowLerp,
        kTolerance);
}

TEST_F(GameTest, FollowPath_TakesUpTheStopsAndTheGoal)
{
    game.followPath(
        {antwika::gfx::Vec3{1.0F, 0.0F, 0.0F}},
        VoxelPosition{.x = 1, .y = 0, .z = 0});

    EXPECT_EQ(game.getPath().size(), 1U);
    ASSERT_TRUE(game.getPathGoal().has_value());
    EXPECT_EQ(game.getPathGoal()->x, 1);
}

TEST_F(GameTest, StepAlongPath_SendsTheWalkerAtTheStopItMakesFor)
{
    stand();
    game.setSimulation(SimulationState{});
    game.followPath(
        {antwika::gfx::Vec3{3.0F, 0.0F, 0.0F}},
        VoxelPosition{.x = 3, .y = 0, .z = 0});
    game.stepAlongPath(true);
    game.run(1);

    EXPECT_GT(game.playerAt().x, 0.0F);
}

TEST_F(GameTest, StepAlongPath_DropsThePathOnceTheLastStopIsReached)
{
    stand();
    game.followPath(
        {antwika::gfx::Vec3{0.0F, 0.0F, 0.0F}},
        VoxelPosition{.x = 0, .y = 0, .z = 0});
    game.stepAlongPath(true);

    EXPECT_TRUE(game.getPath().empty());
    EXPECT_FALSE(game.getPathGoal().has_value());
}

TEST_F(GameTest, StepAlongPath_LeavesThePathAloneWhileNotPlaying)
{
    stand();
    game.followPath(
        {antwika::gfx::Vec3{3.0F, 0.0F, 0.0F}},
        VoxelPosition{.x = 3, .y = 0, .z = 0});
    game.stepAlongPath(false);

    EXPECT_EQ(game.getPath().size(), 1U);
}

TEST_F(GameTest, StepAlongPath_LetsTheKeysWinOverThePath)
{
    stand();
    game.setSimulation(SimulationState{});
    game.setWasdKeys(DirectionKeys{.west = true});
    game.followPath(
        {antwika::gfx::Vec3{3.0F, 0.0F, 0.0F}},
        VoxelPosition{.x = 3, .y = 0, .z = 0});
    game.stepAlongPath(true);
    game.run(1);

    EXPECT_LT(game.playerAt().x, 0.0F);
}

TEST_F(GameTest, ClearPath_DropsWhateverWasBeingFollowed)
{
    game.followPath(
        {antwika::gfx::Vec3{3.0F, 0.0F, 0.0F}},
        VoxelPosition{.x = 3, .y = 0, .z = 0});
    game.clearPath();

    EXPECT_TRUE(game.getPath().empty());
    EXPECT_FALSE(game.getPathGoal().has_value());
}

TEST_F(GameTest, ClearSteering_LeavesTheWalkerStandingStill)
{
    stand();
    game.setSimulation(SimulationState{});
    game.followPath(
        {antwika::gfx::Vec3{3.0F, 0.0F, 0.0F}},
        VoxelPosition{.x = 3, .y = 0, .z = 0});
    game.stepAlongPath(true);
    game.clearSteering();
    game.run(1);

    EXPECT_NEAR(game.playerAt().x, 0.0F, kTolerance);
}

TEST_F(GameTest, SetSimulationPaused_HoldsWhatTheWorldDoesOfItsOwnAccord)
{
    stand();
    game.setSimulation(
        SimulationState{
            .simulationPaused = true,
            .speaking = std::optional<std::uint32_t>{0}});
    game.run(1);

    EXPECT_TRUE(game.getWorld().isAlive(game.getPlayer()));
}

TEST_F(GameTest, ForgetPatrols_LeavesTheWorldStandingAsItWas)
{
    stand();
    game.forgetPatrols();
    game.run(1);

    EXPECT_TRUE(game.getWorld().isAlive(game.getPlayer()));
}

TEST_F(GameTest, Progress_SaysWhichMapAndWhereThePlayerStands)
{
    stand(1.0F, 2.0F);

    const auto savedProgress = game.getProgress(std::string("map.json"));

    EXPECT_EQ(savedProgress.map, "map.json");
    EXPECT_NEAR(savedProgress.stancePlacement.position.x, 1.0F, kTolerance);
    EXPECT_EQ(savedProgress.stancePlacement.way, 3U);
}

TEST_F(GameTest, Run_ReportsTheCheckpointTheWalkerStandsOn)
{
    stand();
    layPad(VoxelPosition{}, antwika::component::PadKind::Checkpoint);

    game.run(1);

    EXPECT_TRUE(
        game.getWorld().has<antwika::component::CheckpointReport>(
            game.getPlayer()));
}

TEST_F(GameTest, Run_ReportsTheExitTheWalkerReaches)
{
    stand();
    layPad(VoxelPosition{}, antwika::component::PadKind::Exit);

    game.run(1);

    EXPECT_TRUE(
        game.getWorld().has<antwika::component::ExitReport>(
            game.getPlayer()));
}

TEST_F(GameTest, Run_MovesTheRespawnToTheCheckpointItRanOver)
{
    stand(2.0F, 3.0F);
    layPad(VoxelPosition{.x = 2, .y = 0, .z = 2},
        antwika::component::PadKind::Checkpoint);

    game.run(1);

    EXPECT_TRUE(
        antwika::gameplay::takeCheckpointReport(
            game, game.getWorld(), game.getPlayer()));
    EXPECT_EQ(
        game.getCheckpoint().onPosition,
        (VoxelPosition{.x = 2, .y = 0, .z = 2}));
}

TEST_F(GameTest, Run_ReadsNoPadWhileTheSimulationIsPaused)
{
    stand();
    layPad(VoxelPosition{}, antwika::component::PadKind::Exit);
    game.setSimulation(SimulationState{.simulationPaused = true});

    game.run(1);

    EXPECT_FALSE(
        game.getWorld().has<antwika::component::ExitReport>(
            game.getPlayer()));
}

TEST_F(GameTest, Game_TakesTheEyeTheWorldAlreadyStands)
{
    const Game secondGame{logger, world, laidMap, solids, patrolPositions};

    EXPECT_EQ(secondGame.getEye(), game.getEye());
}

TEST_F(GameTest, SetOrientation_ShowsTheTurnOnceTheWorldHasCommitted)
{
    world.set<Orientation>(game.getEye(), Orientation{.yaw = 0.5F});

    EXPECT_FLOAT_EQ(world.get<Orientation>(game.getEye()).yaw, 0.0F);

    {
        const OpenPhase phase(world);
    }

    EXPECT_FLOAT_EQ(world.get<Orientation>(game.getEye()).yaw, 0.5F);
}
