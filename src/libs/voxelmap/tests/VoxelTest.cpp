#include <gtest/gtest.h>

#include <glm/geometric.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <ranges>
#include <set>
#include <vector>

#include <antwika/tilemap/AtlasLayout.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/MeshData.hpp>
#include <antwika/gfx/SizeF.hpp>

#include <antwika/voxelmap/Voxel.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/tilemap/TileEdges.hpp>

using antwika::tilemap::atlasSize;
using antwika::tilemap::kAtlasColumns;
using antwika::tilemap::kFloorTileSize;
using antwika::tilemap::kWallTileSize;
using antwika::voxel::kVoxelSide;
using antwika::tilemap::tileCoords;
using antwika::voxelmap::cellMiddle;
using antwika::voxelmap::defaultTileIndex;
using antwika::voxelmap::levelOf;
using antwika::voxelmap::topLevel;
using antwika::voxelmap::bottomLevel;
using antwika::voxelmap::demoCells;
using antwika::voxelmap::modelRotation;
using antwika::voxel::VoxelCell;
using antwika::voxel::VoxelMaterial;
using antwika::voxel::VoxelPosition;
using antwika::voxel::voxelsOf;
using antwika::voxel::Voxels;
using antwika::voxelmap::voxelMesh;
using antwika::voxelmap::visibleFacesOf;
using antwika::voxelmap::defaultTiles;
using antwika::gfx::Vec3;

namespace
{
    constexpr float kTolerance = 0.001F;

    constexpr std::size_t kTouchingFaces = 12 - 2;

    constexpr std::size_t kPyramidFaces = 9 + 12 + 9 + 5;

    constexpr std::size_t kFloorFaces = 9 + 9 + 1;

    constexpr std::size_t kWallFaces = 12 + 4;

    [[nodiscard]] std::size_t facesOf(
        const antwika::gfx::MeshData &mesh)
    {
        return mesh.triangleCount() / 2;
    }

    [[nodiscard]] Vec3 turnedBy(
        const antwika::gfx::Mat4 &modelMatrix, const Vec3 point)
    {
        const auto movedPoint =
            modelMatrix * antwika::gfx::Vec4{point, 1.0F};

        return Vec3{movedPoint.x, movedPoint.y, movedPoint.z};
    }
}

TEST(VoxelTest, DemoCells_StacksOneOnAThreeByThreeBase)
{
    const auto voxels = demoCells();

    EXPECT_EQ(voxels.size(), 10U);

    std::size_t base = 0;

    for (const auto &[position, material] : voxels)
    {
        base += position.y == 0 ? 1U : 0U;
    }

    EXPECT_EQ(base, 9U);
    EXPECT_TRUE(voxels.contains(VoxelPosition{.x = 0, .y = 1, .z = 0}));
}

TEST(VoxelTest, VoxelMesh_DrawsAllSixSidesOfALoneVoxel)
{
    const auto mesh = voxelMesh(voxelsOf({VoxelCell{}}));

    EXPECT_EQ(facesOf(mesh), 6U);
    EXPECT_EQ(mesh.vertices.size(), 24U);
    EXPECT_TRUE(mesh.isComplete());
}

TEST(VoxelTest, VoxelMesh_LeavesOutTheFaceTwoVoxelsShare)
{
    const auto mesh = voxelMesh(
        voxelsOf({VoxelCell{}, VoxelCell{.x = 1}}));

    EXPECT_EQ(facesOf(mesh), kTouchingFaces);
}

TEST(VoxelTest, VoxelMesh_KeepsOnlyTheOutsideOfThePyramid)
{
    const auto mesh = voxelMesh(demoCells());

    EXPECT_EQ(facesOf(mesh), kPyramidFaces);
    EXPECT_TRUE(mesh.isComplete());
}

TEST(VoxelTest, VoxelMesh_StandsAPileWhereItsCellsSay)
{
    const auto mesh = voxelMesh(voxelsOf({VoxelCell{.x = 4, .y = 2, .z = 6}}));

    auto lowest = mesh.vertices.front().position;
    auto highest = lowest;

    for (const auto &vertex : mesh.vertices)
    {
        lowest = glm::min(lowest, vertex.position);
        highest = glm::max(highest, vertex.position);
    }

    const auto middle = (lowest + highest) / 2.0F;

    const auto pileMiddle = cellMiddle(VoxelPosition{.x = 4, .y = 2, .z = 6});

    EXPECT_NEAR(middle.x, pileMiddle.x, 1e-5F);
    EXPECT_NEAR(middle.y, pileMiddle.y, 1e-5F);
    EXPECT_NEAR(middle.z, pileMiddle.z, 1e-5F);
}

