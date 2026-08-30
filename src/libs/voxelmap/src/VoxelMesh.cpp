#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/SizeF.hpp>

#include <antwika/voxelmap/Voxel.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/tilemap/TileEdges.hpp>
#include <antwika/voxelmap/QuadPaint.hpp>

#include "VoxelDetail.hpp"

namespace antwika::voxelmap
{
    using namespace voxeldetail;

    namespace
    {

        [[nodiscard]] voxel::VoxelPosition getAcrossStep(
            const std::size_t side)
        {
            const auto &face = kVoxelFaces[side];
            const auto acrossVector = face.corners[1] - face.corners[0];

            return voxel::VoxelPosition{
                .x =
                acrossVector.x > 0.0F ? 1 : (acrossVector.x < 0.0F ? -1 : 0),
                .z =
                acrossVector.z > 0.0F ? 1 : (acrossVector.z < 0.0F ? -1 : 0)};
        }

        [[nodiscard]] bool isSurfaceContinues(
            const voxel::Voxels &voxels,
            const FaceRef &face,
            const voxel::VoxelPosition wayPosition)
        {
            const auto besidePosition =
                getOffsetBy(face.cell.position, wayPosition);
            const auto besideKind = kindAt(voxels, besidePosition);

            if (besideKind != face.cell.material.kind)
            {
                return false;
            }

            const auto neighbourKind = effectiveKindAt(
                voxels,
                getOffsetBy(
                    besidePosition,
                    kVoxelFaces[face.side].neighbourOffsetPosition));

            return !neighbourKind.has_value()
                   || !voxel::occludes(*neighbourKind, face.cell.material.kind);
        }

        [[nodiscard]] bool isMirroredWithin(
            const voxel::Voxels &voxels, const FaceRef &faceRef)
        {
            const auto &face = kVoxelFaces[faceRef.side];
            const auto climb = faceRef.climbPosition;

            if (climb == voxel::VoxelPosition{} || face.normal.y != 0.0F)
            {
                return false;
            }

            const auto alongDot =
                (face.normal.x * static_cast<float>(climb.x))
                + (face.normal.z * static_cast<float>(climb.z));
            const auto outward = face.normal.x + face.normal.z;

            if (alongDot != 0.0F || outward > 0.0F)
            {
                return false;
            }

            const auto way = getAcrossStep(faceRef.side);
            const auto backPosition =
                voxel::VoxelPosition{.x = -way.x, .y = -way.y, .z = -way.z};

            return isSurfaceContinues(voxels, faceRef, way)
                   == isSurfaceContinues(voxels, faceRef, backPosition);
        }
    }

    gfx::Vec3 getFaceNormal(const std::size_t side)
    {
        return kVoxelFaces.at(side).normal;
    }

    voxel::StairPart stairPartOf(
        const voxel::VoxelPosition climbPosition, const std::size_t side)
    {
        if (climbPosition.x == 0 && climbPosition.z == 0)
        {
            return voxel::StairPart::Any;
        }

        const auto normal = getFaceNormal(side);

        if (normal.y != 0.0F)
        {
            return voxel::StairPart::Any;
        }

        const auto alongDot =
            (normal.x * static_cast<float>(climbPosition.x))
            + (normal.z * static_cast<float>(climbPosition.z));

        return alongDot != 0.0F ? voxel::StairPart::Front
                         : voxel::StairPart::Side;
    }

    gfx::Vec3 getFaceCorner(
        const std::size_t side, const std::size_t corner)
    {
        return kVoxelFaces.at(side).corners.at(corner);
    }

    bool usesMirroredUv(
        const voxel::Voxels &voxels, const FaceRef &face)
    {
        return isMirroredWithin(voxels, face);
    }

