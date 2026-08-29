#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <string_view>

#include <antwika/gfx/NullBackend.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/NullInputBackend.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/map/Marker.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxelmap/Voxel.hpp>

#include "antwika/editor/Editor.hpp"
#include "antwika/editor/fakes/EditorProbe.hpp"

using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::log::mocks::MockLogger;
using antwika::map::Marker;
using antwika::voxel::VoxelPosition;
using ::testing::NiceMock;

namespace
{

    constexpr std::string_view kMissingMapPath =
        "assets/maps/no-such-map.json";

    constexpr VoxelPosition kPadPosition{.x = 2, .y = 0, .z = 0};

    class PlayStateTest : public ::testing::Test
    {
    protected:
        [[nodiscard]] std::vector<VoxelPosition> &checkpointCells()
        {
            return probe.document.map.markers.positionsOf(
                Marker::Checkpoint);
        }

        NiceMock<MockLogger> logger;
        antwika::gfx::NullBackend backend{logger};
        antwika::input::NullInputBackend inputs{logger};
        antwika::editor::Editor editor{
            logger, backend, inputs, std::string(kMissingMapPath)};
        antwika::editor::fakes::EditorProbe probe{editor};
    };

}

TEST_F(PlayStateTest, EscapeAfterPlay_BringsBackWhatThePlaySessionTook)
{
    checkpointCells().push_back(kPadPosition);
    probe.document.map.voxels[kPadPosition] = {};

    const auto voxelsBefore = probe.document.map.voxels;

    probe.keyPressed(KeyPressed{.key = Key::F5});

    ASSERT_TRUE(editor.isPlaying());

    checkpointCells().clear();
    probe.document.map.voxels.erase(kPadPosition);

    probe.keyPressed(KeyPressed{.key = Key::Escape});

    EXPECT_FALSE(editor.isPlaying());
    EXPECT_EQ(checkpointCells().size(), 1U);
    EXPECT_EQ(checkpointCells().front(), kPadPosition);
    EXPECT_EQ(probe.document.map.voxels, voxelsBefore);
}

TEST_F(PlayStateTest, PushUndo_LeavesTheHistoryAloneWhilePlaying)
{
    const auto countBefore = probe.document.getUndoCount();

    probe.keyPressed(KeyPressed{.key = Key::F5});

    ASSERT_TRUE(editor.isPlaying());

    probe.pushUndo();

    EXPECT_EQ(probe.document.getUndoCount(), countBefore);

    probe.keyPressed(KeyPressed{.key = Key::Escape});

    EXPECT_EQ(probe.document.getUndoCount(), countBefore);
}

TEST_F(PlayStateTest, EscapeAfterPlay_KeepsACleanMapUnsoiled)
{
    ASSERT_FALSE(probe.document.isDirty());

    probe.keyPressed(KeyPressed{.key = Key::F5});
    probe.document.map.voxels.erase(probe.document.map.voxels.begin());
    probe.keyPressed(KeyPressed{.key = Key::Escape});

    EXPECT_FALSE(probe.document.isDirty());
}
