#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/IMesh.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockMesh.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/map/MapFile.hpp>
#include <antwika/solver/VoxelWeave.hpp>
#include <antwika/tilemap/AtlasLayout.hpp>
#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/Voxels.hpp>
#include <antwika/voxelmap/Voxel.hpp>

#include "antwika/render/WorldMeshes.hpp"

using antwika::gfx::Bitmap;
using antwika::gfx::IMesh;
using antwika::gfx::Size;
using antwika::gfx::mocks::MockMesh;
using antwika::gfx::mocks::MockRenderer;
using antwika::map::Map;
using antwika::render::WorldMeshes;
using antwika::solver::CornerSeams;
using antwika::voxel::VoxelCell;
using antwika::voxel::voxelsOf;
using antwika::voxelmap::FaceRef;
using ::testing::AtLeast;
using ::testing::NiceMock;

namespace
{
    [[nodiscard]] Bitmap sheetOf(const Size tileSize)
    {
        const auto size = antwika::tilemap::getAtlasSize(tileSize);

        return Bitmap{
            .size = size,
            .pixels = std::vector<std::uint8_t>(
                static_cast<std::size_t>(size.width)
                    * static_cast<std::size_t>(size.height)
                    * antwika::gfx::kBytesPerPixel,
                0)};
    }

    [[nodiscard]] std::array<Bitmap, 2> getBothSheets()
    {
        return {
            sheetOf(antwika::tilemap::kWallTileSize),
            sheetOf(antwika::tilemap::kFloorTileSize)};
    }

    void handsOutMeshes(NiceMock<MockRenderer> &renderer)
    {
        ON_CALL(renderer, createMesh)
            .WillByDefault(
                []
                {
                    return std::unique_ptr<IMesh>{
                        std::make_unique<NiceMock<MockMesh>>()};
                });
    }
}

TEST(WorldMeshesTest, Rebuild_LaysAMeshForTheCubesItIsGiven)
{
    NiceMock<MockRenderer> renderer;
    handsOutMeshes(renderer);
    WorldMeshes meshes;
    Map drawnMap;

    drawnMap.voxels = voxelsOf({VoxelCell{}});

    meshes.rebuild(
        renderer,
        drawnMap,
        drawnMap.voxels,
        CornerSeams::Ignored,
        getBothSheets(),
        0);

    EXPECT_FALSE(meshes.getFaces().empty());
    EXPECT_EQ(meshes.getDrawnAs().size(), meshes.getFaces().size());
    EXPECT_EQ(meshes.getCells().size(), 1U);
}

TEST(WorldMeshesTest, Rebuild_KeepsTheTilesItSolvedForTheSameCubes)
{
    NiceMock<MockRenderer> renderer;
    handsOutMeshes(renderer);
    WorldMeshes meshes;
    Map drawnMap;

    drawnMap.voxels = voxelsOf({VoxelCell{}});

    meshes.rebuild(
        renderer,
        drawnMap,
        drawnMap.voxels,
        CornerSeams::Ignored,
        getBothSheets(),
        0);

    const auto solvedTiles = meshes.getSolvedTiles();

    meshes.rebuild(
        renderer,
        drawnMap,
        drawnMap.voxels,
        CornerSeams::Ignored,
        getBothSheets(),
        0);

    EXPECT_EQ(meshes.getSolvedTiles(), solvedTiles);
}

TEST(WorldMeshesTest, Rebuild_SolvesAfreshForCubesThatHaveChanged)
{
    NiceMock<MockRenderer> renderer;
    handsOutMeshes(renderer);
    WorldMeshes meshes;
    Map drawnMap;

    drawnMap.voxels = voxelsOf({VoxelCell{}});

    meshes.rebuild(
        renderer,
        drawnMap,
        drawnMap.voxels,
        CornerSeams::Ignored,
        getBothSheets(),
        0);

    const auto faceCount = meshes.getFaces().size();

    drawnMap.voxels = voxelsOf(
        {VoxelCell{}, VoxelCell{.position = {.x = 1}}});

    meshes.rebuild(
        renderer,
        drawnMap,
        drawnMap.voxels,
        CornerSeams::Ignored,
        getBothSheets(),
        0);

    EXPECT_NE(meshes.getFaces().size(), faceCount);
    EXPECT_EQ(meshes.getDrawnAs().size(), meshes.getFaces().size());
    EXPECT_EQ(meshes.getCells().size(), 2U);
}

