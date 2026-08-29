#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

#include <antwika/component/AnimationState.hpp>
#include <antwika/component/CheckpointReport.hpp>
#include <antwika/component/ExitReport.hpp>
#include <antwika/component/Pad.hpp>
#include <antwika/component/Player.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/gameplay/fakes/FakeCheckpointProgress.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/map/MapFile.hpp>
#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/Voxels.hpp>

#include "antwika/gameplay/Characters.hpp"
#include "antwika/gameplay/ComponentNames.hpp"
#include "antwika/gameplay/PadReports.hpp"
#include "antwika/gameplay/SpawnSystem.hpp"

using antwika::component::AnimationState;
using antwika::component::CheckpointReport;
using antwika::component::ExitReport;
using antwika::component::Pad;
using antwika::component::PadKind;
using antwika::component::Position;
using antwika::ecs::Entity;
using antwika::ecs::OpenPhase;
using antwika::ecs::World;
using antwika::gameplay::claimModuleComponents;
using antwika::gameplay::spawnPads;
using antwika::gameplay::takeCheckpointReport;
using antwika::gameplay::takeExitReport;
using antwika::gameplay::fakes::FakeCheckpointProgress;
using antwika::log::mocks::MockLogger;
using antwika::map::Map;
using antwika::voxel::VoxelPosition;
using ::testing::NiceMock;

namespace
{

    constexpr VoxelPosition kPadPosition{.x = 4, .y = 0, .z = 2};

    class PadReportsTest : public ::testing::Test
    {
    protected:
        PadReportsTest() : world(logger), walkerEntity(world.create())
        {
            claimModuleComponents(world);

            const OpenPhase phase(world);

            world.add<Position>(
                walkerEntity,
                Position{.x = 1.5F, .y = 2.0F, .z = 3.5F});
            world.add<AnimationState>(
                walkerEntity, AnimationState{.direction = 3});
        }

        void report(const VoxelPosition position)
        {
            const OpenPhase phase(world);

            world.add<CheckpointReport>(
                walkerEntity,
                CheckpointReport{.position = position});
        }

        NiceMock<MockLogger> logger;
        World world;
        Entity walkerEntity;
        FakeCheckpointProgress progress;
    };

}

TEST_F(PadReportsTest, TakeCheckpointReport_MovesTheRespawnToThePad)
{
    report(kPadPosition);

    EXPECT_TRUE(takeCheckpointReport(progress, world, walkerEntity));
    EXPECT_EQ(progress.checkpoint.onPosition, kPadPosition);
    ASSERT_TRUE(progress.checkpoint.placement.has_value());
    EXPECT_FLOAT_EQ(progress.checkpoint.placement->position.x, 1.5F);
    EXPECT_EQ(progress.checkpoint.placement->way, 3U);
    EXPECT_FALSE(world.has<CheckpointReport>(walkerEntity));
}

TEST_F(PadReportsTest, TakeCheckpointReport_HasNothingToTakeWithNoReport)
{
    EXPECT_FALSE(takeCheckpointReport(progress, world, walkerEntity));
    EXPECT_FALSE(progress.checkpoint.onPosition.has_value());
}

TEST_F(PadReportsTest, TakeCheckpointReport_LeavesTheRespawnOnTheSamePad)
{
    report(kPadPosition);

    EXPECT_TRUE(takeCheckpointReport(progress, world, walkerEntity));

    report(kPadPosition);

    EXPECT_FALSE(takeCheckpointReport(progress, world, walkerEntity));
    EXPECT_EQ(progress.checkpoint.onPosition, kPadPosition);
}

TEST_F(PadReportsTest, TakeCheckpointReport_FacesTheWayItStandsWithNoAnimation)
{
    World bareWorld(logger);
    const auto bareWalker = bareWorld.create();

    claimModuleComponents(bareWorld);

    {
        const OpenPhase phase(bareWorld);

        bareWorld.add<Position>(bareWalker, Position{});
        bareWorld.add<CheckpointReport>(
            bareWalker, CheckpointReport{.position = kPadPosition});
    }

    EXPECT_TRUE(takeCheckpointReport(progress, bareWorld, bareWalker));
    ASSERT_TRUE(progress.checkpoint.placement.has_value());
    EXPECT_EQ(progress.checkpoint.placement->way, 0U);
}