TEST(VoxelTest, VoxelMesh_LeavesThePileWhereItWasWhenOneIsAdded)
{
    const auto beforeCells = voxelsOf({
        VoxelCell{}, VoxelCell{.x = 1}});
    const auto grownCells = voxelsOf({
        VoxelCell{}, VoxelCell{.x = 1}, VoxelCell{.x = 2}});

    const auto beforeMesh = voxelMesh(beforeCells);
    const auto afterMesh = voxelMesh(grownCells);

    for (const auto &vertex : beforeMesh.vertices)
    {
        const auto match = std::ranges::find_if(
            afterMesh.vertices,
            [&](const antwika::gfx::Vertex3D &other)
            { return other.position == vertex.position; });

        EXPECT_NE(match, afterMesh.vertices.end());
    }
}

TEST(VoxelTest, VoxelMesh_LeavesThePileWhereItWasWhenOneIsTaken)
{
    const auto beforeCells = voxelsOf({
        VoxelCell{}, VoxelCell{.x = 1}, VoxelCell{.x = 2}});
    const auto fewerCells = voxelsOf({
        VoxelCell{}, VoxelCell{.x = 1}});
    const auto beforeMesh = voxelMesh(beforeCells);
    const auto afterMesh = voxelMesh(fewerCells);

    auto lowest = afterMesh.vertices.front().position;

    for (const auto &vertex : afterMesh.vertices)
    {
        lowest = glm::min(lowest, vertex.position);
    }

    auto wasLowest = beforeMesh.vertices.front().position;

    for (const auto &vertex : beforeMesh.vertices)
    {
        wasLowest = glm::min(wasLowest, vertex.position);
    }

    EXPECT_EQ(lowest, wasLowest);
}

TEST(VoxelTest, VoxelMesh_WindsEveryFaceOutward)
{
    const auto mesh = voxelMesh(demoCells());

    for (std::size_t index = 0; index + 2 < mesh.indices.size(); index += 3)
    {
        const auto &first = mesh.vertices[mesh.indices[index]];
        const auto &second = mesh.vertices[mesh.indices[index + 1]];
        const auto &third = mesh.vertices[mesh.indices[index + 2]];

        const auto facing = glm::cross(
            second.position - first.position,
            third.position - first.position);

        EXPECT_GT(glm::dot(facing, first.normal), 0.0F) << index;
    }
}

TEST(VoxelTest, VoxelMesh_GivesEachSideAWayOfItsOwn)
{
    const auto mesh = voxelMesh(voxelsOf({VoxelCell{}}));

    std::set<std::array<std::int32_t, 3>> normals;

    for (const auto &vertex : mesh.vertices)
    {
        normals.insert(
            {static_cast<std::int32_t>(vertex.normal.x),
             static_cast<std::int32_t>(vertex.normal.y),
             static_cast<std::int32_t>(vertex.normal.z)});
    }

    EXPECT_EQ(normals.size(), 6U);
}

TEST(VoxelTest, AtlasSize_MatchesTheSixteenBySixteenGrid)
{
    EXPECT_EQ(atlasSize(kWallTileSize).width, 270U);
    EXPECT_EQ(atlasSize(kWallTileSize).height, 174U);
    EXPECT_EQ(atlasSize(kFloorTileSize).width, 270U);
    EXPECT_EQ(atlasSize(kFloorTileSize).height, 222U);
}

TEST(VoxelTest, TileCoords_StartsTheFirstTileAtTheCorner)
{
    const auto first = tileCoords(0, kFloorTileSize);
    const auto wholeSize = atlasSize(kFloorTileSize);

    EXPECT_FLOAT_EQ(first.originPoint.x, 0.0F);
    EXPECT_FLOAT_EQ(first.originPoint.y, 0.0F);
    EXPECT_FLOAT_EQ(
        first.size.width,
        static_cast<float>(kFloorTileSize.width)
            / static_cast<float>(wholeSize.width));
    EXPECT_FLOAT_EQ(
        first.size.height,
        static_cast<float>(kFloorTileSize.height)
            / static_cast<float>(wholeSize.height));
}

