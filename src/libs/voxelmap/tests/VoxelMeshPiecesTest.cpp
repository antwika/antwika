#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/gfx/MeshBox.hpp>
#include <antwika/gfx/MeshData.hpp>
#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/Voxels.hpp>
#include <antwika/voxelmap/Voxel.hpp>

using antwika::gfx::getMeshBox;
using antwika::voxel::VoxelCell;
using antwika::voxel::VoxelPosition;
using antwika::voxel::Voxels;
using antwika::voxelmap::getDefaultTiles;
using antwika::voxelmap::getMeshRegionOf;
using antwika::voxelmap::getVoxelMesh;
using antwika::voxelmap::getVoxelMeshPieces;
using antwika::voxelmap::kMeshRegionSide;
using antwika::voxelmap::Pass;
using antwika::voxelmap::visibleFacesOf;

namespace
{
    [[nodiscard]] Voxels getColumnsApart(const std::int32_t apart)
    {
        Voxels voxels;

        voxels[VoxelPosition{}] = antwika::voxel::VoxelMaterial{};
        voxels[VoxelPosition{.x = apart, .z = apart}] =
            antwika::voxel::VoxelMaterial{};

        return voxels;
    }
}

TEST(VoxelMeshPiecesTest, MeshRegionOf_FloorsTowardsTheLowerRegion)
{
    EXPECT_EQ(getMeshRegionOf(VoxelPosition{}, 16), VoxelPosition{});
    EXPECT_EQ(
        getMeshRegionOf(VoxelPosition{.x = 15, .y = 15, .z = 15}, 16),
        VoxelPosition{});
    EXPECT_EQ(
        getMeshRegionOf(VoxelPosition{.x = 16}, 16),
        (VoxelPosition{.x = 1}));
}

TEST(VoxelMeshPiecesTest, MeshRegionOf_FloorsBelowTheOriginToo)
{
    EXPECT_EQ(
        getMeshRegionOf(VoxelPosition{.x = -1}, 16),
        (VoxelPosition{.x = -1}));
    EXPECT_EQ(
        getMeshRegionOf(VoxelPosition{.x = -16}, 16),
        (VoxelPosition{.x = -1}));
    EXPECT_EQ(
        getMeshRegionOf(VoxelPosition{.x = -17}, 16),
        (VoxelPosition{.x = -2}));
}

TEST(VoxelMeshPiecesTest, VoxelMeshPieces_HoldsAsManyVerticesAsOneWholeMesh)
{
    const auto voxels = getColumnsApart(kMeshRegionSide * 3);
    const auto faces = visibleFacesOf(voxels);
    const auto tiles = getDefaultTiles(faces);
    const auto wholeMesh = getVoxelMesh(voxels, faces, tiles, Pass::Solid);
    const auto pieces =
        getVoxelMeshPieces(voxels, faces, tiles, Pass::Solid);

    std::size_t vertexCount = 0;

    for (const auto &piece : pieces)
    {
        vertexCount += piece.vertices.size();
    }

    EXPECT_EQ(vertexCount, wholeMesh.vertices.size());
}

TEST(VoxelMeshPiecesTest, VoxelMeshPieces_PartsCubesThatStandRegionsApart)
{
    const auto voxels = getColumnsApart(kMeshRegionSide * 3);
    const auto faces = visibleFacesOf(voxels);
    const auto tiles = getDefaultTiles(faces);
    const auto pieces =
        getVoxelMeshPieces(voxels, faces, tiles, Pass::Solid);

    ASSERT_EQ(pieces.size(), 2U);

    const auto oneBox = getMeshBox(pieces.front());
    const auto otherBox = getMeshBox(pieces.back());

    EXPECT_LT(oneBox.highPosition.x, otherBox.lowPosition.x);
}

TEST(VoxelMeshPiecesTest, VoxelMeshPieces_KeepsCubesOfOneRegionInOnePiece)
{
    Voxels voxels;

    for (std::int32_t x = 0; x < 4; ++x)
    {
        voxels[VoxelPosition{.x = x}] = antwika::voxel::VoxelMaterial{};
    }

    const auto faces = visibleFacesOf(voxels);
    const auto tiles = getDefaultTiles(faces);
    const auto pieces =
        getVoxelMeshPieces(voxels, faces, tiles, Pass::Solid);

    EXPECT_EQ(pieces.size(), 1U);
}

TEST(VoxelMeshPiecesTest, VoxelMeshPieces_LeavesNothingForAWorldWithNoCubes)
{
    EXPECT_TRUE(
        getVoxelMeshPieces(Voxels{}, {}, {}, Pass::Solid).empty());
}
