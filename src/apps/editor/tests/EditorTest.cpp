#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>

#include <antwika/gfx/NullBackend.hpp>
#include <antwika/input/NullInputBackend.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/map/MapFile.hpp>
#include <antwika/testing/ScratchDirectory.hpp>
#include <antwika/tile/Transitions.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxelmap/Voxel.hpp>

#include "antwika/editor/Editor.hpp"

using antwika::editor::Editor;
using antwika::gfx::NullBackend;
using antwika::input::NullInputBackend;
using antwika::log::Level;
using antwika::log::mocks::MockLogger;
using antwika::testing::ScratchDirectory;
using ::testing::_;
using ::testing::HasSubstr;
using ::testing::NiceMock;

namespace
{

    constexpr std::string_view kMissingMapPath =
        "assets/maps/no-such-map.json";

    class EditorTest : public ::testing::Test
    {
    protected:
        NiceMock<MockLogger> logger;
        NullBackend backend{logger};
        NullInputBackend inputs{logger};
    };

}

TEST_F(EditorTest, Editor_OpensAgainstABackendItIsHanded)
{
    EXPECT_NO_THROW({
        const Editor editor(
            logger, backend, inputs, std::string(kMissingMapPath));
    });
}

TEST_F(EditorTest, Editor_StartsFromTheBuiltInMapWhenThereIsNoneToLoad)
{
    EXPECT_CALL(logger, log(_, _)).Times(::testing::AnyNumber());
    EXPECT_CALL(
        logger,
        log(Level::Info, HasSubstr("starting from the built-in one")));

    const Editor editor(
        logger, backend, inputs, std::string(kMissingMapPath));
}

TEST_F(EditorTest, Editor_OpensStraightIntoPlayWhenItIsAskedTo)
{
    EXPECT_NO_THROW({
        const Editor editor(
            logger, backend, inputs, std::string(kMissingMapPath), true);
    });
}

TEST_F(EditorTest, Editor_OpensAMapWhoseTilesTransitionIntoOneAnother)
{
    const ScratchDirectory scratch("editor-transitions");
    const auto mapPath = scratch.pathIn("transitions.json");

    antwika::map::Map drawnMap;
    drawnMap.tilemap = antwika::tilemap::getDefaultTilemap();
    drawnMap.settings.cornersJoined = true;
    drawnMap.transitions.push_back(
        antwika::tile::TransitionTile{
            .fromTile = {.atlas = antwika::tilemap::Atlas::Wall,
                         .index = 0},
            .toTile = {.atlas = antwika::tilemap::Atlas::Wall,
                       .index = 1},
            .maskTile = {.atlas = antwika::tilemap::Atlas::Wall,
                         .index = 2},
            .outputTile = {.atlas = antwika::tilemap::Atlas::Wall,
                           .index = 3}});

    antwika::map::saveMap(mapPath, drawnMap);

    EXPECT_NO_THROW({
        const Editor editor(logger, backend, inputs, mapPath);
    });
}

TEST_F(EditorTest, Editor_StandsTheCharactersOnTheWorldItJustBuilt)
{
    const ScratchDirectory scratch("editor-spawn");
    const auto mapPath = scratch.pathIn("spawn.json");

    antwika::map::Map drawnMap;
    drawnMap.tilemap = antwika::tilemap::getDefaultTilemap();
    drawnMap.voxels = antwika::voxel::getExpandCubesToVoxels(
        antwika::voxelmap::getDemoCells());
    drawnMap.spawnCubePosition =
        antwika::voxel::VoxelPosition{.x = 1, .y = 1, .z = 1};

    antwika::map::saveMap(mapPath, drawnMap);

    const Editor editor(logger, backend, inputs, mapPath);
    const auto playerPosition = editor.playerStandsAt();

    EXPECT_FLOAT_EQ(playerPosition.x, 1.0F);
    EXPECT_FLOAT_EQ(playerPosition.z, 1.0F);
}