TEST(VoxelTest, TileCoords_StepsOverThePaddingBetweenTiles)
{
    const auto wholeSize = atlasSize(kWallTileSize);
    const auto nextCoords = tileCoords(1, kWallTileSize);
    const auto belowCoords = tileCoords(kAtlasColumns, kWallTileSize);

    EXPECT_FLOAT_EQ(
        nextCoords.originPoint.x,
        static_cast<float>(kWallTileSize.width + 2)
            / static_cast<float>(wholeSize.width));
    EXPECT_FLOAT_EQ(nextCoords.originPoint.y, 0.0F);

    EXPECT_FLOAT_EQ(belowCoords.originPoint.x, 0.0F);
    EXPECT_FLOAT_EQ(
        belowCoords.originPoint.y,
        static_cast<float>(kWallTileSize.height + 2)
            / static_cast<float>(wholeSize.height));
}

TEST(VoxelTest, TileCoords_KeepsEveryTileInsideTheAtlas)
{
    for (std::size_t index = 0; index < 256; ++index)
    {
        const auto tile = tileCoords(index, kFloorTileSize);

        EXPECT_GE(tile.originPoint.x, 0.0F) << index;
        EXPECT_GE(tile.originPoint.y, 0.0F) << index;
        EXPECT_LE(tile.originPoint.x + tile.size.width, 1.0F) << index;
        EXPECT_LE(tile.originPoint.y + tile.size.height, 1.0F) << index;
    }
}

TEST(VoxelTest, DefaultTileIndex_GivesEverySideOfThePileATileOfItsOwn)
{
    std::set<std::size_t> seenIndexes;

    for (const auto &[position, material] : demoCells())
    {
        for (std::size_t side = 0; side < 6; ++side)
        {
            seenIndexes.insert(defaultTileIndex(position, side));
        }
    }

    EXPECT_EQ(seenIndexes.size(), demoCells().size() * 6);
}

TEST(VoxelTest, VoxelMesh_TakesFloorAndWallFacesFromTheirOwnAtlas)
{
    const auto mesh = voxelMesh(demoCells());

    const auto floorHigh =
        static_cast<float>(kFloorTileSize.height)
        / static_cast<float>(atlasSize(kFloorTileSize).height);
    const auto wallHigh =
        static_cast<float>(kWallTileSize.height)
        / static_cast<float>(atlasSize(kWallTileSize).height);

    std::size_t floorFaces = 0;
    std::size_t wallFaces = 0;

    for (std::size_t index = 0; index + 3 < mesh.vertices.size(); index += 4)
    {
        const auto &corner = mesh.vertices[index];
        const auto textureHeight =
            mesh.vertices[index + 3].texCoordinate.y - corner.texCoordinate.y;

        if (corner.normal.y != 0.0F)
        {
            EXPECT_NEAR(-textureHeight, floorHigh, 0.0001F) << index;
            ++floorFaces;
        }
        else
        {
            EXPECT_NEAR(-textureHeight, wallHigh, 0.0001F) << index;
            ++wallFaces;
        }
    }

    EXPECT_EQ(floorFaces, kFloorFaces);
    EXPECT_EQ(wallFaces, kWallFaces);
    EXPECT_EQ(floorFaces + wallFaces, kPyramidFaces);
}

TEST(VoxelTest, VoxelMesh_DrawsNothingForNoVoxelsAtAll)
{
    const auto mesh = voxelMesh({});

    EXPECT_TRUE(mesh.vertices.empty());
    EXPECT_TRUE(mesh.indices.empty());
}

TEST(VoxelTest, ModelRotation_LeavesTheModelWhereItIsWithNoTurn)
{
    const Vec3 corner{1.0F, 2.0F, 3.0F};
    const auto turnedCorner = turnedBy(modelRotation(0.0F, 0.0F), corner);

    EXPECT_NEAR(turnedCorner.x, corner.x, kTolerance);
    EXPECT_NEAR(turnedCorner.y, corner.y, kTolerance);
    EXPECT_NEAR(turnedCorner.z, corner.z, kTolerance);
}

