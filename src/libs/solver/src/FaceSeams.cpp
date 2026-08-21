#include <algorithm>
#include <limits>
#include <ranges>
#include <array>
#include <cmath>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <utility>

#include <antwika/wfc/AdjacencyConstraint.hpp>
#include <antwika/wfc/CompatibilityTable.hpp>
#include <antwika/wfc/Domain.hpp>
#include <antwika/wfc/IConstraint.hpp>
#include <antwika/wfc/SolveResult.hpp>
#include <antwika/wfc/Solver.hpp>

#include <antwika/solver/VoxelWeave.hpp>

#include "VoxelWeaveDetail.hpp"

namespace antwika::solver
{
    using namespace weavedetail;


    std::vector<FaceSeam> faceAdjacency(
        const std::vector<voxelmap::FaceRef> &faces, const CornerSeams corners)
    {
        const auto placedFaces = facesByPlace(faces);

        std::vector<FaceSeam> seams;

        for (std::size_t which = 0; which < faces.size(); ++which)
        {
            const auto face = faces[which];
            const std::array<gfx::Vec3, 4> directions{
                acrossOf(face.side),
                -acrossOf(face.side),
                downOf(face.side),
                -downOf(face.side)};

            for (const auto direction : directions)
            {
                const auto side = sideTowards(face.side, direction);

                const voxelmap::FaceRef besideRef{
                    .cell = offsetBy(face.cell, direction),
                    .side = face.side};
                const auto lying = placedFaces.find(besideRef);

                if (lying != placedFaces.end() && lying->second > which
                    && sameSurface(faces[lying->second], face))
                {
                    const auto kind =
                        voxel::cubeCornerOf(face.cell)
                                == voxel::cubeCornerOf(besideRef.cell)
                                 ? voxel::EdgeKind::Interior
                                 : voxel::EdgeKind::Boundary;

                    seams.push_back(
                        FaceSeam{
                            .faceA = which,
                            .faceB = lying->second,
                            .edgeA =
                                tilemap::TileEdge{.side = side, .edge = kind},
                            .edgeB = tilemap::TileEdge{
                                .side = voxel::facing(side), .edge = kind}});
                }

                if (corners == CornerSeams::Ignored)
                {
                    continue;
                }

                const voxelmap::FaceRef aroundRef{
                    .cell = face.cell, .side = faceAlong(direction)};
                const auto turning = placedFaces.find(aroundRef);

                if (turning == placedFaces.end() || turning->second <= which)
                {
                    continue;
                }

                const auto kind =
                    atCubeFace(face.cell, voxelmap::faceNormal(face.side))
                            && atCubeFace(face.cell, direction)
                        ? voxel::EdgeKind::Boundary
                        : voxel::EdgeKind::Interior;

                seams.push_back(
                    FaceSeam{
                        .faceA = which,
                        .faceB = turning->second,
                        .edgeA =
                            tilemap::TileEdge{.side = side, .edge = kind},
                        .edgeB = tilemap::TileEdge{
                            .side = sideTowards(
                                aroundRef.side,
                                voxelmap::faceNormal(face.side)),
                            .edge = kind}});
            }
        }

        return seams;
    } // GCOVR_EXCL_LINE

    bool isCornerSeam(
        const std::vector<voxelmap::FaceRef> &faces, const FaceSeam &seam)
    {
        return faces[seam.faceA].side != faces[seam.faceB].side;
    }

    std::vector<FaceSeam> sameLevelSeams(
        const std::vector<voxelmap::FaceRef> &faces,
        const std::vector<FaceSeam> &seams,
        const std::int32_t level)
    {
        std::vector<FaceSeam> hereSeams;

        for (const auto &seam : seams)
        {
            if (voxelmap::levelOf(faces[seam.faceA].cell) == level
                && voxelmap::levelOf(faces[seam.faceB].cell) == level)
            {
                hereSeams.push_back(seam);
            }
        }

        return hereSeams;
    } // GCOVR_EXCL_LINE

    std::vector<FaceSeam> crossLevelSeams(
        const std::vector<voxelmap::FaceRef> &faces,
        const std::vector<FaceSeam> &seams,
        const std::int32_t level)
    {
        std::vector<FaceSeam> throughSeams;

        for (const auto &seam : seams)
        {
            const auto hereSeams = voxelmap::levelOf(faces[seam.faceA].cell);
            const auto thereLevel = voxelmap::levelOf(faces[seam.faceB].cell);

            if ((hereSeams == level) != (thereLevel == level))
            {
                throughSeams.push_back(seam);
            }
        }

        return throughSeams;
    } // GCOVR_EXCL_LINE

    std::vector<FaceSeam> satisfiedSeams(
        const std::vector<voxelmap::FaceRef> &faces,
        const std::span<const tilemap::Tile> drawnTiles,
        const tile::TileRules &rules,
        const CornerSeams corners)
    {
        std::vector<FaceSeam> tiedSeams;

        for (const auto &seam : faceAdjacency(faces, corners))
        {
            const auto hereSeams = drawnTiles[seam.faceA];
            const auto thereTile = drawnTiles[seam.faceB];

            if (rules.allows(hereSeams, seam.edgeA, thereTile)
                && rules.allows(thereTile, seam.edgeB, hereSeams))
            {
                tiedSeams.push_back(seam);
            }
        }

        return tiedSeams;
    } // GCOVR_EXCL_LINE

}
