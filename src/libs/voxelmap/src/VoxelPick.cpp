#include "antwika/voxelmap/VoxelPick.hpp"

#include <glm/matrix.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/tilemap/TileEdges.hpp>
#include <antwika/voxelmap/VoxelBounds.hpp>

namespace antwika::voxelmap
{

    namespace
    {
        constexpr std::size_t kAxisCount = 3;

        constexpr float kHalf = voxel::kVoxelSide / 2.0F;

        constexpr float kSameWay = 0.5F;

        constexpr float kPlaneHitTolerance = 0.0001F;

        struct RayHit final
        {
            float awayDistance = 0.0F;

            std::size_t axis = 0;

            bool met = false;
        };

        [[nodiscard]] gfx::Vec3 getUnprojectedPoint(
            const gfx::Mat4 &undoMatrix, const gfx::Vec3 ndcPosition)
        {
            const auto modelPoint =
                undoMatrix
                * gfx::Vec4{
                    ndcPosition.x,
                    ndcPosition.y,
                    ndcPosition.z,
                    1.0F};

            return gfx::Vec3{modelPoint} / modelPoint.w;
        }

        [[nodiscard]] RayHit getMetBy(
            const Ray &ray, const gfx::Vec3 lowestPosition,
            const gfx::Vec3 highestPosition)
        {
            auto nearest = -std::numeric_limits<float>::infinity();
            auto furthest = std::numeric_limits<float>::infinity();
            std::size_t axis = 0;

            for (std::size_t way = 0; way < kAxisCount; ++way)
            {
                const auto alongComponent =
                ray.direction[static_cast<int>(way)];
                const auto fromComponent =
                    ray.fromPosition[static_cast<int>(way)];
                const auto lowBound = lowestPosition[static_cast<int>(way)];
                const auto highBound = highestPosition[static_cast<int>(way)];

                if (std::abs(alongComponent)
                    < std::numeric_limits<float>::min())
                {
                    if (fromComponent < lowBound || fromComponent > highBound)
                    {
                        return RayHit{};
                    }

                    continue;
                }

                const auto one = (lowBound - fromComponent) / alongComponent;
                const auto highCrossing =
                    (highBound - fromComponent) / alongComponent;
                const auto nearCrossing = std::min(one, highCrossing);
                const auto farthest = std::max(one, highCrossing);

                if (nearCrossing > nearest)
                {
                    nearest = nearCrossing;
                    axis = way;
                }

                furthest = std::min(furthest, farthest);
            }

            if (nearest > furthest || furthest < 0.0F)
            {
                return RayHit{};
            }

            return RayHit{
                .awayDistance = nearest, .axis = axis, .met = true};
        }

        [[nodiscard]] std::size_t getSideFacing(const gfx::Vec3 direction)
        {
            for (std::size_t side = 0; side < kVoxelFaceCount; ++side)
            {
                if (glm::dot(getFaceNormal(side), direction) > kSameWay)
                {
                    return side;
                }
            }

            return 0;
        }
    }

    Ray getRayThrough(
        const gfx::Camera3D &camera,
        const gfx::Size canvasSize,
        const gfx::PointF point)
    {
        const auto undoMatrix = glm::inverse(camera.getViewProjection());
        const auto ndcX =
            (2.0F * point.x / static_cast<float>(canvasSize.width)) - 1.0F;
        const auto ndcY =
            1.0F - (2.0F * point.y / static_cast<float>(canvasSize.height));
        const auto nearPoint =
            getUnprojectedPoint(undoMatrix, gfx::Vec3{ndcX, ndcY, -1.0F});
        const auto farPoint =
            getUnprojectedPoint(undoMatrix, gfx::Vec3{ndcX, ndcY, 1.0F});

        return Ray{
            .fromPosition = nearPoint,
            .direction = glm::normalize(farPoint - nearPoint)};
    }

    namespace
    {
        [[nodiscard]] gfx::Vec3 latticeAt(
            const std::int32_t x,
            const std::int32_t y,
            const std::int32_t z)
        {
            return getCellMiddle(voxel::VoxelPosition{.x = x, .y = y, .z = z})
                   - gfx::Vec3{kHalf, kHalf, kHalf};
        }

    }

