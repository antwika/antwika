#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

#include <antwika/gfx/Math3D.hpp>

#include <antwika/camera/FlyCamera.hpp>
#include <antwika/voxelmap/Voxel.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/tilemap/TileEdges.hpp>
#include <antwika/voxelmap/VoxelPick.hpp>

using antwika::tilemap::Atlas;
using antwika::camera::cameraOf;
using antwika::voxelmap::faceNormal;
using antwika::voxelmap::raycastFace;
using antwika::voxelmap::faceTile;
using antwika::voxelmap::projectToScreen;
using antwika::camera::defaultTransform;
using antwika::voxelmap::demoCells;
using antwika::voxelmap::Ray;
using antwika::voxelmap::rayInModelSpace;
using antwika::voxelmap::rayThrough;
using antwika::voxelmap::tilePicked;
using antwika::voxel::VoxelCell;
using antwika::voxel::VoxelMaterial;
using antwika::voxel::VoxelPosition;
using antwika::voxel::voxelsOf;
using antwika::voxel::Voxels;
using antwika::gfx::Vec3;

namespace
{
    constexpr antwika::gfx::Size kCanvasSize{
        .width = 320, .height = 180};

    constexpr float kViewHalfHeight = 1.3F;

    const auto kLone = voxelsOf({VoxelCell{}});

    [[nodiscard]] antwika::gfx::Mat4 unturned()
    {
        return antwika::gfx::identityMatrix();
    }
}

TEST(VoxelPickTest, RaycastFace_MissesAPileTheRayGoesWideOf)
{
    const Ray ray{
        .fromPosition = Vec3{10.0F, 10.0F, 10.0F},
        .direction = Vec3{0.0F, 1.0F, 0.0F}};

    EXPECT_FALSE(raycastFace(kLone, ray).has_value());
}

TEST(VoxelPickTest, RaycastFace_MissesAnEmptyPile)
{
    const Ray ray{
        .fromPosition = Vec3{0.0F, 0.0F, 5.0F},
        .direction = Vec3{0.0F, 0.0F, -1.0F}};

    EXPECT_FALSE(raycastFace({}, ray).has_value());
}

TEST(VoxelPickTest, RaycastFace_TakesTheSideTheRayComesAt)
{
    for (std::size_t side = 0;
         side < antwika::voxelmap::kVoxelFaceCount;
         ++side)
    {
        const auto way = faceNormal(side);
        const Ray ray{
            .fromPosition = way * 5.0F, .direction = -way};
        const auto pickedFace = raycastFace(kLone, ray);

        ASSERT_TRUE(pickedFace.has_value());
        EXPECT_EQ(pickedFace->side, side);
        EXPECT_EQ(pickedFace->cell, VoxelCell{});
    }
}

TEST(VoxelPickTest, RaycastFace_TakesTheNearerOfTwoVoxels)
{
    const auto pairCells = voxelsOf({
        VoxelCell{.position = {.z = 0}}, VoxelCell{.position = {.z = 1}}});
    const Ray ray{
        .fromPosition = Vec3{0.0F, 0.0F, 9.0F},
        .direction = Vec3{0.0F, 0.0F, -1.0F}};
    const auto pickedFace = raycastFace(pairCells, ray);

    ASSERT_TRUE(pickedFace.has_value());
    EXPECT_EQ(pickedFace->cell, (VoxelCell{.position = {.z = 1}}));
}

TEST(VoxelPickTest, RaycastFace_FollowsARayOnlyForwards)
{
    const Ray ray{
        .fromPosition = Vec3{0.0F, 0.0F, 5.0F},
        .direction = Vec3{0.0F, 0.0F, 1.0F}};

    EXPECT_FALSE(raycastFace(kLone, ray).has_value());
}

TEST(VoxelPickTest, RaycastFace_TakesATopOfThePyramidFromAbove)
{
    const auto cells = demoCells();
    const Ray ray{
        .fromPosition = Vec3{0.0F, 9.0F, 0.0F},
        .direction = Vec3{0.0F, -1.0F, 0.0F}};
    const auto pickedFace = raycastFace(cells, ray);

    ASSERT_TRUE(pickedFace.has_value());
    EXPECT_EQ(pickedFace->cell, (VoxelCell{.position = {.x = 0, .y = 1,
        .z = 0}}));
    EXPECT_FLOAT_EQ(faceNormal(pickedFace->side).y, 1.0F);
}