    gfx::RectF getStairUvRect(
        const gfx::RectF tileRect,
        const voxel::StairQuad &quad,
        const bool mirrored)
    {
        const auto uv = getUvWithinFace(
            kVoxelFaces[quad.side], quad.corners[0], quad.corners[2]);

        if (mirrored)
        {
            return gfx::RectF(
                gfx::PointF{
                    tileRect.originPoint.x
                        + ((1.0F - uv.leastU) * tileRect.size.width),
                    tileRect.originPoint.y
                        + ((1.0F - uv.mostV) * tileRect.size.height)},
                gfx::SizeF{
                    (uv.leastU - uv.mostU) * tileRect.size.width,
                    (uv.mostV - uv.leastV) * tileRect.size.height});
        }

        return gfx::RectF(
            gfx::PointF{
                tileRect.originPoint.x + (uv.leastU * tileRect.size.width),
                tileRect.originPoint.y
                    + ((1.0F - uv.mostV) * tileRect.size.height)},
            gfx::SizeF{
                (uv.mostU - uv.leastU) * tileRect.size.width,
                (uv.mostV - uv.leastV) * tileRect.size.height});
    } // GCOVR_EXCL_LINE

    void addFaceQuads(
        gfx::MeshData &mesh,
        const voxel::Voxels &voxels,
        const std::size_t side,
        const voxel::VoxelPosition climbPosition,
        const bool climbing,
        const gfx::Vec3 middlePoint,
        const QuadPaint &paint)
    {
        const auto &face = kVoxelFaces[side];
        const auto flight = climbing
                          ? voxel::getStairQuads(climbPosition)
                          : std::vector<voxel::StairQuad>{};

        std::vector<voxel::StairQuad> layingQuads;

        for (const auto &quad : flight)
        {
            if (quad.side == side)
            {
                layingQuads.push_back(quad);
            }
        }

        if (!climbing)
        {
            layingQuads.push_back(
                voxel::StairQuad{
                    .side = side,
                    .corners = face.corners});
        }

        constexpr auto gridSide =
            static_cast<std::uint32_t>(kFaceGridSide);

        const auto bevelAllowed = paint.beveled && !climbing;
        const voxel::VoxelPosition cellPosition{
            .x = static_cast<std::int32_t>(std::floor(middlePoint.x)),
            .y = static_cast<std::int32_t>(std::floor(middlePoint.y)),
            .z = static_cast<std::int32_t>(std::floor(middlePoint.z))};

        const auto axisWayOf = [](const gfx::Vec3 alongVector)
        {
            const int axis = std::abs(alongVector.x) > 0.5F ? 0
                           : std::abs(alongVector.y) > 0.5F ? 1
                                                      : 2;

            return std::pair<int, std::int32_t>{
                axis, alongVector[axis] > 0.0F ? 1 : -1};
        };

        const auto [heldAxis, heldWay] = axisWayOf(face.normal);
        const auto [acrossAxis, acrossWorldWay] =
            axisWayOf(face.corners[2] - face.corners[3]);
        const auto [downAxis, downWorldWay] =
            axisWayOf(face.corners[0] - face.corners[3]);

        const auto stepped = [](voxel::VoxelPosition fromPosition,
                                const int axis,
                                const std::int32_t way)
        {
            if (axis == 0)
            {
                fromPosition.x += way;
            }
            else if (axis == 1)
            {
                fromPosition.y += way;
            }
            else
            {
                fromPosition.z += way;
            }

            return fromPosition;
        };

        const auto filledAt = [&voxels](const voxel::VoxelPosition spot)
        { return voxels.contains(spot); };

        const auto plainAt = [&voxels](const voxel::VoxelPosition spot)
        {
            const auto foundCell = voxels.find(spot);

            return foundCell != voxels.end()
                   && foundCell->second.kind == voxel::Kind::Normal;
        };

        const auto openAt = [&](const int crossAxis,
                                const std::int32_t crossWay)
        {
            const auto besideCell =
                stepped(cellPosition, crossAxis, crossWay);

            return !filledAt(besideCell)
                   && !filledAt(stepped(besideCell, heldAxis, heldWay));
        };

        const auto carriedOn = [&](const int crossAxis,
                                   const std::int32_t crossWay,
                                   const int alongAxis,
                                   const std::int32_t alongWay)
        {
            const auto nextCell =
                stepped(cellPosition, alongAxis, alongWay);
            const auto nextBeside = stepped(nextCell, crossAxis, crossWay);

            return plainAt(nextCell) && !isRampStep(voxels, nextCell)
                   && !filledAt(stepped(nextCell, heldAxis, heldWay))
                   && !filledAt(nextBeside)
                   && !filledAt(
                       stepped(nextBeside, heldAxis, heldWay));
        };

        const auto cornerOpenAt = [&](const int crossAxis,
                                      const std::int32_t crossWay,
                                      const int alongAxis,
                                      const std::int32_t alongWay)
        {
            const auto beyondCell =
                stepped(cellPosition, alongAxis, alongWay);
            const auto diagonal = stepped(beyondCell, crossAxis, crossWay);

            return !filledAt(beyondCell)
                   && !filledAt(stepped(beyondCell, heldAxis, heldWay))
                   && !filledAt(diagonal)
                   && !filledAt(
                       stepped(diagonal, heldAxis, heldWay));
        };

        const auto sunkWayOf = [&](const int crossAxis,
                                   const std::int32_t crossWay)
        {
            gfx::Vec3 sinkDirection{0.0F, 0.0F, 0.0F};

            sinkDirection[heldAxis] = -static_cast<float>(heldWay);
            sinkDirection[crossAxis] = -static_cast<float>(crossWay);

            return sinkDirection * (kEdgeBevel * 0.5F);
        };

        for (const auto &quad : layingQuads)
        {
            const auto part =
                getStairUvRect(paint.tileRect, quad, paint.mirrored);
            const auto first =
                static_cast<std::uint32_t>(mesh.vertices.size());

            std::array<gfx::Vec3, kCornersPerFace> placedCorners{};

            for (std::size_t corner = 0;
                 corner < kCornersPerFace;
                 ++corner)
            {
                placedCorners[corner] = middlePoint + quad.corners[corner]
                                 + paint.liftPoint;
            }

            const auto pointAt = [&placedCorners](
                                     const float acrossWay,
                                     const float downWay)
            {
                const auto nearSide =
                    ((1.0F - acrossWay) * placedCorners[3])
                    + (acrossWay * placedCorners[2]);
                const auto farSide =
                    ((1.0F - acrossWay) * placedCorners[0])
                    + (acrossWay * placedCorners[1]);

                return ((1.0F - downWay) * nearSide)
                       + (downWay * farSide);
            };

            for (std::uint32_t downAt = 0; downAt < gridSide; ++downAt)
            {
                for (std::uint32_t acrossAt = 0;
                     acrossAt < gridSide;
                     ++acrossAt)
                {
                    const auto acrossStation = kFaceGridWays[acrossAt];
                    const auto downStation = kFaceGridWays[downAt];

                    gfx::Vec3 sunkOffset{0.0F, 0.0F, 0.0F};

                    if (bevelAllowed)
                    {
                        constexpr auto lastStation = gridSide - 1U;
                        const auto onAcrossEdge =
                            acrossAt == 0U || acrossAt == lastStation;
                        const auto onDownEdge =
                            downAt == 0U || downAt == lastStation;
                        const auto acrossCrossWay =
                            acrossAt == 0U ? -acrossWorldWay
                                         : acrossWorldWay;
                        const auto downCrossWay =
                            downAt == 0U ? -downWorldWay : downWorldWay;

                        const auto edgeSink =
                            [&](const int crossAxis,
                                const std::int32_t crossWay,
                                const int alongAxis,
                                const std::int32_t alongWorldWay,
                                const std::uint32_t alongAt)
                        {
                            if (!openAt(crossAxis, crossWay))
                            {
                                return;
                            }

                            const auto nearLowEnd = alongAt <= 1U;
                            const auto nearHighEnd =
                                alongAt + 1U >= lastStation;
                            const auto endWay =
                                nearLowEnd ? -alongWorldWay
                                           : alongWorldWay;

                            if ((!nearLowEnd && !nearHighEnd)
                                || carriedOn(
                                    crossAxis,
                                    crossWay,
                                    alongAxis,
                                    endWay)
                                || cornerOpenAt(
                                    crossAxis,
                                    crossWay,
                                    alongAxis,
                                    endWay))
                            {
                                sunkOffset += sunkWayOf(crossAxis, crossWay);
                            }
                        };

                        if (onAcrossEdge && onDownEdge)
                        {
                            const auto openAcross =
                                openAt(acrossAxis, acrossCrossWay);
                            const auto openDown =
                                openAt(downAxis, downCrossWay);

                            if (openAcross
                                && carriedOn(
                                    acrossAxis,
                                    acrossCrossWay,
                                    downAxis,
                                    downCrossWay))
                            {
                                sunkOffset += sunkWayOf(
                                    acrossAxis, acrossCrossWay);
                            }
                            else if (
                                openDown
                                && carriedOn(
                                    downAxis,
                                    downCrossWay,
                                    acrossAxis,
                                    acrossCrossWay))
                            {
                                sunkOffset += sunkWayOf(
                                    downAxis, downCrossWay);
                            }
                            else if (
                                openAcross && openDown
                                && cornerOpenAt(
                                    acrossAxis,
                                    acrossCrossWay,
                                    downAxis,
                                    downCrossWay))
                            {
                                gfx::Vec3 sinkDirection{
                                    0.0F, 0.0F, 0.0F};

                                sinkDirection[heldAxis] =
                                    -static_cast<float>(heldWay);
                                sinkDirection[acrossAxis] =
                                    -static_cast<float>(acrossCrossWay);
                                sinkDirection[downAxis] =
                                    -static_cast<float>(downCrossWay);

                                sunkOffset +=
                                    sinkDirection * (kEdgeBevel * 0.5F);
                            }
                        }
                        else if (onAcrossEdge)
                        {
                            edgeSink(
                                acrossAxis,
                                acrossCrossWay,
                                downAxis,
                                downWorldWay,
                                downAt);
                        }
                        else if (onDownEdge)
                        {
                            edgeSink(
                                downAxis,
                                downCrossWay,
                                acrossAxis,
                                acrossWorldWay,
                                acrossAt);
                        }
                    }

                    mesh.vertices.push_back(
                        gfx::Vertex3D{
                            .position =
                                pointAt(acrossStation, downStation)
                                + sunkOffset,
                            .normal = face.normal,
                            .texCoordinate =
                                gfx::Vec2{
                                    part.originPoint.x
                                        + acrossStation
                                              * part.size.width,
                                    part.originPoint.y
                                        + downStation
                                              * part.size.height},
                            .color = paint.color});
                }
            }

            for (std::uint32_t downAt = 0; downAt + 1U < gridSide; ++downAt)
            {
                for (std::uint32_t acrossAt = 0;
                     acrossAt + 1U < gridSide;
                     ++acrossAt)
                {
                    const auto belowVertex =
                        first + (downAt * gridSide) + acrossAt;
                    const auto aboveVertex = belowVertex + gridSide;

                    for (const std::uint32_t step :
                         {aboveVertex, aboveVertex + 1U, belowVertex + 1U,
                          aboveVertex, belowVertex + 1U, belowVertex})
                    {
                        mesh.indices.push_back(step);
                    }
                }
            }
        }
    } // GCOVR_EXCL_LINE