TEST_F(PadReportsTest, TakeExitReport_TakesTheReportAwayAsItAnswers)
{
    {
        const OpenPhase phase(world);

        world.add<ExitReport>(
            walkerEntity, ExitReport{.position = kPadPosition});
    }

    EXPECT_TRUE(takeExitReport(world, walkerEntity));
    EXPECT_FALSE(world.has<ExitReport>(walkerEntity));
    EXPECT_FALSE(takeExitReport(world, walkerEntity));
}

TEST_F(PadReportsTest, TakeExitReport_HasNothingToTakeWithNoReport)
{
    EXPECT_FALSE(takeExitReport(world, walkerEntity));
}

TEST_F(PadReportsTest, SpawnPads_StandsOneInEveryCubeTheMapMarks)
{
    Map laidMap;

    laidMap.spawnCubePosition = VoxelPosition{.x = 0, .y = 0, .z = 0};
    laidMap.exitCubePosition = VoxelPosition{.x = 2, .y = 0, .z = 0};
    laidMap.markers.positionsOf(antwika::map::Marker::Checkpoint)
        .push_back(kPadPosition);

    {
        const OpenPhase phase(world);

        spawnPads(world, laidMap);
    }

    std::size_t startCount = 0;
    std::size_t exitCount = 0;
    std::size_t checkpointCount = 0;

    for (const auto entity : world.view<Pad>())
    {
        switch (static_cast<PadKind>(world.get<Pad>(entity).kind))
        {
        case PadKind::Start:
            ++startCount;
            break;
        case PadKind::Exit:
            ++exitCount;
            break;
        case PadKind::Checkpoint:
            ++checkpointCount;
            EXPECT_EQ(world.get<Pad>(entity).position, kPadPosition);
            break;
        }
    }

    EXPECT_EQ(startCount, 1U);
    EXPECT_EQ(exitCount, 1U);
    EXPECT_EQ(checkpointCount, 1U);
}

TEST_F(PadReportsTest, RelayPads_LaysThemAfreshWhenOneHasMovedCube)
{
    Map laidMap;

    laidMap.exitCubePosition = VoxelPosition{.x = 2, .y = 0, .z = 0};

    EXPECT_TRUE(antwika::gameplay::relayPads(world, laidMap));
    EXPECT_FALSE(antwika::gameplay::relayPads(world, laidMap));

    laidMap.exitCubePosition = VoxelPosition{.x = 4, .y = 0, .z = 0};

    EXPECT_TRUE(antwika::gameplay::relayPads(world, laidMap));
    EXPECT_EQ(world.get<Pad>(*world.view<Pad>().begin()).position,
        (VoxelPosition{.x = 4, .y = 0, .z = 0}));
}

TEST_F(PadReportsTest, RelayPads_LaysThemAfreshWhenOneHasChangedKind)
{
    Map laidMap;

    laidMap.spawnCubePosition = VoxelPosition{.x = 2, .y = 0, .z = 0};

    EXPECT_TRUE(antwika::gameplay::relayPads(world, laidMap));

    laidMap.spawnCubePosition.reset();
    laidMap.exitCubePosition = VoxelPosition{.x = 2, .y = 0, .z = 0};

    EXPECT_TRUE(antwika::gameplay::relayPads(world, laidMap));
    EXPECT_EQ(
        static_cast<PadKind>(
            world.get<Pad>(*world.view<Pad>().begin()).kind),
        PadKind::Exit);
}

TEST_F(PadReportsTest, SpawnPads_StandsNoneForAMapThatMarksNothing)
{
    const Map laidMap;

    {
        const OpenPhase phase(world);

        spawnPads(world, laidMap);
    }

    EXPECT_TRUE(world.view<Pad>().begin() == world.view<Pad>().end());
}

namespace
{

    [[nodiscard]] antwika::voxel::Voxels getFloorUnderTheStart()
    {
        antwika::voxel::Voxels voxels;

        for (std::int32_t x = 0; x < 2; ++x)
        {
            for (std::int32_t z = 0; z < 2; ++z)
            {
                voxels.merge(
                    antwika::voxel::voxelsOf(
                        {antwika::voxel::VoxelCell{
                            .position = {.x = x, .y = 0, .z = z}}}));
            }
        }

        return voxels;
    }

    [[nodiscard]] Map getMapWithAHero()
    {
        Map laidMap;

        laidMap.characters.push_back(
            antwika::map::Character{
                .components =
                    std::vector<std::string>{
                        "component::Position",
                        "component::AnimationState",
                        "component::CharacterIndex",
                        "component::Player"},
                .player = true});

        return laidMap;
    }

}