    std::vector<LineSegment> getLevelGridLines(
        const voxel::Voxels &voxels, const std::int32_t level)
    {
        const auto reach = kGridMarginCubes * voxel::kCubeSide;
        const auto bounds = boundsOf(voxels);
        const auto lowX = voxel::cubeCornerOf(
            voxel::VoxelPosition{.x = bounds.lowestPosition.x});
        const auto highX = voxel::cubeCornerOf(
            voxel::VoxelPosition{.x = bounds.highestPosition.x});
        const auto lowZ = voxel::cubeCornerOf(
            voxel::VoxelPosition{.z = bounds.lowestPosition.z});
        const auto highZ = voxel::cubeCornerOf(
            voxel::VoxelPosition{.z = bounds.highestPosition.z});
        const auto fromX = lowX.x - reach;
        const auto toX = highX.x + reach + voxel::kCubeSide;
        const auto fromZ = lowZ.z - reach;
        const auto toZ = highZ.z + reach + voxel::kCubeSide;
        const auto foot = voxel::cubeCornerOf(
            voxel::VoxelPosition{.y = level}).y;

        std::vector<LineSegment> ruledSegments;

        for (auto x = fromX; x <= toX; x += voxel::kCubeSide)
        {
            ruledSegments.push_back(
                LineSegment{
                    .fromPosition = latticeAt(x, foot, fromZ),
                    .toPosition = latticeAt(x, foot, toZ)});
        }

        for (auto z = fromZ; z <= toZ; z += voxel::kCubeSide)
        {
            ruledSegments.push_back(
                LineSegment{
                    .fromPosition = latticeAt(fromX, foot, z),
                    .toPosition = latticeAt(toX, foot, z)});
        }

        return ruledSegments;
    } // GCOVR_EXCL_LINE

    gfx::Vec3 getCubeMiddle(const voxel::VoxelPosition position)
    {
        const auto cornerCell = voxel::cubeCornerOf(position);
        const auto half =
            static_cast<float>(voxel::kCubeSide) * voxel::kVoxelSide / 2.0F;

        return latticeAt(cornerCell.x, cornerCell.y, cornerCell.z)
               + gfx::Vec3{half, half, half};
    } // GCOVR_EXCL_LINE

    std::array<LineSegment, 3> getCubeGizmoSpans(
        const voxel::VoxelPosition position)
    {
        const auto middle = getCubeMiddle(position);
        const auto arm = kCubeGizmoArm;

        return {
            LineSegment{
                .fromPosition = middle - gfx::Vec3{arm, 0.0F, 0.0F},
                .toPosition = middle + gfx::Vec3{arm, 0.0F, 0.0F}},
            LineSegment{
                .fromPosition = middle - gfx::Vec3{0.0F, arm, 0.0F},
                .toPosition = middle + gfx::Vec3{0.0F, arm, 0.0F}},
            LineSegment{
                .fromPosition = middle - gfx::Vec3{0.0F, 0.0F, arm},
                .toPosition = middle + gfx::Vec3{0.0F, 0.0F, arm}}};
    } // GCOVR_EXCL_LINE

    std::array<LineSegment, 12> getCubeWireframe(
        const voxel::VoxelPosition position)
    {
        const auto cornerCell = voxel::cubeCornerOf(position);
        const auto toCell = voxel::VoxelPosition{
            .x = cornerCell.x + voxel::kCubeSide,
            .y = cornerCell.y + voxel::kCubeSide,
            .z = cornerCell.z + voxel::kCubeSide};

        std::array<LineSegment, 12> edgeSegments{};
        std::size_t count = 0;

        for (const auto y : {cornerCell.y, toCell.y})
        {
            for (const auto x : {cornerCell.x, toCell.x})
            {
                edgeSegments.at(count++) = LineSegment{
                    .fromPosition = latticeAt(x, y, cornerCell.z),
                    .toPosition = latticeAt(x, y, toCell.z)};
            }

            for (const auto z : {cornerCell.z, toCell.z})
            {
                edgeSegments.at(count++) = LineSegment{
                    .fromPosition = latticeAt(cornerCell.x, y, z),
                    .toPosition = latticeAt(toCell.x, y, z)};
            }
        }

        for (const auto x : {cornerCell.x, toCell.x})
        {
            for (const auto z : {cornerCell.z, toCell.z})
            {
                edgeSegments.at(count++) = LineSegment{
                    .fromPosition = latticeAt(x, cornerCell.y, z),
                    .toPosition = latticeAt(x, toCell.y, z)};
            }
        }

        return edgeSegments;
    } // GCOVR_EXCL_LINE

