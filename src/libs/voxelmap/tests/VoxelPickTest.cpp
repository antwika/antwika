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
using antwika::voxelmap::getFaceNormal;
using antwika::voxelmap::getRaycastFace;
using antwika::voxelmap::getFaceTile;
using antwika::voxelmap::getProjectToScreen;
using antwika::camera::getDefaultTransform;
using antwika::voxelmap::getDemoCells;
using antwika::voxelmap::Ray;
using antwika::voxelmap::getRayInModelSpace;
using antwika::voxelmap::getRayThrough;
using antwika::voxelmap::getTilePicked;
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

    [[nodiscard]] antwika::gfx::Mat4 getUnturned()
    {
        return antwika::gfx::getIdentityMatrix();
    }
}

TEST(VoxelPickTest, RaycastFace_MissesAPileTheRayGoesWideOf)
{
    const Ray ray{
        .fromPosition = Vec3{10.0F, 10.0F, 10.0F},
        .direction = Vec3{0.0F, 1.0F, 0.0F}};

    EXPECT_FALSE(getRaycastFace(kLone, ray).has_value());
}

TEST(VoxelPickTest, RaycastFace_MissesAnEmptyPile)
{
    const Ray ray{
        .fromPosition = Vec3{0.0F, 0.0F, 5.0F},
        .direction = Vec3{0.0F, 0.0F, -1.0F}};

    EXPECT_FALSE(getRaycastFace({}, ray).has_value());
}

TEST(VoxelPickTest, RaycastFace_TakesTheSideTheRayComesAt)
{
    for (std::size_t side = 0;
         side < antwika::voxelmap::kVoxelFaceCount;
         ++side)
    {
        const auto way = getFaceNormal(side);
        const Ray ray{
            .fromPosition = way * 5.0F, .direction = -way};
        const auto pickedFace = getRaycastFace(kLone, ray);

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
    const auto pickedFace = getRaycastFace(pairCells, ray);

    ASSERT_TRUE(pickedFace.has_value());
    EXPECT_EQ(pickedFace->cell, (VoxelCell{.position = {.z = 1}}));
}

TEST(VoxelPickTest, RaycastFace_FollowsARayOnlyForwards)
{
    const Ray ray{
        .fromPosition = Vec3{0.0F, 0.0F, 5.0F},
        .direction = Vec3{0.0F, 0.0F, 1.0F}};

    EXPECT_FALSE(getRaycastFace(kLone, ray).has_value());
}

TEST(VoxelPickTest, RaycastFace_TakesATopOfThePyramidFromAbove)
{
    const auto cells = getDemoCells();
    const Ray ray{
        .fromPosition = Vec3{0.0F, 9.0F, 0.0F},
        .direction = Vec3{0.0F, -1.0F, 0.0F}};
    const auto pickedFace = getRaycastFace(cells, ray);

    ASSERT_TRUE(pickedFace.has_value());
    EXPECT_EQ(pickedFace->cell, (VoxelCell{.position = {.x = 0, .y = 1,
        .z = 0}}));
    EXPECT_FLOAT_EQ(getFaceNormal(pickedFace->side).y, 1.0F);
}

TEST(VoxelPickTest, FaceTile_DrawsFloorSidesFromTheFloorAtlas)
{
    for (std::size_t side = 0;
         side < antwika::voxelmap::kVoxelFaceCount;
         ++side)
    {
        const auto tile =
            getFaceTile({.cell = VoxelCell{}, .side = side});
        const auto lies = getFaceNormal(side).y != 0.0F;

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
            getFaceTile({.cell = cell, .side = side}).index,
            antwika::voxelmap::getDefaultTileIndex(cell.position, side));
    }
}

TEST(VoxelPickTest, RayThrough_RunsAwayFromTheCamera)
{
    const auto camera =
        cameraOf(getDefaultTransform(), kCanvasSize, kViewHalfHeight);
    const auto ray = getRayThrough(
        camera,
        kCanvasSize,
        antwika::gfx::PointF{160.0F, 90.0F});
    const auto forward =
        glm::normalize(camera.getTarget() - camera.getPosition());

    EXPECT_NEAR(glm::dot(ray.direction, forward), 1.0F, 1e-3F);
}