TEST(VoxelTest, ModelRotation_TurnsHalfWayRoundTheUprightAxis)
{
    constexpr auto kHalfTurn = std::numbers::pi_v<float>;

    const auto turnedCell =
        turnedBy(
            modelRotation(kHalfTurn, 0.0F), Vec3{1.0F, 0.0F, 0.0F});

    EXPECT_NEAR(turnedCell.x, -1.0F, kTolerance);
    EXPECT_NEAR(turnedCell.y, 0.0F, kTolerance);
    EXPECT_NEAR(turnedCell.z, 0.0F, kTolerance);
}

TEST(VoxelTest, ModelRotation_KeepsTheUprightAxisUpAcrossTheView)
{
    for (const auto yaw : {0.4F, 0.8F, 1.6F, 3.0F})
    {
        for (const auto pitch : {0.3F, 0.6F, 1.0F})
        {
            const auto upright =
                turnedBy(modelRotation(yaw, pitch), Vec3{0.0F, 1.0F, 0.0F});

            EXPECT_NEAR(upright.x, 0.0F, kTolerance);
        }
    }
}

TEST(VoxelTest, ModelRotation_TipsAboutTheAxisAcrossTheView)
{
    constexpr auto kQuarterTurn =
        std::numbers::pi_v<float> / 2.0F;

    const auto turnedCell =
        turnedBy(
            modelRotation(0.0F, kQuarterTurn),
            Vec3{0.0F, 1.0F, 0.0F});

    EXPECT_NEAR(turnedCell.x, 0.0F, kTolerance);
    EXPECT_NEAR(turnedCell.y, 0.0F, kTolerance);
    EXPECT_NEAR(turnedCell.z, 1.0F, kTolerance);
}

TEST(VoxelTest, LevelOf_ReckonsALevelByHowHighAVoxelStands)
{
    EXPECT_EQ(levelOf(VoxelPosition{.x = 7, .y = 3, .z = -2}), 3);
    EXPECT_EQ(levelOf(VoxelPosition{.y = -4}), -4);
}

TEST(VoxelTest, TopLevel_TakesTheHighestVoxelOfThePile)
{
    EXPECT_EQ(topLevel(demoCells()), 1);
    EXPECT_EQ(bottomLevel(demoCells()), 0);
}

TEST(VoxelTest, TopLevel_ReckonsNoughtForAPileWithNoVoxels)
{
    EXPECT_EQ(topLevel({}), 0);
    EXPECT_EQ(bottomLevel({}), 0);
}

TEST(VoxelTest, TopLevel_TakesTheHighestWhereEveryVoxelIsBelowNought)
{
    const auto belowCells = voxelsOf({
        VoxelCell{.y = -5}, VoxelCell{.y = -2}, VoxelCell{.y = -9}});

    EXPECT_EQ(topLevel(belowCells), -2);
    EXPECT_EQ(bottomLevel(belowCells), -9);
}

TEST(VoxelTest, VisibleFacesOf_KeepsTheFaceASolidTurnsToWater)
{
    using antwika::voxel::Kind;

    const auto voxels = voxelsOf({
        VoxelCell{.x = 0, .y = 0, .z = 0, .kind = Kind::Normal},
        VoxelCell{.x = 1, .y = 0, .z = 0, .kind = Kind::Water}});
    const auto faces = visibleFacesOf(voxels);

    std::size_t solid = 0;
    std::size_t watery = 0;

    for (const auto face : faces)
    {
        (face.cell.kind == Kind::Water ? watery : solid) += 1;
    }

    EXPECT_EQ(solid, 6U);
    EXPECT_EQ(watery, 5U);
}

TEST(VoxelTest, VisibleFacesOf_KeepsTheGroundASolidStandsOn)
{
    using antwika::voxel::Kind;
    using antwika::voxelmap::faceNormal;

    const auto voxels = voxelsOf({
        VoxelCell{.x = 0, .y = 0, .z = 0, .kind = Kind::Normal},
        VoxelCell{.x = 0, .y = 1, .z = 0, .kind = Kind::Normal}});
    const auto faces = visibleFacesOf(voxels);

    std::size_t lowestTop = 0;

    for (const auto face : faces)
    {
        if (face.cell.y == 0 && faceNormal(face.side).y > 0.0F)
        {
            lowestTop += 1;
        }
    }

    EXPECT_EQ(lowestTop, 1U);
}

