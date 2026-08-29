#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <string>

#include <antwika/gfx/NullBackend.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/NullInputBackend.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/map/Marker.hpp>
#include <antwika/ui/Interactions.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

#include "antwika/editor/Editor.hpp"
#include "antwika/editor/fakes/EditorProbe.hpp"
#include "antwika/editor/ui/WidgetIds.hpp"

using antwika::editor::Tool;
using antwika::editor::ToolButton;
using antwika::input::MouseButton;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{

    constexpr std::string_view kMissingMapPath =
        "assets/maps/no-such-map.json";

    constexpr antwika::voxel::VoxelPosition kMarkerPosition{
        .x = 1, .y = 2, .z = 3};

    /**
     * @brief Where kMarkerPosition lands once its x is asked for the
     * seventh cube, keeping the step it stands at within that cube.
     */
    constexpr antwika::voxel::VoxelPosition kSeventhCubePosition{
        .x = 15, .y = 2, .z = 3};

    class EntityPanelTest : public ::testing::Test
    {
    protected:
        [[nodiscard]] std::vector<antwika::voxel::VoxelPosition> &
        checkpointCells()
        {
            return probe.document.map.markers.positionsOf(
                antwika::map::Marker::Checkpoint);
        }

        [[nodiscard]] bool holdsCheckpointCell(
            const antwika::voxel::VoxelPosition position)
        {
            return std::ranges::find(checkpointCells(), position)
                   != checkpointCells().end();
        }

        void pickCheckpointMarker()
        {
            probe.preferences().tool = Tool::Checkpoint;
            checkpointCells().push_back(kMarkerPosition);
            probe.pressMarker(kMarkerPosition, MouseButton::Left);
        }

        NiceMock<MockLogger> logger;
        antwika::gfx::NullBackend backend{logger};
        antwika::input::NullInputBackend inputs{logger};
        antwika::editor::Editor editor{
            logger, backend, inputs, std::string(kMissingMapPath)};
        antwika::editor::fakes::EditorProbe probe{editor};
    };

}

TEST_F(EntityPanelTest, PressMarker_LaysAMarkerOnAFilledCubeAsItStands)
{
    probe.preferences().tool = Tool::Checkpoint;

    constexpr antwika::voxel::VoxelPosition filledPosition{
        .x = 0, .y = 0, .z = 0};
    const auto voxelsBefore = probe.document.map.voxels;

    ASSERT_NE(
        probe.document.map.voxels.find(filledPosition),
        probe.document.map.voxels.end());

    probe.pressMarker(filledPosition, MouseButton::Left);

    EXPECT_TRUE(holdsCheckpointCell(filledPosition));
    EXPECT_EQ(probe.document.map.voxels, voxelsBefore);
}

TEST_F(EntityPanelTest, PressMarker_LeavesAMarkerInTheAirWithoutACube)
{
    probe.preferences().tool = Tool::Checkpoint;

    ASSERT_EQ(
        probe.document.map.voxels.find(kMarkerPosition),
        probe.document.map.voxels.end());

    const auto voxelsBefore = probe.document.map.voxels;

    probe.pressMarker(kMarkerPosition, MouseButton::Left);

    EXPECT_TRUE(holdsCheckpointCell(kMarkerPosition));
    EXPECT_EQ(probe.document.map.voxels, voxelsBefore);
}

TEST_F(EntityPanelTest, MarkerRemoveButton_TakesTheChosenMarkerAway)
{
    probe.preferences().tool = Tool::Checkpoint;
    probe.pressMarker(kMarkerPosition, MouseButton::Left);
    probe.pressMarker(kMarkerPosition, MouseButton::Left);

    ASSERT_TRUE(probe.isMarkerSectionShown());
    EXPECT_TRUE(
        probe.consumeWidgets(
            antwika::ui::Interactions{
                .activatedWidget = antwika::editor::kMarkerRemoveWidget}));
    EXPECT_TRUE(checkpointCells().empty());
}

TEST_F(EntityPanelTest, PressMarker_PicksTheClickedMarkerWithoutASnapshot)
{
    const auto countBefore = probe.document.getUndoCount();

    pickCheckpointMarker();

    EXPECT_EQ(
        probe.markerPick().marker, antwika::map::Marker::Checkpoint);
    EXPECT_EQ(probe.markerPick().position, kMarkerPosition);
    EXPECT_EQ(probe.document.getUndoCount(), countBefore);
    EXPECT_TRUE(probe.isMarkerSectionShown());
}

TEST_F(EntityPanelTest, PressTool_ClearsTheMarkerPick)
{
    pickCheckpointMarker();
    probe.pressTool(ToolButton::StoneCube);

    EXPECT_FALSE(probe.markerPick().marker.has_value());
    EXPECT_FALSE(probe.isMarkerSectionShown());
}

