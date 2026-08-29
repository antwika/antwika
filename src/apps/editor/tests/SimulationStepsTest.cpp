#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <string>

#include <antwika/component/AnimationState.hpp>
#include <antwika/component/CheckpointReport.hpp>
#include <antwika/component/ConsumeIntent.hpp>
#include <antwika/component/ConsumeReport.hpp>
#include <antwika/component/DialogueLine.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/TalkIntent.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/gameplay/fakes/FakeCheckpointProgress.hpp>
#include <antwika/gameplay/fakes/FakeWorldAccess.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/voxel/Voxels.hpp>

#include "antwika/editor/editor/SimulationSteps.hpp"
#include "antwika/editor/ui/EditorLook.hpp"

using antwika::component::AnimationState;
using antwika::component::CheckpointReport;
using antwika::component::ConsumeReport;
using antwika::component::DialogueLine;
using antwika::component::ItemKind;
using antwika::component::Position;
using antwika::component::TalkIntent;
using antwika::ecs::OpenPhase;
using antwika::ecs::World;
using antwika::editor::SimulationSteps;
using antwika::gameplay::fakes::FakeCheckpointProgress;
using antwika::gameplay::fakes::FakeWorldAccess;
using antwika::log::mocks::MockLogger;
using antwika::voxel::VoxelCell;
using antwika::voxel::VoxelPosition;
using antwika::voxel::voxelsOf;
using ::testing::NiceMock;

namespace
{

    class SimulationStepsTest : public ::testing::Test
    {
    protected:
        SimulationStepsTest()
            : world(logger), playerEntity(world.create()), access(world, playerEntity)
        {
            const OpenPhase phase(world);

            world.add<Position>(
                playerEntity, Position{.x = 1.5F, .y = 2.0F, .z = 3.5F});
            world.add<AnimationState>(
                playerEntity, AnimationState{.direction = 3});
        }

        void reportCheckpoint(const VoxelPosition position)
        {
            const OpenPhase phase(world);

            world.add<CheckpointReport>(
                playerEntity, CheckpointReport{.position = position});
        }

        NiceMock<MockLogger> logger;
        World world;
        antwika::ecs::Entity playerEntity;
        FakeWorldAccess access;
        FakeCheckpointProgress progress;
        antwika::editor::EditorDocument document;
        std::uint32_t tick = 0;
        SimulationSteps simulation{document, tick};
    };

}

TEST_F(SimulationStepsTest, CheckpointReport_SetsTheRespawnOnAPad)
{
    reportCheckpoint(VoxelPosition{.x = 4, .y = 0, .z = 2});

    simulation.sayCheckpointReport(progress, access);

    EXPECT_EQ(
        progress.checkpoint.onPosition,
        (VoxelPosition{.x = 4, .y = 0, .z = 2}));
    ASSERT_TRUE(progress.checkpoint.placement.has_value());
    EXPECT_FLOAT_EQ(progress.checkpoint.placement->position.x, 1.5F);
    EXPECT_EQ(progress.checkpoint.placement->way, 3U);
    EXPECT_EQ(simulation.caption.name, "checkpoint");
    EXPECT_FALSE(world.has<CheckpointReport>(playerEntity));
}

TEST_F(SimulationStepsTest, CheckpointReport_SaysNothingWithNoPadReported)
{
    simulation.sayCheckpointReport(progress, access);

    EXPECT_FALSE(progress.checkpoint.onPosition.has_value());
    EXPECT_TRUE(simulation.caption.line.empty());
}

TEST_F(SimulationStepsTest, CheckpointReport_SetsThePadButOnce)
{
    reportCheckpoint(VoxelPosition{.x = 4, .y = 0, .z = 2});
    simulation.sayCheckpointReport(progress, access);

    const auto saidUntil = simulation.caption.untilTick;

    reportCheckpoint(VoxelPosition{.x = 4, .y = 0, .z = 2});
    simulation.sayCheckpointReport(progress, access);

    EXPECT_EQ(simulation.caption.untilTick, saidUntil);
}

TEST_F(SimulationStepsTest, Interact_SendsATalkIntentWhenNoCaptionIsUp)
{
    simulation.interact(access);

    EXPECT_TRUE(world.has<TalkIntent>(playerEntity));
}

TEST_F(SimulationStepsTest, Interact_HurriesACaptionThatIsStillRevealing)
{
    simulation.sayCaption("someone", "hello");

    simulation.interact(access);

    EXPECT_FALSE(world.has<TalkIntent>(playerEntity));
    EXPECT_EQ(
        simulation.caption.untilTick, antwika::editor::kCaptionHoldTicks);
}

TEST_F(SimulationStepsTest, SayDialogueLine_SpeaksTheCharactersLineAndTakesItBack)
{
    antwika::map::Character speakingCharacter;

    speakingCharacter.name = "the smith";
    speakingCharacter.dialogue = {"hello there", "come again"};
    document.map.characters.push_back(speakingCharacter);

    {
        const OpenPhase phase(world);

        world.add<DialogueLine>(
            playerEntity, DialogueLine{.characterIndex = 0, .lineIndex = 1});
    }

    simulation.sayDialogueLine(access);

    EXPECT_EQ(simulation.caption.name, "the smith");
    EXPECT_EQ(simulation.caption.line, "come again");
    EXPECT_EQ(simulation.caption.speaker, 0U);
    EXPECT_FALSE(world.has<DialogueLine>(playerEntity));
}

TEST_F(SimulationStepsTest, ConsumeItem_SendsTheIntentAndTheReportIsSpoken)
{
    simulation.consumeItem(access, ItemKind::Food);

    EXPECT_TRUE(world.has<antwika::component::ConsumeIntent>(playerEntity));

    {
        const OpenPhase phase(world);

        world.add<ConsumeReport>(
            playerEntity,
            ConsumeReport{
                .kind = static_cast<std::uint8_t>(ItemKind::Food),
                .anyLeft = true});
    }

    simulation.sayConsumeReport(access);

    EXPECT_EQ(simulation.caption.name, "food");
    EXPECT_EQ(simulation.caption.line, "eaten");
    EXPECT_FALSE(world.has<ConsumeReport>(playerEntity));
}

TEST_F(SimulationStepsTest, SayConsumeReport_TellsWhenNothingWasLeft)
{
    {
        const OpenPhase phase(world);

        world.add<ConsumeReport>(
            playerEntity,
            ConsumeReport{
                .kind = static_cast<std::uint8_t>(ItemKind::Water),
                .anyLeft = false});
    }

    simulation.sayConsumeReport(access);

    EXPECT_EQ(simulation.caption.name, "water");
    EXPECT_EQ(simulation.caption.line, "there is none left to take");
}