TEST(VoxelTest, VisibleFacesOf_KeepsNoGroundUnderWaterOrAStair)
{
    using antwika::voxel::Kind;
    using antwika::voxelmap::faceNormal;

    const auto voxels = voxelsOf({
        VoxelCell{.x = 0, .y = 0, .z = 0, .kind = Kind::Water},
        VoxelCell{.x = 0, .y = 1, .z = 0, .kind = Kind::Normal},
        VoxelCell{.x = 4, .y = 0, .z = 0, .kind = Kind::Normal},
        VoxelCell{.x = 4, .y = 1, .z = 0, .kind = Kind::Ramp}});
    const auto faces = visibleFacesOf(voxels);

    std::size_t coveredCount = 0;

    for (const auto face : faces)
    {
        if (face.cell.y == 0 && faceNormal(face.side).y > 0.0F)
        {
            coveredCount += 1;
        }
    }

    EXPECT_EQ(coveredCount, 0U);
}

TEST(VoxelTest, VisibleFacesOf_KeepsEverySideOfARampCube)
{
    using antwika::voxel::Kind;

    const auto voxels = voxelsOf({
        VoxelCell{.x = 0, .y = 0, .z = 0, .kind = Kind::Ramp},
        VoxelCell{.x = 1, .y = 0, .z = 0, .kind = Kind::Normal}});

    std::size_t flight = 0;

    for (const auto face : visibleFacesOf(voxels))
    {
        if (face.cell.kind == Kind::Ramp)
        {
            ++flight;
        }
    }

    EXPECT_EQ(flight, 5U);
}

TEST(VoxelTest, StairUvRect_TakesWholePixelsOfATreadAndARiser)
{
    using antwika::voxel::stairQuads;
    using antwika::voxelmap::stairUvRect;

    const antwika::gfx::RectF floorRect(
        antwika::gfx::PointF{0.0F, 0.0F},
        antwika::gfx::SizeF{15.0F, 12.0F});
    const antwika::gfx::RectF wallRect(
        antwika::gfx::PointF{0.0F, 0.0F},
        antwika::gfx::SizeF{15.0F, 9.0F});

    for (const auto &quad : stairQuads(VoxelPosition{.z = -1}))
    {
        const auto isFloorFace =
            antwika::voxelmap::faceNormal(quad.side).y != 0.0F;
        const auto part =
            stairUvRect(isFloorFace ? floorRect : wallRect, quad);

        for (const auto value :
             {part.originPoint.x, part.originPoint.y, part.size.width,
              part.size.height})
        {
            EXPECT_NEAR(value, std::round(value), 1e-3F);
        }

        EXPECT_GE(part.originPoint.x, -1e-3F);
        EXPECT_GE(part.originPoint.y, -1e-3F);
    }
}

TEST(VoxelTest, StairUvRect_CutsATileIntoAsManyBandsAsThereAreSteps)
{
    using antwika::voxel::kStepsPerVoxel;
    using antwika::voxel::stairQuads;
    using antwika::voxelmap::stairUvRect;

    const antwika::gfx::RectF floorRect(
        antwika::gfx::PointF{0.0F, 0.0F},
        antwika::gfx::SizeF{15.0F, 12.0F});

    std::set<float> bands;
    auto coveredArea = 0.0F;

    for (const auto &quad : stairQuads(VoxelPosition{.z = -1}))
    {
        if (antwika::voxelmap::faceNormal(quad.side).y <= 0.5F)
        {
            continue;
        }

        const auto part = stairUvRect(floorRect, quad);

        bands.insert(part.originPoint.y);
        coveredArea += part.size.height;

        EXPECT_NEAR(part.size.width, floorRect.size.width, 1e-3F);
    }

    EXPECT_EQ(bands.size(), kStepsPerVoxel);
    EXPECT_NEAR(coveredArea, floorRect.size.height, 1e-3F);
}