TEST(VoxelPickTest, RayInModelSpace_LeavesARayAloneWhereNothingIsTurned)
{
    const Ray ray{
        .fromPosition = Vec3{1.0F, 2.0F, 3.0F},
        .direction = glm::normalize(Vec3{0.0F, -1.0F, -1.0F})};
    const auto modelRay = getRayInModelSpace(ray, getUnturned());

    EXPECT_NEAR(glm::length(modelRay.fromPosition - ray.fromPosition), 0.0F,
        1e-5F);
    EXPECT_NEAR(glm::length(modelRay.direction - ray.direction), 0.0F, 1e-5F);
}

TEST(VoxelPickTest, TilePicked_TakesATileWhereThePileIsPointedAt)
{
    const auto camera =
        cameraOf(getDefaultTransform(), kCanvasSize, kViewHalfHeight);

    EXPECT_TRUE(
        getTilePicked(
            getDemoCells(),
            {},
            camera,
            getUnturned(),
            kCanvasSize,
            antwika::gfx::PointF{160.0F, 90.0F})
            .has_value());
}

TEST(VoxelPickTest, TilePicked_TakesNoTileFromTheBareCorner)
{
    const auto camera =
        cameraOf(getDefaultTransform(), kCanvasSize, kViewHalfHeight);

    EXPECT_FALSE(
        getTilePicked(
            getDemoCells(),
            {},
            camera,
            getUnturned(),
            kCanvasSize,
            antwika::gfx::PointF{2.0F, 2.0F})
            .has_value());
}

TEST(VoxelPickTest, TilePicked_TakesTheTileTheMeshDrawsThere)
{
    const auto cells = getDemoCells();
    const auto camera =
        cameraOf(getDefaultTransform(), kCanvasSize, kViewHalfHeight);
    const auto screenPoint = antwika::gfx::PointF{160.0F, 90.0F};
    const auto pickedFace =
        getTilePicked(cells, {}, camera, getUnturned(), kCanvasSize, screenPoint);
    const auto face = getRaycastFace(
        cells,
        getRayInModelSpace(getRayThrough(camera, kCanvasSize, screenPoint),
        getUnturned()));

    ASSERT_TRUE(pickedFace.has_value());
    ASSERT_TRUE(face.has_value());
    EXPECT_EQ(*pickedFace, getFaceTile(*face));
}

TEST(VoxelPickTest, TilePicked_TakesTheTileTheFaceIsDrawingNow)
{
    const auto cells = getDemoCells();
    const auto faces = antwika::voxelmap::visibleFacesOf(cells);
    const auto camera =
        cameraOf(getDefaultTransform(), kCanvasSize, kViewHalfHeight);
    const auto screenPoint = antwika::gfx::PointF{160.0F, 90.0F};
    constexpr antwika::tilemap::Tile oddTile{
        .atlas = Atlas::Floor, .index = 111};

    std::vector<antwika::tilemap::Tile> drawnTiles(faces.size(), oddTile);

    const auto pickedFace =
        getTilePicked(cells, drawnTiles, camera, getUnturned(), kCanvasSize,
            screenPoint);

    ASSERT_TRUE(pickedFace.has_value());
    EXPECT_EQ(*pickedFace, oddTile);
}

TEST(VoxelPickTest, TilePicked_FallsBackOnWhatAFaceWouldDraw)
{
    const auto cells = getDemoCells();
    const auto camera =
        cameraOf(getDefaultTransform(), kCanvasSize, kViewHalfHeight);
    const auto screenPoint = antwika::gfx::PointF{160.0F, 90.0F};
    const auto pickedFace =
        getTilePicked(cells, {}, camera, getUnturned(), kCanvasSize, screenPoint);
    const auto face = getRaycastFace(
        cells,
        getRayInModelSpace(getRayThrough(camera, kCanvasSize, screenPoint),
        getUnturned()));

    ASSERT_TRUE(pickedFace.has_value());
    ASSERT_TRUE(face.has_value());
    EXPECT_EQ(*pickedFace, getFaceTile(*face));
}