TEST(VoxelPickTest, FaceTile_DrawsFloorSidesFromTheFloorAtlas)
{
    for (std::size_t side = 0;
         side < antwika::voxelmap::kVoxelFaceCount;
         ++side)
    {
        const auto tile =
            faceTile({.cell = VoxelCell{}, .side = side});
        const auto lies = faceNormal(side).y != 0.0F;

        EXPECT_EQ(
            tile.atlas, lies ? Atlas::Floor : Atlas::Wall);
    }
}

TEST(VoxelPickTest, FaceTile_MatchesWhatTheMeshDrawsThere)
{
    constexpr VoxelCell cell{.position = {.x = 2, .y = -1, .z = 3}};

    for (std::size_t side = 0;
         side < antwika::voxelmap::kVoxelFaceCount;
         ++side)
    {
        EXPECT_EQ(
            faceTile({.cell = cell, .side = side}).index,
            antwika::voxelmap::defaultTileIndex(cell.position, side));
    }
}

TEST(VoxelPickTest, RayThrough_RunsAwayFromTheCamera)
{
    const auto camera =
        cameraOf(defaultTransform(), kCanvasSize, kViewHalfHeight);
    const auto ray = rayThrough(
        camera,
        kCanvasSize,
        antwika::gfx::PointF{160.0F, 90.0F});
    const auto forward =
        glm::normalize(camera.target() - camera.position());

    EXPECT_NEAR(glm::dot(ray.direction, forward), 1.0F, 1e-3F);
}

TEST(VoxelPickTest, RayInModelSpace_LeavesARayAloneWhereNothingIsTurned)
{
    const Ray ray{
        .fromPosition = Vec3{1.0F, 2.0F, 3.0F},
        .direction = glm::normalize(Vec3{0.0F, -1.0F, -1.0F})};
    const auto modelRay = rayInModelSpace(ray, unturned());

    EXPECT_NEAR(glm::length(modelRay.fromPosition - ray.fromPosition), 0.0F,
        1e-5F);
    EXPECT_NEAR(glm::length(modelRay.direction - ray.direction), 0.0F, 1e-5F);
}

TEST(VoxelPickTest, TilePicked_TakesATileWhereThePileIsPointedAt)
{
    const auto camera =
        cameraOf(defaultTransform(), kCanvasSize, kViewHalfHeight);

    EXPECT_TRUE(
        tilePicked(
            demoCells(),
            {},
            camera,
            unturned(),
            kCanvasSize,
            antwika::gfx::PointF{160.0F, 90.0F})
            .has_value());
}

TEST(VoxelPickTest, TilePicked_TakesNoTileFromTheBareCorner)
{
    const auto camera =
        cameraOf(defaultTransform(), kCanvasSize, kViewHalfHeight);

    EXPECT_FALSE(
        tilePicked(
            demoCells(),
            {},
            camera,
            unturned(),
            kCanvasSize,
            antwika::gfx::PointF{2.0F, 2.0F})
            .has_value());
}

TEST(VoxelPickTest, TilePicked_TakesTheTileTheMeshDrawsThere)
{
    const auto cells = demoCells();
    const auto camera =
        cameraOf(defaultTransform(), kCanvasSize, kViewHalfHeight);
    const auto screenPoint = antwika::gfx::PointF{160.0F, 90.0F};
    const auto pickedFace =
        tilePicked(cells, {}, camera, unturned(), kCanvasSize, screenPoint);
    const auto face = raycastFace(
        cells,
        rayInModelSpace(rayThrough(camera, kCanvasSize, screenPoint),
        unturned()));

    ASSERT_TRUE(pickedFace.has_value());
    ASSERT_TRUE(face.has_value());
    EXPECT_EQ(*pickedFace, faceTile(*face));
}

TEST(VoxelPickTest, TilePicked_TakesTheTileTheFaceIsDrawingNow)
{
    const auto cells = demoCells();
    const auto faces = antwika::voxelmap::visibleFacesOf(cells);
    const auto camera =
        cameraOf(defaultTransform(), kCanvasSize, kViewHalfHeight);
    const auto screenPoint = antwika::gfx::PointF{160.0F, 90.0F};
    constexpr antwika::tilemap::Tile oddTile{
        .atlas = Atlas::Floor, .index = 111};

    std::vector<antwika::tilemap::Tile> drawnTiles(faces.size(), oddTile);

    const auto pickedFace =
        tilePicked(cells, drawnTiles, camera, unturned(), kCanvasSize,
            screenPoint);

    ASSERT_TRUE(pickedFace.has_value());
    EXPECT_EQ(*pickedFace, oddTile);
}