TEST(VoxelTest, VoxelMesh_DrawsARampAsAFlightOfSteps)
{
    using antwika::voxel::Kind;
    using antwika::voxelmap::Pass;

    const auto voxels = voxelsOf({
        VoxelCell{.x = 0, .y = 0, .z = 0, .kind = Kind::Ramp},
        VoxelCell{.x = 1, .y = 0, .z = 0, .kind = Kind::Normal}});
    const auto faces = visibleFacesOf(voxels);
    const auto mesh =
        voxelMesh(voxels, defaultTiles(faces), Pass::Solid);

    std::set<float> tops;

    for (const auto &vertex : mesh.vertices)
    {
        if (vertex.position.x < 0.99F && vertex.normal.y > 0.5F)
        {
            tops.insert(vertex.position.y);
        }

        EXPECT_NEAR(
            std::abs(vertex.normal.x) + std::abs(vertex.normal.y)
                + std::abs(vertex.normal.z),
            1.0F,
            1e-4F);
    }

    EXPECT_EQ(tops.size(), antwika::voxel::kStepsPerVoxel);
}

TEST(VoxelTest, VoxelMesh_LaysTheWateryFacesInTheirOwnPass)
{
    using antwika::voxel::Kind;
    using antwika::voxelmap::Pass;

    const auto voxels = voxelsOf({
        VoxelCell{.x = 0, .y = 0, .z = 0, .kind = Kind::Normal},
        VoxelCell{.x = 2, .y = 0, .z = 0, .kind = Kind::Water}});
    const auto faces = visibleFacesOf(voxels);
    const auto tiles = defaultTiles(faces);
    const auto solid = voxelMesh(voxels, tiles, Pass::Solid);
    const auto watery = voxelMesh(voxels, tiles, Pass::Water);
    const auto wholeMesh = voxelMesh(voxels, tiles);

    EXPECT_EQ(
        solid.vertices.size() + watery.vertices.size(),
        static_cast<std::size_t>(faces.size()) * 4U);
    EXPECT_EQ(solid.vertices.size(), wholeMesh.vertices.size());

    for (const auto &vertex : watery.vertices)
    {
        EXPECT_EQ(vertex.color.alpha, antwika::voxelmap::kWaterAlpha);
    }

    for (const auto &vertex : solid.vertices)
    {
        EXPECT_EQ(vertex.color.alpha, 255);
    }
}

TEST(VoxelTest, VisibleFacesOf_FillsARampWithAnotherStandingOnIt)
{
    using antwika::voxel::Kind;

    const auto voxels = voxelsOf({
        VoxelCell{.x = 0, .y = 0, .z = 0, .kind = Kind::Ramp},
        VoxelCell{.x = 0, .y = 1, .z = 0, .kind = Kind::Ramp}});

    std::size_t belowCells = 0;

    for (const auto face : visibleFacesOf(voxels))
    {
        if (face.cell.y == 0)
        {
            ++belowCells;
            EXPECT_LE(antwika::voxelmap::faceNormal(face.side).y, 0.0F);
        }
    }

    EXPECT_EQ(belowCells, 5U);
}

TEST(VoxelTest, VisibleFacesOf_LetsARampHideTheSideOfAnother)
{
    using antwika::voxel::Kind;

    const auto voxels = voxelsOf({
        VoxelCell{.x = 0, .y = 0, .z = 0, .kind = Kind::Ramp},
        VoxelCell{.x = 0, .y = 0, .z = 1, .kind = Kind::Ramp}});

    for (const auto face : visibleFacesOf(voxels))
    {
        const auto way = antwika::voxelmap::faceNormal(face.side).z;

        EXPECT_FALSE(face.cell.z == 0 && way > 0.0F);
        EXPECT_FALSE(face.cell.z == 1 && way < 0.0F);
    }
}

TEST(VoxelTest, VoxelMesh_LeavesARampUnderAnotherWhole)
{
    using antwika::voxel::Kind;
    using antwika::voxelmap::Pass;

    const auto voxels = voxelsOf({
        VoxelCell{.x = 0, .y = 0, .z = 0, .kind = Kind::Ramp},
        VoxelCell{.x = 0, .y = 1, .z = 0, .kind = Kind::Ramp}});
    const auto faces = visibleFacesOf(voxels);
    const auto mesh =
        voxelMesh(voxels, defaultTiles(faces), Pass::Solid);

    const auto lowest = cellMiddle(VoxelPosition{});

    for (const auto &vertex : mesh.vertices)
    {
        if (vertex.position.y > lowest.y)
        {
            continue;
        }

        EXPECT_NEAR(
            vertex.position.y, lowest.y - (kVoxelSide / 2.0F), 1e-4F);
    }
}