TEST(VoxelPickTest, FacePicked_NamesTheFaceThePileIsPointedAt)
{
    const auto cells = getDemoCells();
    const auto faces = antwika::voxelmap::visibleFacesOf(cells);
    const auto camera =
        cameraOf(getDefaultTransform(), kCanvasSize, kViewHalfHeight);
    const auto screenPoint = antwika::gfx::PointF{160.0F, 90.0F};
    const auto which = antwika::voxelmap::getFacePicked(
        cells, faces, camera, getUnturned(), kCanvasSize, screenPoint);
    const auto pickedFace = getRaycastFace(
        cells,
        getRayInModelSpace(getRayThrough(camera, kCanvasSize, screenPoint),
        getUnturned()));

    ASSERT_TRUE(which.has_value());
    ASSERT_TRUE(pickedFace.has_value());
    ASSERT_LT(*which, faces.size());
    EXPECT_EQ(faces.at(*which), *pickedFace);
}

TEST(VoxelPickTest, FacePicked_NamesNothingClearOfThePile)
{
    const auto cells = getDemoCells();
    const auto faces = antwika::voxelmap::visibleFacesOf(cells);
    const auto camera =
        cameraOf(getDefaultTransform(), kCanvasSize, kViewHalfHeight);

    EXPECT_FALSE(
        antwika::voxelmap::getFacePicked(
            cells,
            faces,
            camera,
            getUnturned(),
            kCanvasSize,
            antwika::gfx::PointF{2.0F, 2.0F})
            .has_value());
}

TEST(VoxelPickTest, FacePicked_NamesNothingAmongNoFaces)
{
    const auto camera =
        cameraOf(getDefaultTransform(), kCanvasSize, kViewHalfHeight);

    EXPECT_FALSE(
        antwika::voxelmap::getFacePicked(
            getDemoCells(),
            {},
            camera,
            getUnturned(),
            kCanvasSize,
            antwika::gfx::PointF{160.0F, 90.0F})
            .has_value());
}

TEST(VoxelPickTest, TilePicked_TellsOneFaceFromAnotherAmongTheDrawn)
{
    const auto cells = getDemoCells();
    const auto faces = antwika::voxelmap::visibleFacesOf(cells);
    const auto camera =
        cameraOf(getDefaultTransform(), kCanvasSize, kViewHalfHeight);
    const auto screenPoint = antwika::gfx::PointF{160.0F, 90.0F};
    auto drawnTiles = antwika::voxelmap::getDefaultTiles(faces);
    const auto face = getRaycastFace(
        cells,
        getRayInModelSpace(getRayThrough(camera, kCanvasSize, screenPoint),
        getUnturned()));

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
        getTilePicked(cells, drawnTiles, camera, getUnturned(), kCanvasSize,
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
        const auto middlePoint = antwika::voxelmap::getFaceMiddle(
            {.cell = VoxelCell{}, .side = side});

        EXPECT_NEAR(
            glm::dot(
                middlePoint - antwika::voxelmap::getCellMiddle(VoxelPosition{}),
                getFaceNormal(side)),
            antwika::voxel::kVoxelSide / 2.0F,
            1e-5F);
    }
}

TEST(VoxelPickTest, ProjectToScreen_PutsBackWhatRayThroughTookOff)
{
    const auto camera =
        cameraOf(getDefaultTransform(), kCanvasSize, kViewHalfHeight);
    const antwika::gfx::PointF point{123.0F, 77.0F};
    const auto ray = getRayThrough(camera, kCanvasSize, point);
    const auto alongPoint = ray.fromPosition + (ray.direction * 3.0F);
    const auto backPoint =
        getProjectToScreen(camera, getUnturned(), kCanvasSize, alongPoint);

    ASSERT_TRUE(backPoint.has_value());
    EXPECT_NEAR(backPoint->x, point.x, 0.01F);
    EXPECT_NEAR(backPoint->y, point.y, 0.01F);
}

