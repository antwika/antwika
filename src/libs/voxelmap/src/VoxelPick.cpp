#include "antwika/voxelmap/VoxelPick.hpp"

#include <glm/matrix.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/tilemap/TileEdges.hpp>

namespace antwika::voxelmap
{

    namespace
    {
        constexpr std::size_t kAxisCount = 3;

        constexpr float kHalf = voxel::kVoxelSide / 2.0F;

        constexpr float kSameWay = 0.5F;

        struct RayHit final
        {
            float awayDistance = 0.0F;

            std::size_t axis = 0;

            bool met = false;
        };

        [[nodiscard]] gfx::Vec3 unprojected(
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

        [[nodiscard]] RayHit metBy(
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

        [[nodiscard]] std::size_t sideFacing(const gfx::Vec3 direction)
        {
            for (std::size_t side = 0; side < kVoxelFaceCount; ++side)
            {
                if (glm::dot(faceNormal(side), direction) > kSameWay)
                {
                    return side;
                }
            }

            return 0;
        }
    }

    Ray rayThrough(
        const gfx::Camera3D &camera,
        const gfx::Size canvasSize,
        const gfx::PointF point)
    {
        const auto undoMatrix = glm::inverse(camera.viewProjection());
        const auto ndcX =
            (2.0F * point.x / static_cast<float>(canvasSize.width)) - 1.0F;
        const auto ndcY =
            1.0F - (2.0F * point.y / static_cast<float>(canvasSize.height));
        const auto nearPoint =
            unprojected(undoMatrix, gfx::Vec3{ndcX, ndcY, -1.0F});
        const auto farPoint =
            unprojected(undoMatrix, gfx::Vec3{ndcX, ndcY, 1.0F});

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
            return cellMiddle(voxel::VoxelCell{.x = x, .y = y, .z = z})
                   - gfx::Vec3{kHalf, kHalf, kHalf};
        }

        [[nodiscard]] std::int32_t lowestOf(
            const std::vector<voxel::VoxelCell> &cells,
            const std::int32_t voxel::VoxelCell::*way)
        {
            if (cells.empty())
            {
                return 0;
            }

            auto least = cells.front().*way;

            for (const auto cell : cells)
            {
                least = std::min(least, cell.*way);
            }

            return least;
        }

        [[nodiscard]] std::int32_t highestOf(
            const std::vector<voxel::VoxelCell> &cells,
            const std::int32_t voxel::VoxelCell::*way)
        {
            if (cells.empty())
            {
                return 0;
            }

            auto most = cells.front().*way;

            for (const auto cell : cells)
            {
                most = std::max(most, cell.*way);
            }

            return most;
        }
    }