    std::array<LineSegment, 4> getCellRimSegments(const CellRim rim)
    {
        const auto corner = latticeAt(rim.cellX, rim.latticeFoot, rim.cellZ);
        const auto acrossPoint =
            latticeAt(rim.cellX + 1, rim.latticeFoot, rim.cellZ);
        const auto alongPoint =
            latticeAt(rim.cellX, rim.latticeFoot, rim.cellZ + 1);
        const auto bothPoint =
            latticeAt(rim.cellX + 1, rim.latticeFoot, rim.cellZ + 1);

        return std::array<LineSegment, 4>{
            LineSegment{
                .fromPosition = corner,
                .toPosition = acrossPoint},
            LineSegment{
                .fromPosition = corner,
                .toPosition = alongPoint},
            LineSegment{
                .fromPosition = acrossPoint,
                .toPosition = bothPoint},
            LineSegment{
                .fromPosition = alongPoint,
                .toPosition = bothPoint}};
    } // GCOVR_EXCL_LINE

    std::vector<LineSegment> getBuildableTopOutlines(
        const voxel::Voxels &voxels, const std::int32_t level)
    {
        const auto foot =
            voxel::cubeCornerOf(voxel::VoxelPosition{.y = level}).y;

        std::vector<LineSegment> rimSegments;

        for (const auto &[position, material] : voxels)
        {
            const voxel::VoxelPosition overPosition{
                .x = position.x, .y = position.y + 1, .z = position.z};

            if (position.y != foot - 1
                || material.kind != voxel::Kind::Normal
                || voxels.contains(overPosition))
            {
                continue;
            }

            const auto cellSegments = getCellRimSegments(
                CellRim{
                    .cellX = position.x,
                    .cellZ = position.z,
                    .latticeFoot = foot});

            rimSegments.insert(
                rimSegments.end(),
                cellSegments.begin(),
                cellSegments.end());
        }

        return rimSegments;
    } // GCOVR_EXCL_LINE

    gfx::Vec3 getFaceMiddle(const FaceRef face)
    {
        return getCellMiddle(face.cell.position) + (getFaceNormal(
            face.side) * kHalf);
    }

    bool isFrontFacing(
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix,
        const std::size_t side)
    {
        const auto turnedNormal = gfx::Vec3{
            modelMatrix * gfx::Vec4{getFaceNormal(side), 0.0F}};
        const auto looking =
            glm::normalize(camera.getTarget() - camera.getPosition());

        return glm::dot(turnedNormal, looking) < 0.0F;
    }

    std::optional<gfx::PointF> getProjectToScreen(
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix,
        const gfx::Size canvasSize,
        const gfx::Vec3 position)
    {
        return getProjectToScreen(
            camera.getViewProjection() * modelMatrix, canvasSize, position);
    }

    std::optional<gfx::PointF> getProjectToScreen(
        const gfx::Mat4 &clipMatrix,
        const gfx::Size canvasSize,
        const gfx::Vec3 position)
    {
        const auto clipPoint = clipMatrix * gfx::Vec4{position, 1.0F};

        if (clipPoint.w <= 0.0F)
        {
            return std::nullopt;
        }

        const auto ndcPosition = gfx::Vec3{clipPoint} / clipPoint.w;

        return gfx::PointF{
            (ndcPosition.x + 1.0F) * 0.5F
                * static_cast<float>(canvasSize.width),
            (1.0F - ndcPosition.y) * 0.5F
                * static_cast<float>(canvasSize.height)};
    }

    std::optional<gfx::Vec3> getPlaneHit(
        const Ray &ray, const float height)
    {
        if (std::abs(ray.direction.y) < kPlaneHitTolerance)
        {
            return std::nullopt;
        }

        const auto awayFraction =
            (height - ray.fromPosition.y) / ray.direction.y;

        if (awayFraction < 0.0F)
        {
            return std::nullopt;
        }

        return ray.fromPosition + (ray.direction * awayFraction);
    }

    Ray getRayInModelSpace(const Ray &ray, const gfx::Mat4 &modelMatrix)
    {
        const auto undoMatrix = glm::inverse(modelMatrix);
        const auto fromPoint =
            gfx::Vec3{undoMatrix * gfx::Vec4{ray.fromPosition, 1.0F}};
        const auto onward =
            gfx::Vec3{undoMatrix * gfx::Vec4{ray.direction, 0.0F}};

        return Ray{.fromPosition = fromPoint,
            .direction = glm::normalize(onward)};
    }