TEST(VoxelPickTest, ProjectToScreen_PutsAFaceWhereItsOwnPixelPicksIt)
{
    const auto cells = getDemoCells();
    const auto camera =
        cameraOf(getDefaultTransform(), kCanvasSize, kViewHalfHeight);
    const antwika::gfx::PointF point{160.0F, 90.0F};
    const auto face = getRaycastFace(
        cells,
        getRayInModelSpace(getRayThrough(camera, kCanvasSize, point), getUnturned()));

    ASSERT_TRUE(face.has_value());

    const auto where = getProjectToScreen(
        camera,
        getUnturned(),
        kCanvasSize,
        antwika::voxelmap::getFaceMiddle(*face));

    ASSERT_TRUE(where.has_value());

    const auto secondFace = getRaycastFace(
        cells,
        getRayInModelSpace(
            getRayThrough(camera, kCanvasSize, *where), getUnturned()));

    ASSERT_TRUE(secondFace.has_value());
    EXPECT_EQ(*secondFace, *face);
}

TEST(VoxelPickTest, IsFrontFacing_TellsTheNearSideFromTheFar)
{
    const auto camera =
        cameraOf(getDefaultTransform(), kCanvasSize, kViewHalfHeight);
    auto towardsCount = 0;
    auto awayCount = 0;

    for (std::size_t side = 0;
         side < antwika::voxelmap::kVoxelFaceCount;
         ++side)
    {
        (antwika::voxelmap::isFrontFacing(camera, getUnturned(), side)
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
        cameraOf(getDefaultTransform(), kCanvasSize, kViewHalfHeight);
    const auto rotationMatrix =
        antwika::voxelmap::getModelRotation(0.0F, std::numbers::pi_v<float>);

    for (std::size_t side = 0;
         side < antwika::voxelmap::kVoxelFaceCount;
         ++side)
    {
        if (std::abs(getFaceNormal(side).x) > 0.5F)
        {
            continue;
        }

        EXPECT_NE(
            antwika::voxelmap::isFrontFacing(camera, getUnturned(), side),
            antwika::voxelmap::isFrontFacing(camera, rotationMatrix, side));
    }
}

TEST(VoxelPickTest, CellAtLevel_TakesTheCellALineMeetsTheLevelIn)
{
    const auto loneCells = voxelsOf({VoxelCell{}});
    const Ray downRay{
        .fromPosition = Vec3{0.0F, 5.0F, 0.0F},
        .direction = Vec3{0.0F, -1.0F, 0.0F}};
    const auto pickedCell = antwika::voxelmap::getCellAtLevel(downRay, 0);

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
        const auto pickedCell = antwika::voxelmap::getCellAtLevel(downRay, level);

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
        antwika::voxelmap::getCellAtLevel(acrossRay, 0).has_value());
}

TEST(VoxelPickTest, CellAtLevel_MissesALevelBehindWhereItStarts)
{
    const auto loneCells = voxelsOf({VoxelCell{}});
    const Ray upRay{
        .fromPosition = Vec3{0.0F, 5.0F, 0.0F},
        .direction = Vec3{0.0F, 1.0F, 0.0F}};

    EXPECT_FALSE(
        antwika::voxelmap::getCellAtLevel(upRay, 0).has_value());
}

TEST(VoxelPickTest, CellUnder_TakesTheCellThePileIsPointedAt)
{
    const auto cells = getDemoCells();
    const auto camera =
        cameraOf(getDefaultTransform(), kCanvasSize, kViewHalfHeight);
    const auto pickedCell = antwika::voxelmap::getCellUnder(
        camera,
        getUnturned(),
        kCanvasSize,
        antwika::gfx::PointF{160.0F, 90.0F},
        0);

    ASSERT_TRUE(pickedCell.has_value());
    EXPECT_EQ(pickedCell->y, 0);
}

TEST(VoxelPickTest, LevelGridLines_RulesTheLevelIntoCubes)
{
    const auto cells =
        antwika::voxel::getExpandCubesToVoxels(voxelsOf({VoxelCell{}}));
    const auto gridLines = antwika::voxelmap::getLevelGridLines(cells, 0);

    EXPECT_FALSE(gridLines.empty());

    for (const auto &span : gridLines)
    {
        EXPECT_FLOAT_EQ(span.fromPosition.y, span.toPosition.y);
    }
}

TEST(VoxelPickTest, LevelGridLines_LiesAtTheLevelItIsGiven)
{
    const auto cells =
        antwika::voxel::getExpandCubesToVoxels(voxelsOf({VoxelCell{}}));
    const auto lowLines = antwika::voxelmap::getLevelGridLines(cells, 0);
    const auto highLines = antwika::voxelmap::getLevelGridLines(cells, 2);

    EXPECT_GT(highLines.front().fromPosition.y,
        lowLines.front().fromPosition.y);
}

TEST(VoxelPickTest, LevelGridLines_ReachesPastThePileEveryWay)
{
    const auto cells =
        antwika::voxel::getExpandCubesToVoxels(voxelsOf({VoxelCell{}}));
    const auto gridLines = antwika::voxelmap::getLevelGridLines(cells, 0);
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
        antwika::voxel::getExpandCubesToVoxels(voxelsOf({VoxelCell{}}));

    EXPECT_EQ(antwika::voxelmap::getCubeWireframe(VoxelPosition{}).size(),
              12U);
}

TEST(VoxelPickTest, CubeWireframe_TakesEveryCellOfACubeToTheOneCube)
{
    const auto cells =
        antwika::voxel::getExpandCubesToVoxels(voxelsOf({VoxelCell{}}));
    const auto wireframe =
        antwika::voxelmap::getCubeWireframe(VoxelPosition{});

    for (const auto cell :
         antwika::voxel::getCubeCells(
             antwika::voxel::cubeCornerOf(VoxelPosition{})))
    {
        EXPECT_EQ(
            antwika::voxelmap::getCubeWireframe(cell), wireframe);
    }
}

TEST(VoxelPickTest, CubeWireframe_RunsACubeSideAlongEveryEdge)
{
    const auto cells =
        antwika::voxel::getExpandCubesToVoxels(voxelsOf({VoxelCell{}}));

    for (const auto &span :
         antwika::voxelmap::getCubeWireframe(VoxelPosition{}))
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
        const auto pickedCell = antwika::voxelmap::getCellAtLevel(downRay, level);

        ASSERT_TRUE(pickedCell.has_value());
        EXPECT_EQ(pickedCell->y, 0);
    }

    for (const auto level : {2, 3})
    {
        const auto pickedCell = antwika::voxelmap::getCellAtLevel(downRay, level);

        ASSERT_TRUE(pickedCell.has_value());
        EXPECT_EQ(pickedCell->y, antwika::voxel::kCubeSide);
    }
}

