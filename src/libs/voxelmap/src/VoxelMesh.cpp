#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/SizeF.hpp>

#include <antwika/voxelmap/Voxel.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/tilemap/TileEdges.hpp>

#include "VoxelDetail.hpp"

namespace antwika::voxelmap
{
    using namespace voxeldetail;

    namespace
    {

        [[nodiscard]] voxel::VoxelCell acrossStep(const std::size_t side)
        {
            const auto &face = kVoxelFaces[side];
            const auto acrossVector = face.corners[1] - face.corners[0];

            return voxel::VoxelCell{
                .x =
                acrossVector.x > 0.0F ? 1 : (acrossVector.x < 0.0F ? -1 : 0),
                .z =
                acrossVector.z > 0.0F ? 1 : (acrossVector.z < 0.0F ? -1 : 0)};
        }

        [[nodiscard]] bool surfaceContinues(
            const std::set<voxel::VoxelCell> &filledCells,
            const FaceRef &face,
            const voxel::VoxelCell wayCell)
        {
            const auto besideCell = offsetBy(face.cell, wayCell);
            const auto besideKind = kindAt(filledCells, besideCell);

            if (besideKind != face.cell.kind)
            {
                return false;
            }

            const auto neighbourKind = effectiveKindAt(
                filledCells,
                offsetBy(
                    besideCell, kVoxelFaces[face.side].neighbourOffsetCell));

            return !neighbourKind.has_value()
                   || !voxel::occludes(*neighbourKind, face.cell.kind);
        }

        [[nodiscard]] bool mirroredWithin(
            const std::set<voxel::VoxelCell> &filledCells,
            const FaceRef &faceRef)
        {
            const auto &face = kVoxelFaces[faceRef.side];
            const auto climb = faceRef.climbCell;

            if (climb == voxel::VoxelCell{} || face.normal.y != 0.0F)
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

            const auto way = acrossStep(faceRef.side);
            const auto backCell =
                voxel::VoxelCell{.x = -way.x, .y = -way.y, .z = -way.z};

            return surfaceContinues(filledCells, faceRef, way)
                   == surfaceContinues(filledCells, faceRef, backCell);
        }
    }

    gfx::Vec3 faceNormal(const std::size_t side)
    {
        return kVoxelFaces.at(side).normal;
    }

    voxel::StairPart stairPartOf(
        const voxel::VoxelCell climbCell, const std::size_t side)
    {
        if (climbCell.x == 0 && climbCell.z == 0)
        {
            return voxel::StairPart::Any;
        }

        const auto normal = faceNormal(side);

        if (normal.y != 0.0F)
        {
            return voxel::StairPart::Any;
        }

        const auto alongDot =
            (normal.x * static_cast<float>(climbCell.x))
            + (normal.z * static_cast<float>(climbCell.z));

        return alongDot != 0.0F ? voxel::StairPart::Front
                         : voxel::StairPart::Side;
    }

    gfx::Vec3 faceCorner(
        const std::size_t side, const std::size_t corner)
    {
        return kVoxelFaces.at(side).corners.at(corner);
    }

    bool usesMirroredUv(
        const std::vector<voxel::VoxelCell> &cells, const FaceRef &face)
    {
        const std::set<voxel::VoxelCell> filledCells(
            cells.begin(),
            cells.end());

        return mirroredWithin(filledCells, face);
    }

    gfx::RectF stairUvRect(
        const gfx::RectF tileRect,
        const voxel::StairQuad &quad,
        const bool mirrored)
    {
        const auto uv = uvWithinFace(
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

    gfx::MeshData voxelMesh(const std::vector<voxel::VoxelCell> &cells)
    {
        const auto faces = visibleFacesOf(cells);

        return voxelMesh(cells, defaultTiles(faces));
    } // GCOVR_EXCL_LINE

    gfx::MeshData voxelMesh(
        const std::vector<voxel::VoxelCell> &cells,
        const std::span<const tilemap::Tile> wovenTiles,
        const Pass pass)
    {
        const auto faces = visibleFacesOf(cells);

        return voxelMesh(cells, faces, wovenTiles, pass);
    } // GCOVR_EXCL_LINE

    gfx::MeshData voxelMesh(
        const std::vector<voxel::VoxelCell> &cells,
        const std::span<const FaceRef> faces,
        const std::span<const tilemap::Tile> wovenTiles,
        const Pass pass)
    {
        const std::set<voxel::VoxelCell> filledCells(
            cells.begin(),
            cells.end());

        gfx::MeshData mesh;

        for (std::size_t index = 0; index < faces.size(); ++index)
        {
            const auto faceRef = faces[index];
            const auto watery = faceRef.cell.kind == voxel::Kind::Water;

            if (watery != (pass == Pass::Water))
            {
                continue;
            }

            const auto &face = kVoxelFaces[faceRef.side];
            const auto middlePoint = cellMiddle(faceRef.cell);
            const auto wovenTile = wovenTiles[index];
            const auto tile = tilemap::tileCoords(
                wovenTile.index, tilemap::tileSizeOf(wovenTile.atlas));
            const auto veil = watery
                            ? gfx::Color{
                                        .red = 255,
                                        .green = 255,
                                        .blue = 255,
                                        .alpha = kWaterAlpha}
                                               : kNoTintColor;
            const auto flight =
                isRampStep(filledCells, faceRef.cell)
                    ? voxel::stairQuads(faceRef.climbCell)
                    : std::vector<voxel::StairQuad>{};

            std::vector<voxel::StairQuad> layingQuads;

            for (const auto &quad : flight)
            {
                if (quad.side == faceRef.side)
                {
                    layingQuads.push_back(quad);
                }
            }

            if (flight.empty())
            {
                layingQuads.push_back(
                    voxel::StairQuad{
                        .side = faceRef.side,
                        .corners = face.corners});
            }

            for (const auto &quad : layingQuads)
            {
                const auto part = stairUvRect(
                    tile, quad, mirroredWithin(filledCells, faceRef));
                const auto first =
                    static_cast<std::uint32_t>(mesh.vertices.size());

                for (std::size_t corner = 0;
                     corner < kCornersPerFace;
                     ++corner)
                {
                    const auto cornerOffset = kFaceCorners[corner];

                    mesh.vertices.push_back(
                        gfx::Vertex3D{
                            .position = middlePoint + quad.corners[corner],
                            .normal = face.normal,
                            .texCoordinate =
                                gfx::Vec2{
                                    part.originPoint.x
                                        + cornerOffset.x * part.size.width,
                                    part.originPoint.y
                                        + cornerOffset.y
                                              * part.size.height},
                            .color = veil});
                }

                for (const std::uint32_t step :
                     {0U, 1U, 2U, 0U, 2U, 3U})
                {
                    mesh.indices.push_back(first + step);
                }
            }
        }

        return mesh;
    } // GCOVR_EXCL_LINE

}