TEST(VoxelPickTest, TilePicked_FallsBackOnWhatAFaceWouldDraw)
{
    const auto cells = demoCells();
    const auto camera =
        cameraOf(defaultTransform(), kCanvasSize, kViewHalfHeight);
    const auto screenPoint = antwika::gfx::PointF{160.0F, 90.0F};
    const auto pickedFace =
        tilePicked(cells, {}, camera, unturned(), kCanvasSize, screenPoint);
    const auto face = raycastFace(
        cells,
        rayInModelSpace(rayThrough(camera, kCanvasSize, screenPoint),
        unturned()));

    ASSERT_TRUE(pickedFace.has_value());
    ASSERT_TRUE(face.has_value());
    EXPECT_EQ(*pickedFace, faceTile(*face));
}

TEST(VoxelPickTest, FacePicked_NamesTheFaceThePileIsPointedAt)
{
    const auto cells = demoCells();
    const auto faces = antwika::voxelmap::visibleFacesOf(cells);
    const auto camera =
        cameraOf(defaultTransform(), kCanvasSize, kViewHalfHeight);
    const auto screenPoint = antwika::gfx::PointF{160.0F, 90.0F};
    const auto which = antwika::voxelmap::facePicked(
        cells, faces, camera, unturned(), kCanvasSize, screenPoint);
    const auto pickedFace = raycastFace(
        cells,
        rayInModelSpace(rayThrough(camera, kCanvasSize, screenPoint),
        unturned()));

    ASSERT_TRUE(which.has_value());
    ASSERT_TRUE(pickedFace.has_value());
    ASSERT_LT(*which, faces.size());
    EXPECT_EQ(faces.at(*which), *pickedFace);
}

TEST(VoxelPickTest, FacePicked_NamesNothingClearOfThePile)
{
    const auto cells = demoCells();
    const auto faces = antwika::voxelmap::visibleFacesOf(cells);
    const auto camera =
        cameraOf(defaultTransform(), kCanvasSize, kViewHalfHeight);

    EXPECT_FALSE(
        antwika::voxelmap::facePicked(
            cells,
            faces,
            camera,
            unturned(),
            kCanvasSize,
            antwika::gfx::PointF{2.0F, 2.0F})
            .has_value());
}

TEST(VoxelPickTest, FacePicked_NamesNothingAmongNoFaces)
{
    const auto camera =
        cameraOf(defaultTransform(), kCanvasSize, kViewHalfHeight);

    EXPECT_FALSE(
        antwika::voxelmap::facePicked(
            demoCells(),
            {},
            camera,
            unturned(),
            kCanvasSize,
            antwika::gfx::PointF{160.0F, 90.0F})
            .has_value());
}

TEST(VoxelPickTest, TilePicked_TellsOneFaceFromAnotherAmongTheDrawn)
{
    const auto cells = demoCells();
    const auto faces = antwika::voxelmap::visibleFacesOf(cells);
    const auto camera =
        cameraOf(defaultTransform(), kCanvasSize, kViewHalfHeight);
    const auto screenPoint = antwika::gfx::PointF{160.0F, 90.0F};
    auto drawnTiles = antwika::voxelmap::defaultTiles(faces);
    const auto face = raycastFace(
        cells,
        rayInModelSpace(rayThrough(camera, kCanvasSize, screenPoint),
        unturned()));

    ASSERT_TRUE(face.has_value());

    for (std::size_t which = 0; which < faces.size(); ++which)
    {
        drawnTiles[which] = faces[which] == *face
                          ? antwika::tilemap::Tile{
                                 .atlas = Atlas::Floor, .index = 7}
                                        : antwika::tilemap::Tile{
                                 .atlas = Atlas::Floor, .index = 200};
    }

    const auto pickedFace =
        tilePicked(cells, drawnTiles, camera, unturned(), kCanvasSize,
            screenPoint);

    ASSERT_TRUE(pickedFace.has_value());
    EXPECT_EQ(pickedFace->index, 7);
}