    std::optional<FaceRef> getRaycastFace(
        const voxel::Voxels &voxels, const Ray &ray)
    {
        std::optional<FaceRef> pickedRef;
        auto nearest = std::numeric_limits<float>::infinity();

        for (const auto &[position, material] : voxels)
        {
            const auto middlePoint = getCellMiddle(position);
            const auto hit = getMetBy(
                ray,
                middlePoint - gfx::Vec3{kHalf, kHalf, kHalf},
                middlePoint + gfx::Vec3{kHalf, kHalf, kHalf});

            if (!hit.met || hit.awayDistance < 0.0F
                || hit.awayDistance >= nearest)
            {
                continue;
            }

            gfx::Vec3 direction{0.0F, 0.0F, 0.0F};

            direction[static_cast<int>(hit.axis)] =
                ray.direction[static_cast<int>(hit.axis)] < 0.0F ? 1.0F
                                                             : -1.0F;

            nearest = hit.awayDistance;
            pickedRef = FaceRef{
                .cell = voxel::VoxelCell{
                        .position = position,
                        .material = material},
                .side = getSideFacing(direction)};
        }

        return pickedRef;
    }

    std::optional<voxel::VoxelPosition> getCellAtLevel(
        const Ray &ray, const std::int32_t level)
    {
        const auto foot =
            voxel::cubeCornerOf(voxel::VoxelPosition{.y = level}).y;
        const auto lying =
            (static_cast<float>(foot) * voxel::kVoxelSide)
            - (voxel::kVoxelSide / 2.0F);
        const auto hitPoint = getPlaneHit(ray, lying);

        if (!hitPoint.has_value())
        {
            return std::nullopt;
        }

        return voxel::VoxelPosition{
            .x = static_cast<std::int32_t>(
                std::floor(hitPoint->x / voxel::kVoxelSide)),
            .y = foot,
            .z = static_cast<std::int32_t>(
                std::floor(hitPoint->z / voxel::kVoxelSide))};
    }

    std::optional<voxel::VoxelPosition> getCellUnder(
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix,
        const gfx::Size canvasSize,
        const gfx::PointF point,
        const std::int32_t level)
    {
        return getCellAtLevel(
            getRayInModelSpace(getRayThrough(camera, canvasSize, point), modelMatrix),
            level);
    }

    tilemap::Tile getFaceTile(const FaceRef pickRef)
    {
        const auto lies = getFaceNormal(pickRef.side).y != 0.0F;

        return tilemap::Tile{
            .atlas = lies ? tilemap::Atlas::Floor : tilemap::Atlas::Wall,
            .index = static_cast<std::uint16_t>(
                getDefaultTileIndex(pickRef.cell.position, pickRef.side))};
    }

    std::optional<tilemap::Tile> getTilePicked(
        const voxel::Voxels &voxels,
        const std::span<const tilemap::Tile> drawnTiles,
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix,
        const gfx::Size canvasSize,
        const gfx::PointF point)
    {
        const auto faces = drawnTiles.empty()
                         ? std::vector<FaceRef>{}
                         : visibleFacesOf(voxels);

        return getTilePicked(
            voxels, faces, drawnTiles, camera, modelMatrix, canvasSize, point);
    } // GCOVR_EXCL_LINE

    std::optional<tilemap::Tile> getTilePicked(
        const voxel::Voxels &voxels,
        const std::span<const FaceRef> faces,
        const std::span<const tilemap::Tile> drawnTiles,
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix,
        const gfx::Size canvasSize,
        const gfx::PointF point)
    {
        if (!drawnTiles.empty())
        {
            const auto which = getFacePicked(
                voxels, faces, camera, modelMatrix, canvasSize, point);

            if (which.has_value())
            {
                return drawnTiles[*which];
            }
        }

        const auto pickedRef = getRaycastFace(
            voxels,
            getRayInModelSpace(
                getRayThrough(camera, canvasSize, point),
                modelMatrix));

        if (!pickedRef.has_value())
        {
            return std::nullopt;
        }

        return getFaceTile(*pickedRef);
    }

    std::optional<std::size_t> getFacePicked(
        const voxel::Voxels &voxels,
        const std::span<const FaceRef> faces,
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix,
        const gfx::Size canvasSize,
        const gfx::PointF point)
    {
        const auto pickedRef = getRaycastFace(
            voxels,
            getRayInModelSpace(
                getRayThrough(camera, canvasSize, point),
                modelMatrix));

        if (!pickedRef.has_value())
        {
            return std::nullopt;
        }

        for (std::size_t which = 0; which < faces.size(); ++which)
        {
            if (faces[which].refersToSameFace(*pickedRef))
            {
                return which;
            }
        }

        return std::nullopt;
    }

}
