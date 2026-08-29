#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <string_view>

#include <antwika/gfx/NullBackend.hpp>
#include <antwika/input/NullInputBackend.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/voxel/VoxelMaterial.hpp>

#include "antwika/editor/Editor.hpp"
#include "antwika/editor/fakes/EditorProbe.hpp"
#include "antwika/editor/ui/MenuBar.hpp"
#include "antwika/editor/ui/ToolPanel.hpp"

using antwika::editor::MenuItem;
using antwika::editor::Tool;
using antwika::editor::ToolButton;
using antwika::log::mocks::MockLogger;
using antwika::voxel::Kind;
using ::testing::NiceMock;

namespace
{

    constexpr std::string_view kMissingMapPath =
        "assets/maps/no-such-map.json";

    class EditorMenusTest : public ::testing::Test
    {
    protected:
        NiceMock<MockLogger> logger;
        antwika::gfx::NullBackend backend{logger};
        antwika::input::NullInputBackend inputs{logger};
        antwika::editor::Editor editor{
            logger, backend, inputs, std::string(kMissingMapPath)};
        antwika::editor::fakes::EditorProbe probe{editor};
    };

}

TEST_F(EditorMenusTest, GameLighting_TurnsTheMapsOwnLightingOnAndOff)
{
    const auto lightingBefore = probe.document.map.settings.lighting;

    probe.onMenuItem(MenuItem::GameLighting);

    EXPECT_NE(probe.document.map.settings.lighting, lightingBefore);
    EXPECT_EQ(
        probe.isChecked(MenuItem::GameLighting),
        probe.document.map.settings.lighting);
}

TEST_F(EditorMenusTest, EditorLighting_TurnsTheEditorsOwnLightingOnAndOff)
{
    const auto lightingBefore = probe.preferences().lighting;

    probe.onMenuItem(MenuItem::EditorLighting);

    EXPECT_NE(probe.preferences().lighting, lightingBefore);
    EXPECT_EQ(
        probe.isChecked(MenuItem::EditorLighting),
        probe.preferences().lighting);
}

TEST_F(EditorMenusTest, Lighting_LeavesTheOtherMenusLightingWhereItStood)
{
    const auto gameBefore = probe.document.map.settings.lighting;
    const auto editorBefore = probe.preferences().lighting;

    probe.onMenuItem(MenuItem::GameLighting);

    EXPECT_EQ(probe.preferences().lighting, editorBefore);

    probe.onMenuItem(MenuItem::EditorLighting);
    probe.onMenuItem(MenuItem::EditorLighting);

    EXPECT_NE(probe.document.map.settings.lighting, gameBefore);
    EXPECT_EQ(probe.preferences().lighting, editorBefore);
}

TEST_F(EditorMenusTest, FreeLook_TurnsTheFreeCameraOnAndOffFromTheViewMenu)
{
    EXPECT_FALSE(probe.cameraRig().freeLook);

    probe.onMenuItem(MenuItem::FreeLook);

    EXPECT_TRUE(probe.cameraRig().freeLook);
    EXPECT_TRUE(probe.isChecked(MenuItem::FreeLook));

    probe.onMenuItem(MenuItem::FreeLook);

    EXPECT_FALSE(probe.cameraRig().freeLook);
    EXPECT_FALSE(probe.isChecked(MenuItem::FreeLook));
}

TEST_F(EditorMenusTest, PressTool_TakesUpBothTheToolAndTheKindACubeButtonNames)
{
    probe.pressTool(ToolButton::WaterCube);

    EXPECT_EQ(probe.preferences().tool, Tool::Brush);
    EXPECT_EQ(probe.preferences().kind, Kind::Water);
    EXPECT_TRUE(probe.isToolButtonActive(ToolButton::WaterCube));
    EXPECT_FALSE(probe.isToolButtonActive(ToolButton::StoneCube));

    probe.pressTool(ToolButton::RampCube);

    EXPECT_EQ(probe.preferences().kind, Kind::Ramp);
    EXPECT_TRUE(probe.isToolButtonActive(ToolButton::RampCube));
}

TEST_F(EditorMenusTest, PressTool_LeavesTheChosenKindAloneForToolsThatNameNone)
{
    probe.pressTool(ToolButton::WaterCube);
    probe.pressTool(ToolButton::Rubber);

    EXPECT_EQ(probe.preferences().tool, Tool::Eraser);
    EXPECT_EQ(probe.preferences().kind, Kind::Water);
    EXPECT_TRUE(probe.isToolButtonActive(ToolButton::Rubber));
}