TEST(VoxelPickTest, FaceMiddle_LiesOnTheFaceNotInTheVoxel)
{
    const auto loneCells = voxelsOf({VoxelCell{}});

    for (std::size_t side = 0;
         side < antwika::voxelmap::kVoxelFaceCount;
         ++side)
    {
        const auto middlePoint = antwika::voxelmap::faceMiddle(
            {.cell = VoxelCell{}, .side = side});

        EXPECT_NEAR(
            glm::dot(
                middlePoint - antwika::voxelmap::cellMiddle(VoxelPosition{}),
                faceNormal(side)),
            antwika::voxel::kVoxelSide / 2.0F,
            1e-5F);
    }
}

TEST(VoxelPickTest, ProjectToScreen_PutsBackWhatRayThroughTookOff)
{
    const auto camera =
        cameraOf(defaultTransform(), kCanvasSize, kViewHalfHeight);
    const antwika::gfx::PointF point{123.0F, 77.0F};
    const auto ray = rayThrough(camera, kCanvasSize, point);
    const auto alongPoint = ray.fromPosition + (ray.direction * 3.0F);
    const auto backPoint =
        projectToScreen(camera, unturned(), kCanvasSize, alongPoint);

    ASSERT_TRUE(backPoint.has_value());
    EXPECT_NEAR(backPoint->x, point.x, 0.01F);
    EXPECT_NEAR(backPoint->y, point.y, 0.01F);
}

TEST(VoxelPickTest, ProjectToScreen_PutsAFaceWhereItsOwnPixelPicksIt)
{
    const auto cells = demoCells();
    const auto camera =
        cameraOf(defaultTransform(), kCanvasSize, kViewHalfHeight);
    const antwika::gfx::PointF point{160.0F, 90.0F};
    const auto face = raycastFace(
        cells,
        rayInModelSpace(rayThrough(camera, kCanvasSize, point), unturned()));

    ASSERT_TRUE(face.has_value());

    const auto where = projectToScreen(
        camera,
        unturned(),
        kCanvasSize,
        antwika::voxelmap::faceMiddle(*face));

    ASSERT_TRUE(where.has_value());

    const auto secondFace = raycastFace(
        cells,
        rayInModelSpace(
            rayThrough(camera, kCanvasSize, *where), unturned()));

    ASSERT_TRUE(secondFace.has_value());
    EXPECT_EQ(*secondFace, *face);
}

TEST(VoxelPickTest, IsFrontFacing_TellsTheNearSideFromTheFar)
{
    const auto camera =
        cameraOf(defaultTransform(), kCanvasSize, kViewHalfHeight);
    auto towardsCount = 0;
    auto awayCount = 0;

    for (std::size_t side = 0;
         side < antwika::voxelmap::kVoxelFaceCount;
         ++side)
    {
        (antwika::voxelmap::isFrontFacing(camera, unturned(), side)
             ? towardsCount
             : awayCount)
            += 1;
    }

    EXPECT_GT(towardsCount, 0);
    EXPECT_GT(awayCount, 0);
    EXPECT_EQ(towardsCount + awayCount, antwika::voxelmap::kVoxelFaceCount);
}

TEST(VoxelPickTest, IsFrontFacing_TurnsAFaceAwayWhereTheModelTurns)
{
    const auto camera =
        cameraOf(defaultTransform(), kCanvasSize, kViewHalfHeight);
    const auto rotationMatrix =
        antwika::voxelmap::modelRotation(0.0F, std::numbers::pi_v<float>);

    for (std::size_t side = 0;
         side < antwika::voxelmap::kVoxelFaceCount;
         ++side)
    {
        if (std::abs(faceNormal(side).x) > 0.5F)
        {
            continue;
        }

        EXPECT_NE(
            antwika::voxelmap::isFrontFacing(camera, unturned(), side),
            antwika::voxelmap::isFrontFacing(camera, rotationMatrix, side));
    }
}

TEST(VoxelPickTest, CellAtLevel_TakesTheCellALineMeetsTheLevelIn)
{
    const auto loneCells = voxelsOf({VoxelCell{}});
    const Ray downRay{
        .fromPosition = Vec3{0.0F, 5.0F, 0.0F},
        .direction = Vec3{0.0F, -1.0F, 0.0F}};
    const auto pickedCell = antwika::voxelmap::cellAtLevel(downRay, 0);

    ASSERT_TRUE(pickedCell.has_value());
    EXPECT_EQ(*pickedCell, VoxelPosition{});
}