TEST(VoxelTest, VisibleFacesOf_CarriesTheClimbOfTheFlightItBelongsTo)
{
    using antwika::voxel::Kind;

    const auto voxels = voxelsOf({
        VoxelCell{.x = 0, .y = 0, .z = 0, .kind = Kind::Ramp},
        VoxelCell{.x = 1, .y = 0, .z = 0, .kind = Kind::Normal}});

    for (const auto face : visibleFacesOf(voxels))
    {
        if (face.cell.kind == Kind::Ramp)
        {
            EXPECT_EQ(face.climbPosition.x, 1);
            EXPECT_EQ(face.climbPosition.z, 0);
        }
        else
        {
            EXPECT_EQ(face.climbPosition, VoxelPosition{});
        }
    }
}

TEST(VoxelTest, UsesMirroredUv_ReadsBackwardsOnlyWhereNoRimCanSay)
{
    using antwika::voxel::cubeVoxels;
    using antwika::voxel::Kind;
    using antwika::voxelmap::usesMirroredUv;

    Voxels voxels;

    for (std::int32_t x = -4; x <= 6; ++x)
    {
        for (std::int32_t z = -4; z <= 6; ++z)
        {
            voxels[VoxelPosition{.x = x, .y = -1, .z = z}] =
                VoxelMaterial{};
        }
    }

    for (const auto &[position, material] :
         cubeVoxels(VoxelPosition{}, Kind::Ramp, VoxelPosition{.x = 1}))
    {
        voxels[position] = material;
    }

    std::size_t backwards = 0;
    std::size_t wholeCount = 0;

    for (const auto face : visibleFacesOf(voxels))
    {
        if (face.cell.kind != Kind::Ramp)
        {
            continue;
        }

        if (usesMirroredUv(voxels, face))
        {
            ++backwards;

            EXPECT_EQ(antwika::voxelmap::faceNormal(face.side).y, 0.0F);
        }
        else
        {
            ++wholeCount;
        }
    }

    EXPECT_GT(backwards, 0U);
    EXPECT_GT(wholeCount, backwards);
}

TEST(VoxelTest, UsesMirroredUv_LeavesAFaceOfNoFlightAlone)
{
    using antwika::voxelmap::usesMirroredUv;

    const auto voxels = voxelsOf({VoxelCell{}});

    for (const auto face : visibleFacesOf(voxels))
    {
        EXPECT_FALSE(usesMirroredUv(voxels, face));
    }
}

TEST(VoxelTest, UsesMirroredUv_KeepsTheNearSideWhicheverWayAFlightClimbs)
{
    using antwika::voxel::cubeVoxels;
    using antwika::voxelmap::faceNormal;
    using antwika::voxel::Kind;
    using antwika::voxelmap::usesMirroredUv;

    const std::array<VoxelPosition, 4> wayPositions{
        VoxelPosition{.x = 1},
        VoxelPosition{.x = -1},
        VoxelPosition{.z = 1},
        VoxelPosition{.z = -1}};

    for (const auto way : wayPositions)
    {
        Voxels voxels;

        for (std::int32_t x = -5; x <= 7; ++x)
        {
            for (std::int32_t z = -5; z <= 7; ++z)
            {
                voxels[VoxelPosition{.x = x, .y = -1, .z = z}] =
                VoxelMaterial{};
            }
        }

        for (const auto &[position, material] :
             cubeVoxels(VoxelPosition{}, Kind::Ramp, way))
        {
            voxels[position] = material;
        }

        std::size_t backwards = 0;

        for (const auto face : visibleFacesOf(voxels))
        {
            if (face.cell.kind != Kind::Ramp
                || !usesMirroredUv(voxels, face))
            {
                continue;
            }

            ++backwards;

            const auto normal = faceNormal(face.side);

            EXPECT_LT(normal.x + normal.z, 0.0F);
        }

        EXPECT_EQ(backwards, 1U);
    }
}