TEST(VoxelPickTest, CellAtLevel_StandsACubeOnTheGridItIsRuledOn)
{
    const auto gridLines = antwika::voxelmap::getLevelGridLines(voxelsOf(
        {VoxelCell{}}), 0);
    const Ray downRay{
        .fromPosition = Vec3{0.0F, 9.0F, 0.0F},
        .direction = Vec3{0.0F, -1.0F, 0.0F}};
    const auto pickedCell = antwika::voxelmap::getCellAtLevel(downRay, 0);

    ASSERT_TRUE(pickedCell.has_value());

    const auto edges = antwika::voxelmap::getCubeWireframe(*pickedCell);
    auto lowest = edges.front().fromPosition.y;

    for (const auto &span : edges)
    {
        lowest = std::min({lowest, span.fromPosition.y, span.toPosition.y});
    }

    EXPECT_NEAR(lowest, gridLines.front().fromPosition.y, 1e-5F);
}

TEST(VoxelPickTest, OcclusionMask_MarksAPlaceAtItsOwnLevelsBit)
{
    using antwika::voxelmap::getOcclusionMask;

    const VoxelPosition cornerPosition{.x = -4, .z = -4};
    const auto mask = getOcclusionMask(
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
    using antwika::voxelmap::getOcclusionMask;

    const auto mask = getOcclusionMask(
        voxelsOf({
            VoxelCell{.position = {.x = 90, .y = 0, .z = 0}},
            VoxelCell{.position = {.x = 0, .y = 90, .z = 0}}}),
        VoxelPosition{});

    for (const auto pixel : mask.pixels)
    {
        EXPECT_EQ(pixel, 0U);
    }
}
