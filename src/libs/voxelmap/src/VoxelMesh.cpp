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

        const auto beveled = paint.beveled && !climbing;
        const voxel::VoxelPosition cellPosition{
            .x = static_cast<std::int32_t>(std::floor(middlePoint.x)),
            .y = static_cast<std::int32_t>(std::floor(middlePoint.y)),
            .z = static_cast<std::int32_t>(std::floor(middlePoint.z))};

        const auto axisWayOf = [](const gfx::Vec3 along)
        {
            const int axis = std::abs(along.x) > 0.5F ? 0
                           : std::abs(along.y) > 0.5F ? 1
                                                      : 2;

            return std::pair<int, std::int32_t>{
                axis, along[axis] > 0.0F ? 1 : -1};
        };

        const auto [heldAxis, heldWay] = axisWayOf(face.normal);
        const auto [acrossAxis, acrossWorldWay] =
            axisWayOf(face.corners[2] - face.corners[3]);
        const auto [downAxis, downWorldWay] =
            axisWayOf(face.corners[0] - face.corners[3]);

        const auto stepped = [](voxel::VoxelPosition from,
                                const int axis,
                                const std::int32_t way)
        {
            if (axis == 0)
            {
                from.x += way;
            }
            else if (axis == 1)
            {
                from.y += way;
            }
            else
            {
                from.z += way;
            }

            return from;
        };

        const auto filledAt = [&voxels](const voxel::VoxelPosition spot)
        { return voxels.contains(spot); };

        const auto plainAt = [&voxels](const voxel::VoxelPosition spot)
        {
            const auto held = voxels.find(spot);

            return held != voxels.end()
                   && held->second.kind == voxel::Kind::Normal;
        };

        // An edge stands open when no coplanar face continues past
        // it and no block leans against it across the corner; only
        // there may the border band sink.
        const auto openAt = [&](const int crossAxis,
                                const std::int32_t crossWay)
        {
            const auto beside =
                stepped(cellPosition, crossAxis, crossWay);

            return !filledAt(beside)
                   && !filledAt(stepped(beside, heldAxis, heldWay));
        };

        // At a cell corner the bevel carries on only when the next
        // cell along holds the same kind of open edge, so a long
        // edge chamfers in one run while a block's true corner
        // tapers back to a sharp point. Every face that shares the
        // corner asks the same questions of the same cells, so they
        // all sink it alike and the mesh stays sealed.
        const auto carriedOn = [&](const int crossAxis,
                                   const std::int32_t crossWay,
                                   const int alongAxis,
                                   const std::int32_t alongWay)
        {
            const auto next =
                stepped(cellPosition, alongAxis, alongWay);
            const auto nextBeside = stepped(next, crossAxis, crossWay);

            return plainAt(next) && !isRampStep(voxels, next)
                   && !filledAt(stepped(next, heldAxis, heldWay))
                   && !filledAt(nextBeside)
                   && !filledAt(
                       stepped(nextBeside, heldAxis, heldWay));
        };

        // A corner is mitred only when every cell around it stands
        // empty; each of the three faces meeting there asks after
        // the same cells, so they sink the shared corner alike.
        // openAt covers the two cells beside the edge, so together
        // the checks see all seven cells around the corner point.
        const auto cornerOpenAt = [&](const int crossAxis,
                                      const std::int32_t crossWay,
                                      const int alongAxis,
                                      const std::int32_t alongWay)
        {
            const auto beyond =
                stepped(cellPosition, alongAxis, alongWay);
            const auto diagonal = stepped(beyond, crossAxis, crossWay);

            return !filledAt(beyond)
                   && !filledAt(stepped(beyond, heldAxis, heldWay))
                   && !filledAt(diagonal)
                   && !filledAt(
                       stepped(diagonal, heldAxis, heldWay));
        };

        // Each face folds its edge row down to the middle of the
        // cut, half the band width along both the normal and the
        // crossing axis; the partner face folds to the same line
        // from its side, and the two half-bands meet as one flat
        // 45-degree chamfer.
        const auto sunkWayOf = [&](const int crossAxis,
                                   const std::int32_t crossWay)
        {
            gfx::Vec3 way{0.0F, 0.0F, 0.0F};

            way[heldAxis] = -static_cast<float>(heldWay);
            way[crossAxis] = -static_cast<float>(crossWay);

            return way * (kEdgeBevel * 0.5F);
        };

        for (const auto &quad : layingQuads)
        {
            const auto part =
                getStairUvRect(paint.tileRect, quad, paint.mirrored);
            const auto first =
                static_cast<std::uint32_t>(mesh.vertices.size());

            std::array<gfx::Vec3, kCornersPerFace> placed{};

            for (std::size_t corner = 0;
                 corner < kCornersPerFace;
                 ++corner)
            {
                placed[corner] = middlePoint + quad.corners[corner]
                                 + paint.liftPoint;
            }

            // A quad's corners sit on the tile's (across, down)
            // plane with corner 3 at (0, 0), 2 at (1, 0), 0 at
            // (0, 1) and 1 at (1, 1). One product per
            // corner and commutative sums keep a point two faces
            // share bit-identical on both, so the jitter hash in the
            // voxel shader moves the two copies together.
            const auto pointAt =
                [&placed](const float across, const float down)
            {
                const auto nearSide = ((1.0F - across) * placed[3])
                                      + (across * placed[2]);
                const auto farSide = ((1.0F - across) * placed[0])
                                     + (across * placed[1]);

                return ((1.0F - down) * nearSide) + (down * farSide);
            };

            for (std::uint32_t down = 0; down < gridSide; ++down)
            {
                for (std::uint32_t across = 0;
                     across < gridSide;
                     ++across)
                {
                    const auto acrossStation = kFaceGridWays[across];
                    const auto downStation = kFaceGridWays[down];

                    gfx::Vec3 sunk{0.0F, 0.0F, 0.0F};

                    if (beveled)
                    {
                        constexpr auto last = gridSide - 1U;
                        const auto onAcrossEdge =
                            across == 0U || across == last;
                        const auto onDownEdge =
                            down == 0U || down == last;
                        const auto acrossCrossWay =
                            across == 0U ? -acrossWorldWay
                                         : acrossWorldWay;
                        const auto downCrossWay =
                            down == 0U ? -downWorldWay : downWorldWay;

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

                            // The two stations nearest an end sink
                            // when the edge carries on into the
                            // next cell or turns a fully open,
                            // mitred corner; only a corner a block
                            // rests against tapers back to sharp.
                            const auto nearLowEnd = alongAt <= 1U;
                            const auto nearHighEnd =
                                alongAt + 1U >= last;
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
                                sunk += sunkWayOf(crossAxis, crossWay);
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
                                sunk += sunkWayOf(
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
                                sunk += sunkWayOf(
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
                                // The corner sinks to where the
                                // three chamfer planes cross, so
                                // the bevels wrap the block's
                                // corner as one mitred cut.
                                gfx::Vec3 way{0.0F, 0.0F, 0.0F};

                                way[heldAxis] =
                                    -static_cast<float>(heldWay);
                                way[acrossAxis] = -static_cast<float>(
                                    acrossCrossWay);
                                way[downAxis] = -static_cast<float>(
                                    downCrossWay);

                                sunk += way * (kEdgeBevel * 0.5F);
                            }
                        }
                        else if (onAcrossEdge)
                        {
                            edgeSink(
                                acrossAxis,
                                acrossCrossWay,
                                downAxis,
                                downWorldWay,
                                down);
                        }
                        else if (onDownEdge)
                        {
                            edgeSink(
                                downAxis,
                                downCrossWay,
                                acrossAxis,
                                acrossWorldWay,
                                across);
                        }
                    }

                    mesh.vertices.push_back(
                        gfx::Vertex3D{
                            .position =
                                pointAt(acrossStation, downStation)
                                + sunk,
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

            for (std::uint32_t down = 0; down + 1U < gridSide; ++down)
            {
                for (std::uint32_t across = 0;
                     across + 1U < gridSide;
                     ++across)
                {
                    const auto below =
                        first + (down * gridSide) + across;
                    const auto above = below + gridSide;

                    for (const std::uint32_t step :
                         {above, above + 1U, below + 1U,
                          above, below + 1U, below})
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