TEST(VoxelTest, VisibleFacesOf_CarriesTheLevelOfTheFlightAFaceStandsIn)
{
    using antwika::voxel::cubeVoxels;
    using antwika::voxel::stairHalfOf;
    using antwika::voxel::Kind;

    Voxels voxels;

    for (const auto &[position, material] :
         cubeVoxels(VoxelPosition{}, Kind::Ramp, VoxelPosition{.x = 1}))
    {
        voxels[position] = material;
    }

    for (const auto face : visibleFacesOf(voxels))
    {
        EXPECT_EQ(face.levelHalf, stairHalfOf(voxels, face.cell.position()));
    }
}

TEST(VoxelTest, VisibleFacesOf_HidesWhatStandsAgainstTheHeadOfAFlight)
{
    using antwika::voxel::Facing;
    using antwika::voxelmap::faceNormal;
    using antwika::voxel::Kind;

    const auto voxels = voxelsOf({
        VoxelCell{.x = 0, .y = 0, .z = 0, .kind = Kind::Normal},
        VoxelCell{
            .x = 0,
            .y = 0,
            .z = 1,
            .kind = Kind::Ramp,
            .facing = Facing::North}});

    for (const auto face : visibleFacesOf(voxels))
    {
        if (face.cell.kind == Kind::Normal)
        {
            EXPECT_LE(faceNormal(face.side).z, 0.0F);
        }
    }
}

TEST(VoxelTest, VisibleFacesOf_KeepsWhatOnlyTheFootOfAFlightStandsAt)
{
    using antwika::voxel::Facing;
    using antwika::voxelmap::faceNormal;
    using antwika::voxel::Kind;

    const auto voxels = voxelsOf({
        VoxelCell{.x = 0, .y = 0, .z = 0, .kind = Kind::Normal},
        VoxelCell{
            .x = 0,
            .y = 0,
            .z = 1,
            .kind = Kind::Ramp,
            .facing = Facing::South}});

    std::size_t towardsCount = 0;

    for (const auto face : visibleFacesOf(voxels))
    {
        if (face.cell.kind == Kind::Normal
            && faceNormal(face.side).z > 0.0F)
        {
            towardsCount += 1;
        }
    }

    EXPECT_EQ(towardsCount, 1U);
}

TEST(VoxelTest, StairPartOf_TellsTheFrontsOfAFlightFromItsSides)
{
    using antwika::voxelmap::faceNormal;
    using antwika::voxelmap::kVoxelFaceCount;
    using antwika::voxel::StairPart;
    using antwika::voxelmap::stairPartOf;

    constexpr VoxelPosition eastwardPosition{.x = 1};

    for (std::size_t side = 0; side < kVoxelFaceCount; ++side)
    {
        const auto normal = faceNormal(side);
        const auto expectedPart =
            normal.y != 0.0F    ? StairPart::Any
                      : normal.x != 0.0F  ? StairPart::Front
                      : StairPart::Side;

        EXPECT_EQ(stairPartOf(eastwardPosition, side), expectedPart);
    }
}

TEST(VoxelTest, StairPartOf_TurnsWithTheClimb)
{
    using antwika::voxelmap::faceNormal;
    using antwika::voxelmap::kVoxelFaceCount;
    using antwika::voxel::StairPart;
    using antwika::voxelmap::stairPartOf;

    constexpr VoxelPosition northwardPosition{.z = -1};

    for (std::size_t side = 0; side < kVoxelFaceCount; ++side)
    {
        const auto normal = faceNormal(side);
        const auto expectedPart =
            normal.y != 0.0F    ? StairPart::Any
                      : normal.z != 0.0F  ? StairPart::Front
                      : StairPart::Side;

        EXPECT_EQ(stairPartOf(northwardPosition, side), expectedPart);
    }
}

TEST(VoxelTest, StairPartOf_NamesNoPartOfAFaceWithNoFlight)
{
    using antwika::voxelmap::kVoxelFaceCount;
    using antwika::voxel::StairPart;
    using antwika::voxelmap::stairPartOf;

    for (std::size_t side = 0; side < kVoxelFaceCount; ++side)
    {
        EXPECT_EQ(
            stairPartOf(VoxelPosition{}, side), StairPart::Any);
    }
}