TEST_F(EntityPanelTest, IsMarkerSectionShown_NeedsTheMatchingTool)
{
    pickCheckpointMarker();

    probe.preferences().tool = Tool::Food;

    EXPECT_FALSE(probe.isMarkerSectionShown());

    probe.preferences().tool = Tool::Checkpoint;

    EXPECT_TRUE(probe.isMarkerSectionShown());
}

TEST_F(EntityPanelTest, MarkerFieldEnter_MovesTheMarkerAndUndoes)
{
    pickCheckpointMarker();

    const auto countBefore = probe.document.getUndoCount();
    const auto fieldWidget = antwika::editor::getMarkerFieldWidget(0);

    EXPECT_TRUE(
        probe.consumeWidgets(
            antwika::ui::Interactions{.activatedWidget = fieldWidget}));
    EXPECT_EQ(probe.document.getUndoCount(), countBefore);
    EXPECT_EQ(probe.markerPick().pendingAxisText, "0");

    probe.carryEdit(fieldWidget, "7");

    EXPECT_TRUE(
        probe.consumeTextInput(
            antwika::input::KeyPressed{
                .key = antwika::input::Key::Enter}));
    EXPECT_EQ(probe.document.getUndoCount(), countBefore + 1);
    EXPECT_TRUE(holdsCheckpointCell(kSeventhCubePosition));
    EXPECT_FALSE(holdsCheckpointCell(kMarkerPosition));
    EXPECT_TRUE(probe.isMarkerSectionShown());

    probe.undo();

    EXPECT_TRUE(holdsCheckpointCell(kMarkerPosition));
    EXPECT_FALSE(holdsCheckpointCell(kSeventhCubePosition));
}

TEST_F(EntityPanelTest, MarkerFieldEscape_LeavesTheMarkerAlone)
{
    pickCheckpointMarker();

    const auto countBefore = probe.document.getUndoCount();
    const auto fieldWidget = antwika::editor::getMarkerFieldWidget(2);

    EXPECT_TRUE(
        probe.consumeWidgets(
            antwika::ui::Interactions{.activatedWidget = fieldWidget}));

    probe.carryEdit(fieldWidget, "9");

    EXPECT_TRUE(
        probe.consumeTextInput(
            antwika::input::KeyPressed{
                .key = antwika::input::Key::Escape}));
    EXPECT_EQ(probe.document.getUndoCount(), countBefore);
    EXPECT_TRUE(holdsCheckpointCell(kMarkerPosition));
    EXPECT_FALSE(probe.markerPick().editingAxis.has_value());
    EXPECT_EQ(
        probe.getFocusedField(), antwika::editor::FocusedField::Nothing);
}

TEST_F(EntityPanelTest, MarkerFieldEnter_RefusesTextThatIsNoWholeNumber)
{
    pickCheckpointMarker();

    const auto countBefore = probe.document.getUndoCount();
    const auto fieldWidget = antwika::editor::getMarkerFieldWidget(1);

    EXPECT_TRUE(
        probe.consumeWidgets(
            antwika::ui::Interactions{.activatedWidget = fieldWidget}));

    probe.carryEdit(fieldWidget, "over there");

    EXPECT_TRUE(
        probe.consumeTextInput(
            antwika::input::KeyPressed{
                .key = antwika::input::Key::Enter}));
    EXPECT_EQ(probe.document.getUndoCount(), countBefore);
    EXPECT_TRUE(holdsCheckpointCell(kMarkerPosition));
}

TEST(MarkerAxisTest, CubeAxisOf_CountsThePositionInWholeCubes)
{
    using antwika::editor::getCubeAxisOf;

    constexpr antwika::voxel::VoxelPosition somePosition{
        .x = 5, .y = -1, .z = 0};

    EXPECT_EQ(getCubeAxisOf(somePosition, 0), 2);
    EXPECT_EQ(getCubeAxisOf(somePosition, 1), -1);
    EXPECT_EQ(getCubeAxisOf(somePosition, 2), 0);
}

TEST(MarkerAxisTest, WithCubeAxisSet_KeepsTheStepWithinTheCube)
{
    using antwika::editor::getCubeAxisOf;
    using antwika::editor::getWithCubeAxisSet;

    constexpr antwika::voxel::VoxelPosition somePosition{
        .x = 5, .y = 2, .z = 3};
    const auto nextPosition = getWithCubeAxisSet(somePosition, 0, 4);

    EXPECT_EQ(nextPosition.x, 9);
    EXPECT_EQ(nextPosition.y, somePosition.y);
    EXPECT_EQ(nextPosition.z, somePosition.z);
    EXPECT_EQ(getCubeAxisOf(nextPosition, 0), 4);
}

TEST(MarkerAxisTest, WithCubeAxisSet_TakesBackWhatCubeAxisOfGave)
{
    using antwika::editor::getCubeAxisOf;
    using antwika::editor::getWithCubeAxisSet;

    constexpr antwika::voxel::VoxelPosition somePosition{
        .x = -3, .y = 2, .z = 3};

    EXPECT_EQ(
        getWithCubeAxisSet(somePosition, 0, getCubeAxisOf(somePosition, 0)),
        somePosition);
}