TEST_F(PadReportsTest, SpawnSystem_StandsAHeroOnTheStartPad)
{
    auto laidMap = getMapWithAHero();

    laidMap.spawnCubePosition = VoxelPosition{.x = 0, .y = 0, .z = 0};

    World bareWorld(logger);

    claimModuleComponents(bareWorld);

    {
        const OpenPhase phase(bareWorld);

        spawnPads(bareWorld, laidMap);
    }

    const auto solidVoxels = getFloorUnderTheStart();
    antwika::gameplay::SpawnSystem spawnSystem(
        laidMap, solidVoxels, progress);

    spawnSystem.update(bareWorld, 0);

    EXPECT_TRUE(bareWorld.isAlive(spawnSystem.getStoodWalkerEntity()));
    EXPECT_TRUE(
        bareWorld.has<antwika::component::Player>(
            spawnSystem.getStoodWalkerEntity()));
    EXPECT_NEAR(
        bareWorld.get<Position>(spawnSystem.getStoodWalkerEntity()).x,
        1.0F,
        0.001F);
}

TEST_F(PadReportsTest, SpawnSystem_StandsNoSecondHeroWhileOneWalks)
{
    const auto laidMap = getMapWithAHero();
    World bareWorld(logger);

    claimModuleComponents(bareWorld);

    const antwika::voxel::Voxels solidVoxels;
    antwika::gameplay::SpawnSystem spawnSystem(
        laidMap, solidVoxels, progress);

    spawnSystem.update(bareWorld, 0);

    const auto stoodWalkerEntity = spawnSystem.getStoodWalkerEntity();

    spawnSystem.update(bareWorld, 1);

    EXPECT_EQ(spawnSystem.getStoodWalkerEntity(), stoodWalkerEntity);
}

TEST_F(PadReportsTest, SpawnSystem_StandsAHeroAfreshOnceTheOneBeforeIsGone)
{
    const auto laidMap = getMapWithAHero();
    World bareWorld(logger);

    claimModuleComponents(bareWorld);

    const antwika::voxel::Voxels solidVoxels;
    antwika::gameplay::SpawnSystem spawnSystem(
        laidMap, solidVoxels, progress);

    spawnSystem.update(bareWorld, 0);

    const auto stoodWalkerEntity = spawnSystem.getStoodWalkerEntity();

    {
        const OpenPhase phase(bareWorld);

        bareWorld.destroy(stoodWalkerEntity);
    }

    spawnSystem.update(bareWorld, 1);

    EXPECT_NE(spawnSystem.getStoodWalkerEntity(), stoodWalkerEntity);
    EXPECT_TRUE(bareWorld.isAlive(spawnSystem.getStoodWalkerEntity()));
}

TEST_F(PadReportsTest, SpawnSystem_StandsTheHeroOnTheCheckpointItReached)
{
    auto laidMap = getMapWithAHero();

    laidMap.spawnCubePosition = VoxelPosition{.x = 0, .y = 0, .z = 0};

    World bareWorld(logger);

    claimModuleComponents(bareWorld);

    {
        const OpenPhase phase(bareWorld);

        spawnPads(bareWorld, laidMap);
    }

    progress.checkpoint = antwika::gameplay::CheckpointState{
        .placement =
            antwika::map::Placement{.position = {9.0F, 3.0F, 4.0F}},
        .onPosition = kPadPosition};

    const antwika::voxel::Voxels solidVoxels;
    antwika::gameplay::SpawnSystem spawnSystem(
        laidMap, solidVoxels, progress);

    spawnSystem.update(bareWorld, 0);

    EXPECT_NEAR(
        bareWorld.get<Position>(spawnSystem.getStoodWalkerEntity()).x,
        9.0F,
        0.001F);
}

TEST_F(PadReportsTest, SpawnSystem_StandsNoHeroForAMapThatNamesNone)
{
    const Map laidMap;
    World bareWorld(logger);

    claimModuleComponents(bareWorld);

    const antwika::voxel::Voxels solidVoxels;
    antwika::gameplay::SpawnSystem spawnSystem(
        laidMap, solidVoxels, progress);

    spawnSystem.update(bareWorld, 0);

    EXPECT_FALSE(bareWorld.isAlive(spawnSystem.getStoodWalkerEntity()));
}
