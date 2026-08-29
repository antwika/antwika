#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <string_view>

#include <antwika/gfx/NullBackend.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/NullInputBackend.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/map/Marker.hpp>
#include <antwika/ui/Interactions.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

#include "antwika/editor/Editor.hpp"
#include "antwika/editor/fakes/EditorProbe.hpp"
#include "antwika/editor/ui/WidgetIds.hpp"

using antwika::editor::EntityKind;
using antwika::editor::Tool;
using antwika::editor::ToolButton;
using antwika::input::MouseButton;
using antwika::log::mocks::MockLogger;
using antwika::map::Marker;
using antwika::voxel::VoxelPosition;
using ::testing::NiceMock;

namespace
{

    constexpr std::string_view kMissingMapPath =
        "assets/maps/no-such-map.json";

    constexpr VoxelPosition kFarPosition{.x = 20, .y = 8, .z = 20};

    class EditorSelectTest : public ::testing::Test
    {
    protected:
        EditorSelectTest()
        {
            probe.preferences().tool = Tool::Select;
        }

        [[nodiscard]] std::vector<VoxelPosition> &foodCells()
        {
            return probe.document.map.markers.positionsOf(Marker::Food);
        }

        NiceMock<MockLogger> logger;
        antwika::gfx::NullBackend backend{logger};
        antwika::input::NullInputBackend inputs{logger};
        antwika::editor::Editor editor{
            logger, backend, inputs, std::string(kMissingMapPath)};
        antwika::editor::fakes::EditorProbe probe{editor};
    };

}

TEST_F(EditorSelectTest, PressSelect_PicksTheMarkerUnderThePointer)
{
    foodCells().push_back(kFarPosition);

    const auto countBefore = probe.document.getUndoCount();

    probe.pressSelect(kFarPosition, MouseButton::Left);

    EXPECT_EQ(probe.entityPick().kind, EntityKind::Food);
    EXPECT_EQ(probe.entityPick().position, kFarPosition);
    EXPECT_EQ(probe.document.getUndoCount(), countBefore);
    EXPECT_TRUE(probe.isEntitySectionShown());
}

TEST_F(EditorSelectTest, PressSelect_PicksTheLampStartAndExit)
{
    probe.document.map.lamps.push_back(
        antwika::light::Lamp{
            .position = kFarPosition, .tintColor = antwika::gfx::Color{}});
    probe.pressSelect(kFarPosition, MouseButton::Left);

    EXPECT_EQ(probe.entityPick().kind, EntityKind::Lamp);

    constexpr VoxelPosition startPosition{.x = 24, .y = 0, .z = 24};

    probe.document.map.spawnCubePosition = startPosition;
    probe.pressSelect(startPosition, MouseButton::Left);

    EXPECT_EQ(probe.entityPick().kind, EntityKind::Start);

    constexpr VoxelPosition exitPosition{.x = 28, .y = 0, .z = 28};

    probe.document.map.exitCubePosition = exitPosition;
    probe.pressSelect(exitPosition, MouseButton::Left);

    EXPECT_EQ(probe.entityPick().kind, EntityKind::Exit);
}

TEST_F(EditorSelectTest, PressSelect_LetsGoWhereNothingStands)
{
    foodCells().push_back(kFarPosition);
    probe.pressSelect(kFarPosition, MouseButton::Left);

    ASSERT_TRUE(probe.entityPick().kind.has_value());

    probe.pressSelect(
        VoxelPosition{.x = -20, .y = 8, .z = -20}, MouseButton::Left);

    EXPECT_FALSE(probe.entityPick().kind.has_value());
    EXPECT_FALSE(probe.isEntitySectionShown());
}

TEST_F(EditorSelectTest, PressSelect_RightLetsGoOfThePick)
{
    foodCells().push_back(kFarPosition);
    probe.pressSelect(kFarPosition, MouseButton::Left);
    probe.pressSelect(kFarPosition, MouseButton::Right);

    EXPECT_FALSE(probe.entityPick().kind.has_value());
}

TEST_F(EditorSelectTest, MoveEntityTo_CarriesTheMarkerAndPushesOneSnapshot)
{
    foodCells().push_back(kFarPosition);
    probe.pressSelect(kFarPosition, MouseButton::Left);
    probe.entityPick().dragging = false;

    const auto countBefore = probe.document.getUndoCount();

    constexpr VoxelPosition nextPosition{.x = 22, .y = 8, .z = 20};

    EXPECT_TRUE(probe.moveEntityTo(nextPosition, false));
    EXPECT_EQ(foodCells().back(), nextPosition);
    EXPECT_EQ(probe.entityPick().position, nextPosition);
    EXPECT_EQ(probe.document.getUndoCount(), countBefore + 1);

    constexpr VoxelPosition thirdPosition{.x = 24, .y = 8, .z = 20};

    EXPECT_TRUE(probe.moveEntityTo(thirdPosition, false));
    EXPECT_EQ(probe.document.getUndoCount(), countBefore + 2);

    probe.undo();

    EXPECT_EQ(foodCells().back(), nextPosition);
}