    gfx::MeshData getVoxelMesh(const voxel::Voxels &voxels)
    {
        const auto faces = visibleFacesOf(voxels);

        return getVoxelMesh(voxels, getDefaultTiles(faces));
    } // GCOVR_EXCL_LINE

    gfx::MeshData getVoxelMesh(
        const voxel::Voxels &voxels,
        const std::span<const tilemap::Tile> wovenTiles,
        const Pass pass)
    {
        const auto faces = visibleFacesOf(voxels);

        return getVoxelMesh(voxels, faces, wovenTiles, pass);
    } // GCOVR_EXCL_LINE

    gfx::MeshData getVoxelMesh(
        const voxel::Voxels &voxels,
        const std::span<const FaceRef> faces,
        const std::span<const tilemap::Tile> wovenTiles,
        const Pass pass)
    {
        if (faces.size() != wovenTiles.size())
        {
            throw std::invalid_argument{
                "one woven tile must stand for every face"};
        }

        gfx::MeshData mesh;

        for (std::size_t index = 0; index < faces.size(); ++index)
        {
            const auto faceRef = faces[index];
            const auto watery =
                faceRef.cell.material.kind == voxel::Kind::Water;

            if (watery != (pass == Pass::Water))
            {
                continue;
            }

            const auto wovenTile = wovenTiles[index];
            const auto tile = tilemap::getTileCoords(
                wovenTile.index, tilemap::tileSizeOf(wovenTile.atlas));
            const auto veil = watery
                            ? gfx::Color{
                                        .red = 255,
                                        .green = 255,
                                        .blue = 255,
                                        .alpha = kWaterAlpha}
                                               : kNoTintColor;

            addFaceQuads(
                mesh,
                voxels,
                faceRef.side,
                faceRef.climbPosition,
                isRampStep(voxels, faceRef.cell.position),
                getCellMiddle(faceRef.cell.position),
                QuadPaint{
                    .tileRect = tile,
                    .mirrored = isMirroredWithin(voxels, faceRef),
                    .color = veil,
                    .beveled = faceRef.cell.material.kind
                               == voxel::Kind::Normal});
        }

        return mesh;
    } // GCOVR_EXCL_LINE