    std::vector<LineSegment> levelGridLines(
        const std::vector<voxel::VoxelCell> &cells,
        const std::int32_t level)
    {
        const auto reach = kGridMarginCubes * voxel::kCubeSide;
        const auto lowX = voxel::cubeCornerOf(
            voxel::VoxelCell{.x = lowestOf(cells, &voxel::VoxelCell::x)});
        const auto highX = voxel::cubeCornerOf(
            voxel::VoxelCell{.x = highestOf(cells, &voxel::VoxelCell::x)});
        const auto lowZ = voxel::cubeCornerOf(
            voxel::VoxelCell{.z = lowestOf(cells, &voxel::VoxelCell::z)});
        const auto highZ = voxel::cubeCornerOf(
            voxel::VoxelCell{.z = highestOf(cells, &voxel::VoxelCell::z)});
        const auto fromX = lowX.x - reach;
        const auto toX = highX.x + reach + voxel::kCubeSide;
        const auto fromZ = lowZ.z - reach;
        const auto toZ = highZ.z + reach + voxel::kCubeSide;
        const auto foot = voxel::cubeCornerOf(voxel::VoxelCell{.y = level}).y;

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

    std::array<LineSegment, 12> cubeWireframe(const voxel::VoxelCell cell)
    {
        const auto cornerCell = voxel::cubeCornerOf(cell);
        const auto toCell = voxel::VoxelCell{
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

    std::vector<LineSegment> buildableTopOutlines(
        const std::vector<voxel::VoxelCell> &cells, const std::int32_t level)
    {
        const auto foot = voxel::cubeCornerOf(voxel::VoxelCell{.y = level}).y;
        const std::set<voxel::VoxelCell> filledCells(
            cells.begin(),
            cells.end());

        std::vector<LineSegment> rimSegments;

        for (const auto cell : cells)
        {
            const voxel::VoxelCell overCell{
                .x = cell.x, .y = cell.y + 1, .z = cell.z};

            if (cell.y != foot - 1 || cell.kind != voxel::Kind::Normal
                || filledCells.contains(overCell))
            {
                continue;
            }

            const auto corner = latticeAt(cell.x, foot, cell.z);
            const auto acrossCell = latticeAt(cell.x + 1, foot, cell.z);
            const auto alongCell = latticeAt(cell.x, foot, cell.z + 1);
            const auto both =
                latticeAt(cell.x + 1, foot, cell.z + 1);

            rimSegments.push_back(
                LineSegment{
                    .fromPosition = corner,
                    .toPosition = acrossCell});
            rimSegments.push_back(
                LineSegment{
                    .fromPosition = corner,
                    .toPosition = alongCell});
            rimSegments.push_back(
                LineSegment{
                    .fromPosition = acrossCell,
                    .toPosition = both});
            rimSegments.push_back(
                LineSegment{
                    .fromPosition = alongCell,
                    .toPosition = both});
        }

        return rimSegments;
    } // GCOVR_EXCL_LINE

    gfx::Vec3 faceMiddle(const FaceRef face)
    {
        return cellMiddle(face.cell) + (faceNormal(face.side) * kHalf);
    }

    bool isFrontFacing(
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix,
        const std::size_t side)
    {
        const auto turnedNormal = gfx::Vec3{
            modelMatrix * gfx::Vec4{faceNormal(side), 0.0F}};
        const auto looking =
            glm::normalize(camera.target() - camera.position());

        return glm::dot(turnedNormal, looking) < 0.0F;
    }

    std::optional<gfx::PointF> projectToScreen(
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix,
        const gfx::Size canvasSize,
        const gfx::Vec3 position)
    {
        return projectToScreen(
            camera.viewProjection() * modelMatrix, canvasSize, position);
    }

    std::optional<gfx::PointF> projectToScreen(
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

    std::optional<gfx::Vec3> planeHit(
        const Ray &ray, const float height)
    {
        if (std::abs(ray.direction.y) < 0.0001F)
        {
            return std::nullopt;
        }

        const auto awayFraction =
            (height - ray.fromPosition.y) / ray.direction.y;

        if (awayFraction <= 0.0F)
        {
            return std::nullopt;
        }

        return ray.fromPosition + (ray.direction * awayFraction);
    }

    Ray rayInModelSpace(const Ray &ray, const gfx::Mat4 &modelMatrix)
    {
        const auto undoMatrix = glm::inverse(modelMatrix);
        const auto fromPoint =
            gfx::Vec3{undoMatrix * gfx::Vec4{ray.fromPosition, 1.0F}};
        const auto onward =
            gfx::Vec3{undoMatrix * gfx::Vec4{ray.direction, 0.0F}};

        return Ray{.fromPosition = fromPoint,
            .direction = glm::normalize(onward)};
    }

    std::optional<FaceRef> raycastFace(
        const std::vector<voxel::VoxelCell> &cells, const Ray &ray)
    {
        std::optional<FaceRef> pickedRef;
        auto nearest = std::numeric_limits<float>::infinity();

        for (const auto cell : cells)
        {
            const auto middlePoint = cellMiddle(cell);
            const auto hit = metBy(
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
            pickedRef =
                FaceRef{.cell = cell, .side = sideFacing(direction)};
        }

        return pickedRef;
    }

    std::optional<voxel::VoxelCell> cellAtLevel(
        const Ray &ray, const std::int32_t level)
    {
        if (std::abs(ray.direction.y)
            < std::numeric_limits<float>::epsilon())
        {
            return std::nullopt;
        }

        const auto foot = voxel::cubeCornerOf(voxel::VoxelCell{.y = level}).y;
        const auto lying =
            (static_cast<float>(foot) * voxel::kVoxelSide)
            - (voxel::kVoxelSide / 2.0F);
        const auto awayFraction =
            (lying - ray.fromPosition.y) / ray.direction.y;

        if (awayFraction < 0.0F)
        {
            return std::nullopt;
        }

        const auto point = ray.fromPosition + (ray.direction * awayFraction);

        return voxel::VoxelCell{
            .x = static_cast<std::int32_t>(
                std::floor(point.x / voxel::kVoxelSide)),
            .y = foot,
            .z = static_cast<std::int32_t>(
                std::floor(point.z / voxel::kVoxelSide))};
    }

    std::optional<voxel::VoxelCell> cellUnder(
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix,
        const gfx::Size canvasSize,
        const gfx::PointF point,
        const std::int32_t level)
    {
        return cellAtLevel(
            rayInModelSpace(rayThrough(camera, canvasSize, point), modelMatrix),
            level);
    }

    tilemap::Tile faceTile(const FaceRef pickRef)
    {
        const auto lies = faceNormal(pickRef.side).y != 0.0F;

        return tilemap::Tile{
            .atlas = lies ? tilemap::Atlas::Floor : tilemap::Atlas::Wall,
            .index = static_cast<std::uint16_t>(
                defaultTileIndex(pickRef.cell, pickRef.side))};
    }

    std::optional<tilemap::Tile> tilePicked(
        const std::vector<voxel::VoxelCell> &cells,
        const std::span<const tilemap::Tile> drawnTiles,
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix,
        const gfx::Size canvasSize,
        const gfx::PointF point)
    {
        const auto faces = drawnTiles.empty()
                         ? std::vector<FaceRef>{}
                         : visibleFacesOf(cells);

        return tilePicked(
            cells, faces, drawnTiles, camera, modelMatrix, canvasSize, point);
    } // GCOVR_EXCL_LINE

    std::optional<tilemap::Tile> tilePicked(
        const std::vector<voxel::VoxelCell> &cells,
        const std::span<const FaceRef> faces,
        const std::span<const tilemap::Tile> drawnTiles,
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix,
        const gfx::Size canvasSize,
        const gfx::PointF point)
    {
        const auto pickedRef = raycastFace(
            cells,
            rayInModelSpace(
                rayThrough(camera, canvasSize, point),
                modelMatrix));

        if (!pickedRef.has_value())
        {
            return std::nullopt;
        }

        if (drawnTiles.empty())
        {
            return faceTile(*pickedRef);
        }

        for (std::size_t which = 0; which < faces.size(); ++which)
        {
            if (faces[which] == *pickedRef)
            {
                return drawnTiles[which];
            }
        }

        return faceTile(*pickedRef);
    }

    std::optional<std::size_t> facePicked(
        const std::vector<voxel::VoxelCell> &cells,
        const std::span<const FaceRef> faces,
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix,
        const gfx::Size canvasSize,
        const gfx::PointF point)
    {
        const auto pickedRef = raycastFace(
            cells,
            rayInModelSpace(
                rayThrough(camera, canvasSize, point),
                modelMatrix));

        if (!pickedRef.has_value())
        {
            return std::nullopt;
        }

        for (std::size_t which = 0; which < faces.size(); ++which)
        {
            if (faces[which] == *pickedRef)
            {
                return which;
            }
        }

        return std::nullopt;
    }

}