TEST(VoxelPickTest, CellAtLevel_TakesTheCubeLevelHoldingTheOneGiven)
{
    const Ray downRay{
        .fromPosition = Vec3{0.0F, 9.0F, 0.0F},
        .direction = Vec3{0.0F, -1.0F, 0.0F}};

    for (const auto level : {-2, 0, 3})
    {
        const auto pickedCell = antwika::voxelmap::cellAtLevel(downRay, level);

        ASSERT_TRUE(pickedCell.has_value());
        EXPECT_EQ(pickedCell->y, antwika::voxel::cubeCornerOf(
                             VoxelPosition{.y = level}).y);
    }
}

TEST(VoxelPickTest, CellAtLevel_MissesALevelALineRunsAlong)
{
    const auto loneCells = voxelsOf({VoxelCell{}});
    const Ray acrossRay{
        .fromPosition = Vec3{0.0F, 5.0F, 0.0F},
        .direction = Vec3{1.0F, 0.0F, 0.0F}};

    EXPECT_FALSE(
        antwika::voxelmap::cellAtLevel(acrossRay, 0).has_value());
}

TEST(VoxelPickTest, CellAtLevel_MissesALevelBehindWhereItStarts)
{
    const auto loneCells = voxelsOf({VoxelCell{}});
    const Ray upRay{
        .fromPosition = Vec3{0.0F, 5.0F, 0.0F},
        .direction = Vec3{0.0F, 1.0F, 0.0F}};

    EXPECT_FALSE(
        antwika::voxelmap::cellAtLevel(upRay, 0).has_value());
}

TEST(VoxelPickTest, CellUnder_TakesTheCellThePileIsPointedAt)
{
    const auto cells = demoCells();
    const auto camera =
        cameraOf(defaultTransform(), kCanvasSize, kViewHalfHeight);
    const auto pickedCell = antwika::voxelmap::cellUnder(
        camera,
        unturned(),
        kCanvasSize,
        antwika::gfx::PointF{160.0F, 90.0F},
        0);

    ASSERT_TRUE(pickedCell.has_value());
    EXPECT_EQ(pickedCell->y, 0);
}

TEST(VoxelPickTest, LevelGridLines_RulesTheLevelIntoCubes)
{
    const auto cells =
        antwika::voxel::expandCubesToVoxels(voxelsOf({VoxelCell{}}));
    const auto gridLines = antwika::voxelmap::levelGridLines(cells, 0);

    EXPECT_FALSE(gridLines.empty());

    for (const auto &span : gridLines)
    {
        EXPECT_FLOAT_EQ(span.fromPosition.y, span.toPosition.y);
    }
}

TEST(VoxelPickTest, LevelGridLines_LiesAtTheLevelItIsGiven)
{
    const auto cells =
        antwika::voxel::expandCubesToVoxels(voxelsOf({VoxelCell{}}));
    const auto lowLines = antwika::voxelmap::levelGridLines(cells, 0);
    const auto highLines = antwika::voxelmap::levelGridLines(cells, 2);

    EXPECT_GT(highLines.front().fromPosition.y,
        lowLines.front().fromPosition.y);
}

TEST(VoxelPickTest, LevelGridLines_ReachesPastThePileEveryWay)
{
    const auto cells =
        antwika::voxel::expandCubesToVoxels(voxelsOf({VoxelCell{}}));
    const auto gridLines = antwika::voxelmap::levelGridLines(cells, 0);
    auto least = gridLines.front().fromPosition.x;
    auto most = gridLines.front().toPosition.x;

    for (const auto &span : gridLines)
    {
        least = std::min({least, span.fromPosition.x, span.toPosition.x});
        most = std::max({most, span.fromPosition.x, span.toPosition.x});
    }

    EXPECT_LT(least, -antwika::voxel::kVoxelSide);
    EXPECT_GT(most, antwika::voxel::kVoxelSide);
}

TEST(VoxelPickTest, CubeWireframe_GivesACubeItsTwelveEdges)
{
    const auto cells =
        antwika::voxel::expandCubesToVoxels(voxelsOf({VoxelCell{}}));

    EXPECT_EQ(antwika::voxelmap::cubeWireframe(VoxelPosition{}).size(),
              12U);
}