    voxel::VoxelPosition getMeshRegionOf(
        const voxel::VoxelPosition position, const std::int32_t regionSide)
    {
        const auto slotOf = [regionSide](const std::int32_t place)
        {
            const auto offset = place % regionSide;

            return (place - (offset < 0 ? offset + regionSide : offset))
                   / regionSide;
        };

        return voxel::VoxelPosition{
            .x = slotOf(position.x),
            .y = slotOf(position.y),
            .z = slotOf(position.z)};
    }

    std::vector<gfx::MeshData> getVoxelMeshPieces(
        const voxel::Voxels &voxels,
        const std::span<const FaceRef> faces,
        const std::span<const tilemap::Tile> wovenTiles,
        const Pass pass,
        const std::int32_t regionSide)
    {
        if (faces.size() != wovenTiles.size())
        {
            throw std::invalid_argument{
                "one woven tile must stand for every face"};
        }

        std::map<voxel::VoxelPosition, std::vector<std::size_t>>
            facesByRegion;

        for (std::size_t index = 0; index < faces.size(); ++index)
        {
            facesByRegion[getMeshRegionOf(
                              faces[index].cell.position, regionSide)]
                .push_back(index);
        }

        std::vector<gfx::MeshData> pieceMeshes;

        for (const auto &[region, indexes] : facesByRegion)
        {
            std::vector<FaceRef> regionFaces;
            std::vector<tilemap::Tile> regionTiles;

            regionFaces.reserve(indexes.size());
            regionTiles.reserve(indexes.size());

            for (const auto index : indexes)
            {
                regionFaces.push_back(faces[index]);
                regionTiles.push_back(wovenTiles[index]);
            }

            const auto regionMesh =
                getVoxelMesh(voxels, regionFaces, regionTiles, pass);

            if (regionMesh.vertices.empty())
            {
                continue;
            }

            for (auto &piece :
                 gfx::getSplitMesh(regionMesh, kMeshPieceVertices))
            {
                pieceMeshes.push_back(std::move(piece));
            }
        }

        return pieceMeshes;
    } // GCOVR_EXCL_LINE

}
