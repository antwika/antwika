#include <gtest/gtest.h>

#include <cmath>
#include <set>
#include <string>
#include <vector>

#include <antwika/camera/FlyCamera.hpp>
#include <antwika/character/Character.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/gameplay/GateState.hpp>
#include <antwika/component/AnimationState.hpp>
#include <antwika/component/Orientation.hpp>
#include <antwika/component/Player.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/Velocity.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/collision/Collision.hpp>

#include "antwika/gameplay/Game.hpp"

using antwika::ecs::OpenPhase;
using antwika::ecs::World;
using antwika::voxel::VoxelPosition;
using antwika::voxel::Voxels;
using antwika::component::AnimationState;
using antwika::gameplay::Game;
using antwika::gameplay::GateState;
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

    [[nodiscard]] Voxels floorOver(
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
            const auto entity = game.world().create();

            {
                const OpenPhase phase(game.world());

                game.setPlayer(entity);
                game.world().add<Player>(entity, Player{});
                game.world().add<Velocity>(entity, Velocity{});
                game.world().add<Position>(
                    entity,
                    Position{
                        .x = x,
                        .y = antwika::voxel::kVoxelSide,
                        .z = z});
                game.world().add<AnimationState>(
                    entity, AnimationState{.direction = 3});
            }
        }

        NiceMock<MockLogger> logger;
        Voxels solids = floorOver(6);
        std::vector<std::vector<VoxelPosition>> patrolPositions;
        World world{logger};
        Game game{logger, world, solids, patrolPositions};
    };

}

TEST_F(GameTest, Game_StandsAnEyeTheViewIsTurnedBy)
{
    const auto &constGame = game;

    EXPECT_TRUE(constGame.world().alive(game.eye()));
    EXPECT_EQ(game.world().get<Orientation>(game.eye()).yaw, 0.0F);
}

TEST_F(GameTest, SetPlayer_NamesWhoTheGameIsPlayedAs)
{
    stand();

    EXPECT_TRUE(game.world().alive(game.player()));
    EXPECT_NE(game.player(), game.eye());
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
    game.setWalkerFrozen(false);
    game.arrowKeys().east = true;
    game.run(1);

    EXPECT_GT(game.playerAt().x, 0.0F);
}

TEST_F(GameTest, SetWalkerFrozen_HoldsTheWalkerStill)
{
    stand();
    game.setWalkerFrozen(true);
    game.arrowKeys().east = true;
    game.run(1);

    EXPECT_NEAR(game.playerAt().x, 0.0F, kTolerance);
}

TEST_F(GameTest, SetRunning_SendsTheWalkerFurtherInOneStep)
{
    stand();
    game.setWalkerFrozen(false);
    game.wasdKeys().east = true;
    game.run(1);

    const auto walkedX = game.playerAt().x;

    game.setRunning(true);
    game.run(2);

    EXPECT_GT(game.playerAt().x - walkedX, walkedX);
}

TEST_F(GameTest, Gates_StartEmptyAndAreKeptAsTheyAreSet)
{
    EXPECT_EQ(game.gates().keysHeld, 0U);

    game.gates().keysHeld = 2;

    const auto &constGame = game;

    EXPECT_EQ(constGame.gates().keysHeld, 2U);
}

TEST_F(GameTest, CameraTransform_StartsAtTheDefaultAndZoomAtTheDefaultZoom)
{
    const auto &constGame = game;

    EXPECT_EQ(
        constGame.cameraTransform().yaw,
        antwika::camera::defaultTransform().yaw);
    EXPECT_EQ(game.zoom(), antwika::camera::kDefaultZoom);
}

TEST_F(GameTest, AimAt_PutsThePlayedCameraOnAPlaceAtOnce)
{
    game.aimAt(
        antwika::gfx::Mat4(1.0F),
        antwika::gfx::Vec3{4.0F, 0.0F, 5.0F});

    EXPECT_NEAR(game.cameraTarget().x, 4.0F, kTolerance);
    EXPECT_NEAR(game.cameraTransform().position.x, 4.0F, kTolerance);
}

TEST_F(GameTest, Follow_ClosesOnlyAShareOfTheWayToThePlace)
{
    game.follow(
        antwika::gfx::Mat4(1.0F),
        antwika::gfx::Vec3{10.0F, 0.0F, 0.0F});

    EXPECT_NEAR(
        game.cameraTarget().x,
        10.0F * antwika::gameplay::kCameraFollowLerp,
        kTolerance);
}

TEST_F(GameTest, FollowPath_TakesUpTheStopsAndTheGoal)
{
    game.followPath(
        {antwika::gfx::Vec3{1.0F, 0.0F, 0.0F}},
        VoxelPosition{.x = 1, .y = 0, .z = 0});

    EXPECT_EQ(game.path().size(), 1U);
    ASSERT_TRUE(game.pathGoal().has_value());
    EXPECT_EQ(game.pathGoal()->x, 1);
}

TEST_F(GameTest, StepAlongPath_SendsTheWalkerAtTheStopItMakesFor)
{
    stand();
    game.setWalkerFrozen(false);
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

    EXPECT_TRUE(game.path().empty());
    EXPECT_FALSE(game.pathGoal().has_value());
}

TEST_F(GameTest, StepAlongPath_LeavesThePathAloneWhileNotPlaying)
{
    stand();
    game.followPath(
        {antwika::gfx::Vec3{3.0F, 0.0F, 0.0F}},
        VoxelPosition{.x = 3, .y = 0, .z = 0});
    game.stepAlongPath(false);

    EXPECT_EQ(game.path().size(), 1U);
}

TEST_F(GameTest, StepAlongPath_LetsTheKeysWinOverThePath)
{
    stand();
    game.setWalkerFrozen(false);
    game.wasdKeys().west = true;
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

    EXPECT_TRUE(game.path().empty());
    EXPECT_FALSE(game.pathGoal().has_value());
}

TEST_F(GameTest, ClearSteering_LeavesTheWalkerStandingStill)
{
    stand();
    game.setWalkerFrozen(false);
    game.followPath(
        {antwika::gfx::Vec3{3.0F, 0.0F, 0.0F}},
        VoxelPosition{.x = 3, .y = 0, .z = 0});
    game.stepAlongPath(true);
    game.clearSteering();
    game.run(1);

    EXPECT_NEAR(game.playerAt().x, 0.0F, kTolerance);
}

TEST_F(GameTest, SetWorldFrozen_HoldsWhatTheWorldDoesOfItsOwnAccord)
{
    stand();
    game.setWorldFrozen(true);
    game.setSpeaking(std::optional<std::uint32_t>{0});
    game.run(1);

    EXPECT_TRUE(game.world().alive(game.player()));
}

TEST_F(GameTest, ForgetPatrols_LeavesTheWorldStandingAsItWas)
{
    stand();
    game.forgetPatrols();
    game.run(1);

    EXPECT_TRUE(game.world().alive(game.player()));
}

TEST_F(GameTest, Progress_SaysWhichMapAndWhereThePlayerStands)
{
    stand(1.0F, 2.0F);

    const auto savedProgress = game.progress(std::string("map.json"));

    EXPECT_EQ(savedProgress.map, "map.json");
    EXPECT_NEAR(savedProgress.stancePlacement.position.x, 1.0F, kTolerance);
    EXPECT_EQ(savedProgress.stancePlacement.way, 3U);
}