TEST(VoxelPickTest, CubeWireframe_TakesEveryCellOfACubeToTheOneCube)
{
    const auto cells =
        antwika::voxel::expandCubesToVoxels(voxelsOf({VoxelCell{}}));
    const auto wireframe =
        antwika::voxelmap::cubeWireframe(VoxelPosition{});

    for (const auto cell :
         antwika::voxel::cubeCells(
             antwika::voxel::cubeCornerOf(VoxelPosition{})))
    {
        EXPECT_EQ(
            antwika::voxelmap::cubeWireframe(cell), wireframe);
    }
}

TEST(VoxelPickTest, CubeWireframe_RunsACubeSideAlongEveryEdge)
{
    const auto cells =
        antwika::voxel::expandCubesToVoxels(voxelsOf({VoxelCell{}}));

    for (const auto &span :
         antwika::voxelmap::cubeWireframe(VoxelPosition{}))
    {
        EXPECT_NEAR(
            glm::length(span.toPosition - span.fromPosition),
            antwika::voxel::kCubeSide * antwika::voxel::kVoxelSide,
            1e-4F);
    }
}

TEST(VoxelPickTest, CellAtLevel_MeetsTheLevelAtTheFootOfItsCube)
{
    const Ray downRay{
        .fromPosition = Vec3{0.0F, 9.0F, 0.0F},
        .direction = Vec3{0.0F, -1.0F, 0.0F}};

    for (const auto level : {0, 1})
    {
        const auto pickedCell = antwika::voxelmap::cellAtLevel(downRay, level);

        ASSERT_TRUE(pickedCell.has_value());
        EXPECT_EQ(pickedCell->y, 0);
    }

    for (const auto level : {2, 3})
    {
        const auto pickedCell = antwika::voxelmap::cellAtLevel(downRay, level);

        ASSERT_TRUE(pickedCell.has_value());
        EXPECT_EQ(pickedCell->y, antwika::voxel::kCubeSide);
    }
}

TEST(VoxelPickTest, CellAtLevel_StandsACubeOnTheGridItIsRuledOn)
{
    const auto gridLines = antwika::voxelmap::levelGridLines(voxelsOf(
        {VoxelCell{}}), 0);
    const Ray downRay{
        .fromPosition = Vec3{0.0F, 9.0F, 0.0F},
        .direction = Vec3{0.0F, -1.0F, 0.0F}};
    const auto pickedCell = antwika::voxelmap::cellAtLevel(downRay, 0);

    ASSERT_TRUE(pickedCell.has_value());

    const auto edges = antwika::voxelmap::cubeWireframe(*pickedCell);
    auto lowest = edges.front().fromPosition.y;

    for (const auto &span : edges)
    {
        lowest = std::min({lowest, span.fromPosition.y, span.toPosition.y});
    }

    EXPECT_NEAR(lowest, gridLines.front().fromPosition.y, 1e-5F);
}

TEST(VoxelPickTest, OcclusionMask_MarksAPlaceAtItsOwnLevelsBit)
{
    using antwika::voxelmap::occlusionMask;

    const VoxelPosition cornerPosition{.x = -4, .z = -4};
    const auto mask = occlusionMask(
        voxelsOf({
            VoxelCell{.position = {.x = 0, .y = 9, .z = 0}},
            VoxelCell{.position = {.x = 1, .y = 0, .z = 0}}}),
        cornerPosition);

    const auto faceAt = [&cornerPosition](const VoxelPosition cell)
    {
        return ((static_cast<std::size_t>(cell.z - cornerPosition.z)
                 * antwika::voxelmap::kOcclusionMaskWidth)
                + static_cast<std::size_t>(cell.x - cornerPosition.x))
               * antwika::gfx::kBytesPerPixel;
    };

    EXPECT_EQ(
        mask.pixels[faceAt(VoxelPosition{.x = 0, .z = 0}) + 1], 1U << 1);
    EXPECT_EQ(
        mask.pixels[faceAt(VoxelPosition{.x = 1, .z = 0})], 1U);
    EXPECT_EQ(
        mask.pixels[faceAt(VoxelPosition{.x = 2, .z = 0})], 0U);
}

TEST(VoxelPickTest, OcclusionMask_LeavesOutWhatFallsOffTheSquare)
{
    using antwika::voxelmap::occlusionMask;

    const auto mask = occlusionMask(
        voxelsOf({
            VoxelCell{.position = {.x = 90, .y = 0, .z = 0}},
            VoxelCell{.position = {.x = 0, .y = 90, .z = 0}}}),
        VoxelPosition{});

    for (const auto pixel : mask.pixels)
    {
        EXPECT_EQ(pixel, 0U);
    }
}