TEST(WorldMeshesTest, Rebuild_KeepsTheMeshesItLaidForTheSameCubes)
{
    NiceMock<MockRenderer> renderer;
    handsOutMeshes(renderer);
    WorldMeshes meshes;
    Map drawnMap;

    drawnMap.voxels = voxelsOf({VoxelCell{}});

    meshes.rebuild(
        renderer,
        drawnMap,
        drawnMap.voxels,
        CornerSeams::Ignored,
        getBothSheets(),
        0);

    const auto solidCount = meshes.getSolid().size();

    EXPECT_CALL(renderer, createMesh).Times(0);

    meshes.rebuild(
        renderer,
        drawnMap,
        drawnMap.voxels,
        CornerSeams::Ignored,
        getBothSheets(),
        0);

    EXPECT_EQ(meshes.getSolid().size(), solidCount);
}

TEST(WorldMeshesTest, Rebuild_LaysFreshMeshesForCubesThatHaveChanged)
{
    NiceMock<MockRenderer> renderer;
    handsOutMeshes(renderer);
    WorldMeshes meshes;
    Map drawnMap;

    drawnMap.voxels = voxelsOf({VoxelCell{}});

    meshes.rebuild(
        renderer,
        drawnMap,
        drawnMap.voxels,
        CornerSeams::Ignored,
        getBothSheets(),
        0);

    drawnMap.voxels = voxelsOf(
        {VoxelCell{}, VoxelCell{.position = {.x = 1}}});

    EXPECT_CALL(renderer, createMesh).Times(AtLeast(1));

    meshes.rebuild(
        renderer,
        drawnMap,
        drawnMap.voxels,
        CornerSeams::Ignored,
        getBothSheets(),
        0);
}

TEST(WorldMeshesTest, Rebuild_LaysFreshMeshesWhenACellChangeLeavesTheFacesAlike)
{
    NiceMock<MockRenderer> renderer;
    handsOutMeshes(renderer);
    WorldMeshes meshes;
    Map drawnMap;

    drawnMap.voxels = voxelsOf(
        {VoxelCell{
             .material =
                 {.kind = antwika::voxel::Kind::Ramp,
                  .facing = antwika::voxel::Facing::East}},
         VoxelCell{
             .position = {.y = 1},
             .material = {.kind = antwika::voxel::Kind::Water}},
         VoxelCell{.position = {.y = 2}},
         VoxelCell{.position = {.x = 1, .y = 1}},
         VoxelCell{.position = {.x = -1, .y = 1}},
         VoxelCell{.position = {.y = 1, .z = 1}},
         VoxelCell{.position = {.y = 1, .z = -1}}});

    meshes.rebuild(
        renderer,
        drawnMap,
        drawnMap.voxels,
        CornerSeams::Ignored,
        getBothSheets(),
        0);

    const std::vector<FaceRef> laidFaces = meshes.getFaces();
    const auto laidTiles = meshes.getDrawnAs();

    drawnMap.voxels.erase(antwika::voxel::VoxelPosition{.y = 1});

    EXPECT_CALL(renderer, createMesh).Times(AtLeast(1));

    meshes.rebuild(
        renderer,
        drawnMap,
        drawnMap.voxels,
        CornerSeams::Ignored,
        getBothSheets(),
        0);

    EXPECT_TRUE(
        std::ranges::equal(
            meshes.getFaces(), laidFaces, &FaceRef::isIdenticalTo));
    EXPECT_EQ(meshes.getDrawnAs(), laidTiles);
}