TEST_F(EditorSelectTest, MoveEntityTo_KeepsOneSnapshotForAWholeDrag)
{
    foodCells().push_back(kFarPosition);
    probe.pressSelect(kFarPosition, MouseButton::Left);

    ASSERT_TRUE(probe.entityPick().dragging);

    const auto countBefore = probe.document.getUndoCount();

    EXPECT_TRUE(
        probe.moveEntityTo(
            VoxelPosition{.x = 22, .y = 8, .z = 20}, false));
    EXPECT_TRUE(
        probe.moveEntityTo(
            VoxelPosition{.x = 24, .y = 8, .z = 20}, false));
    EXPECT_EQ(probe.document.getUndoCount(), countBefore + 1);

    probe.undo();

    EXPECT_EQ(foodCells().back(), kFarPosition);
}

TEST_F(EditorSelectTest, MoveEntityTo_RefusesACubeAnotherMarkerHolds)
{
    constexpr VoxelPosition otherPosition{.x = 22, .y = 8, .z = 20};

    foodCells().push_back(kFarPosition);
    foodCells().push_back(otherPosition);
    probe.pressSelect(kFarPosition, MouseButton::Left);

    EXPECT_FALSE(probe.moveEntityTo(otherPosition, false));
    EXPECT_EQ(foodCells().front(), kFarPosition);
}

TEST_F(EditorSelectTest, MoveEntityTo_CarriesTheLampWithItsTint)
{
    const antwika::gfx::Color tintColor{
        .red = 200, .green = 100, .blue = 50, .alpha = 255};

    probe.document.map.lamps.push_back(
        antwika::light::Lamp{
            .position = kFarPosition, .tintColor = tintColor});
    probe.pressSelect(kFarPosition, MouseButton::Left);

    constexpr VoxelPosition nextPosition{.x = 24, .y = 8, .z = 24};

    EXPECT_TRUE(probe.moveEntityTo(nextPosition, false));
    ASSERT_EQ(probe.document.map.lamps.size(), 1U);
    EXPECT_EQ(probe.document.map.lamps.front().position, nextPosition);
    EXPECT_EQ(probe.document.map.lamps.front().tintColor, tintColor);
}

TEST_F(EditorSelectTest, RemoveEntityPick_TakesTheChosenEntityAway)
{
    foodCells().push_back(kFarPosition);
    probe.pressSelect(kFarPosition, MouseButton::Left);

    const auto countBefore = probe.document.getUndoCount();

    EXPECT_TRUE(
        probe.consumeWidgets(
            antwika::ui::Interactions{
                .activatedWidget = antwika::editor::kEntityRemoveWidget}));
    EXPECT_TRUE(foodCells().empty());
    EXPECT_FALSE(probe.entityPick().kind.has_value());
    EXPECT_EQ(probe.document.getUndoCount(), countBefore + 1);

    probe.undo();

    EXPECT_EQ(foodCells().size(), 1U);
}

TEST_F(EditorSelectTest, RemoveEntityPick_LeavesThePlayerCharacterStanding)
{
    const auto countCharacters = probe.document.map.characters.size();

    ASSERT_GT(countCharacters, 0U);

    const auto stoodCell = antwika::voxel::VoxelPosition{.x = 0, .y = 0, .z = 0};

    probe.entityPick().kind = EntityKind::Character;
    probe.entityPick().position = stoodCell;
    probe.entityPick().characterIndex = 0;

    ASSERT_TRUE(probe.document.map.characters.at(0).player);

    probe.removeEntityPick();

    EXPECT_EQ(probe.document.map.characters.size(), countCharacters);
}

TEST_F(EditorSelectTest, DeleteChord_TakesThePickWithTheKeyboard)
{
    foodCells().push_back(kFarPosition);
    probe.pressSelect(kFarPosition, MouseButton::Left);
    probe.keyPressed(
        antwika::input::KeyPressed{.key = antwika::input::Key::Delete});

    EXPECT_TRUE(foodCells().empty());
    EXPECT_FALSE(probe.entityPick().kind.has_value());
}

TEST_F(EditorSelectTest, PressTool_LetsGoOfThePickOnTheWayOut)
{
    foodCells().push_back(kFarPosition);
    probe.pressSelect(kFarPosition, MouseButton::Left);
    probe.pressTool(ToolButton::StoneCube);

    EXPECT_FALSE(probe.entityPick().kind.has_value());
}
