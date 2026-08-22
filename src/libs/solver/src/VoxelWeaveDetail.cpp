#include "VoxelWeaveDetail.hpp"

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

namespace antwika::solver
{
    using namespace weavedetail;

    namespace weavedetail
    {

        [[nodiscard]] voxel::VoxelPosition offsetBy(
            const voxel::VoxelPosition fromPosition, const gfx::Vec3 offset)
        {
            return voxel::VoxelPosition{
                .x = fromPosition.x + static_cast<std::int32_t>(
                                  std::lround(offset.x)),
                .y = fromPosition.y + static_cast<std::int32_t>(
                                  std::lround(offset.y)),
                .z = fromPosition.z + static_cast<std::int32_t>(
                                  std::lround(offset.z))};
        }

        [[nodiscard]] gfx::Vec3 acrossOf(const std::size_t side)
        {
            return voxelmap::faceCorner(side, kTopRightCorner)
                   - voxelmap::faceCorner(side, kTopLeftCorner);
        }

        [[nodiscard]] gfx::Vec3 downOf(const std::size_t side)
        {
            return voxelmap::faceCorner(side, kBottomLeftCorner)
                   - voxelmap::faceCorner(side, kTopLeftCorner);
        }

        [[nodiscard]] tilemap::Atlas atlasOf(const std::size_t side)
        {
            return voxelmap::faceNormal(side).y != 0.0F ? tilemap::Atlas::Floor
                                                 : tilemap::Atlas::Wall;
        }

        [[nodiscard]] std::map<DomainKey, std::set<tilemap::Tile>>
        ruledTilesByDomain(const tile::TileRules &rules)
        {
            std::map<DomainKey, std::set<tilemap::Tile>> spokenTiles;

            for (const auto &rule : rules.allRules())
            {
                spokenTiles[DomainKey{
                           rule.tile.atlas,
                           rules.kindOf(rule.tile)}]
                    .insert(rule.tile);
            }

            for (const auto &[tile, part] : rules.parts())
            {
                spokenTiles[DomainKey{
                           tile.atlas, rules.kindOf(tile)}]
                    .insert(tile);
            }

            return spokenTiles;
        } // GCOVR_EXCL_LINE

        template <typename Tag, typename Said>
        [[nodiscard]] std::set<tilemap::Tile> filteredByTag(
            const std::set<tilemap::Tile> &spokenTiles,
            const Tag wantedTag,
            const Said &saidTags)
        {
            if (wantedTag == Tag::Any)
            {
                return spokenTiles;
            }

            std::set<tilemap::Tile> drawnForTiles;
            std::set<tilemap::Tile> drawnForAnyTiles;

            for (const auto tile : spokenTiles)
            {
                const auto spokenTile = saidTags(tile);

                if (spokenTile == wantedTag)
                {
                    drawnForTiles.insert(tile);
                }

                if (spokenTile == Tag::Any)
                {
                    drawnForAnyTiles.insert(tile);
                }
            }

            if (!drawnForTiles.empty())
            {
                return drawnForTiles;
            }

            return drawnForAnyTiles.empty() ? spokenTiles : drawnForAnyTiles;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] std::set<tilemap::Tile> tilesFor(
            const tile::TileRules &rules,
            const std::set<tilemap::Tile> &spokenTiles,
            const voxelmap::FaceRef &face)
        {
            const auto taggedTiles = filteredByTag(
                spokenTiles,
                voxel::facingOfStep(face.climbPosition),
                [&rules](const tilemap::Tile tile)
                { return rules.facingOf(tile); });

            const auto dressedTiles = filteredByTag(
                taggedTiles,
                voxelmap::stairPartOf(face.climbPosition, face.side),
                [&rules](const tilemap::Tile tile)
                { return rules.partOf(tile); });

            return filteredByTag(
                dressedTiles,
                face.levelHalf,
                [&rules](const tilemap::Tile tile)
                { return rules.levelOf(tile); });
        } // GCOVR_EXCL_LINE

        [[nodiscard]] bool edgesCompatible(
            const tile::TileRules &rules,
            const tilemap::Tile hereTile,
            const tilemap::TileEdge hereEdge,
            const tilemap::Tile thereTile,
            const tilemap::TileEdge thereEdge)
        {
            return (rules.hasNoRuleFor(hereTile, hereEdge, thereTile.atlas)
                    || rules.allows(hereTile, hereEdge, thereTile))
                   && (rules.hasNoRuleFor(thereTile, thereEdge, hereTile.atlas)
                       || rules.allows(thereTile, thereEdge, hereTile));
        }

        [[nodiscard]] voxel::Side sideTowards(
            const std::size_t face, const gfx::Vec3 direction)
        {
            const auto acrossDot = glm::dot(direction, acrossOf(face));

            if (acrossDot > 0.5F)
            {
                return voxel::Side::Right;
            }

            if (acrossDot < -0.5F)
            {
                return voxel::Side::Left;
            }

            return glm::dot(direction, downOf(face)) > 0.5F
                       ? voxel::Side::Bottom
                       : voxel::Side::Top;
        }

        [[nodiscard]] std::size_t faceAlong(const gfx::Vec3 direction)
        {
            for (std::size_t side = 0; side < voxelmap::kVoxelFaceCount;
                 ++side)
            {
                if (glm::dot(voxelmap::faceNormal(side), direction) > 0.5F)
                {
                    return side;
                }
            }

            return 0;
        }

        [[nodiscard]] bool atCubeFace(
            const voxel::VoxelPosition position, const gfx::Vec3 direction)
        {
            const auto corner = voxel::cubeCornerOf(position);
            const std::array<std::int32_t, kAxisCount> cubeOffsets{
                position.x - corner.x, position.y - corner.y,
                position.z - corner.z};

            for (std::size_t axis = 0; axis < kAxisCount; ++axis)
            {
                const auto alongComponent = direction[static_cast<int>(axis)];

                if (std::abs(alongComponent) < 0.5F)
                {
                    continue;
                }

                return cubeOffsets.at(axis)
                       == (alongComponent > 0.0F ? voxel::kCubeSide - 1 : 0);
            }

            return false;
        }

        [[nodiscard]] bool sameSurface(
            const voxelmap::FaceRef &oneFace,
            const voxelmap::FaceRef &otherFace)
        {
            return oneFace.cell.kind == otherFace.cell.kind
                   && oneFace.climbPosition == otherFace.climbPosition;
        }

        [[nodiscard]] bool sameSurface(
            const std::map<voxelmap::FaceRef, std::size_t> &faceIndexes,
            const std::vector<voxelmap::FaceRef> &faces,
            const voxelmap::FaceRef placeFace,
            const voxelmap::FaceRef &face)
        {
            const auto foundFace = faceIndexes.find(placeFace);

            return foundFace != faceIndexes.end()
                   && sameSurface(face, faces[foundFace->second]);
        }

        [[nodiscard]] std::vector<FaceEdge> edgesOf(
            const std::map<voxelmap::FaceRef, std::size_t> &faceIndexes,
            const std::vector<voxelmap::FaceRef> &faces,
            const voxelmap::FaceRef face)
        {
            std::vector<FaceEdge> edges;

            for (const auto way :
                 {acrossOf(face.side),
                  -acrossOf(face.side),
                  downOf(face.side),
                  -downOf(face.side)})
            {
                const voxelmap::FaceRef besideRef{
                    .cell = voxel::voxelCellAt(
                            offsetBy(face.cell.position(), way),
                            voxel::VoxelMaterial{}),
                    .side = face.side};

                edges.push_back(
                    FaceEdge{
                        .edge =
                            tilemap::TileEdge{
                                .side =
                                    sideTowards(face.side, way),
                                .edge =
                                    voxel::cubeCornerOf(face.cell.position())
                                            == voxel::cubeCornerOf(
                                                besideRef.cell.position())
                                        ? voxel::EdgeKind::Interior
                                        : voxel::EdgeKind::Boundary},
                        .atRim =
                            !sameSurface(faceIndexes, faces, besideRef, face)});
            }

            return edges;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] std::pair<gfx::Vec3, gfx::Vec3> cornerWays(
            const std::size_t side, const voxel::Corner corner)
        {
            const auto acrossDirection = acrossOf(side);
            const auto downDirection = downOf(side);
            const auto left = corner == voxel::Corner::TopLeft
                              || corner == voxel::Corner::BottomLeft;
            const auto topCorner = corner == voxel::Corner::TopLeft
                            || corner == voxel::Corner::TopRight;

            return {left ? -acrossDirection : acrossDirection,
                topCorner ? -downDirection : downDirection};
        }

        [[nodiscard]] gfx::Vec3 cornerWay(
            const std::size_t side, const voxel::Corner corner)
        {
            const auto [acrossWay, downWay] = cornerWays(side, corner);

            return acrossWay + downWay;
        }

        [[nodiscard]] bool wrapsAroundCorner(
            const std::map<voxelmap::FaceRef, std::size_t> &faceIndexes,
            const std::vector<voxelmap::FaceRef> &faces,
            const voxelmap::FaceRef face,
            const voxel::Corner corner)
        {
            const auto [acrossWay, downWay] = cornerWays(face.side, corner);

            for (const auto way : {acrossWay, downWay})
            {
                const auto foundFace = faceIndexes.find(
                    voxelmap::FaceRef{
                        .cell = voxel::voxelCellAt(
                            offsetBy(face.cell.position(), way),
                            voxel::VoxelMaterial{}),
                        .side = face.side});

                if (foundFace != faceIndexes.end()
                    && sameSurface(faces[foundFace->second], face))
                {
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]] std::map<voxel::Corner, bool> cornersBeyond(
            const std::map<voxelmap::FaceRef, std::size_t> &faceIndexes,
            const std::vector<voxelmap::FaceRef> &faces,
            const voxelmap::FaceRef face)
        {
            std::map<voxel::Corner, bool> beyondCorners;

            for (const auto corner : voxel::kEveryCorner)
            {
                beyondCorners.emplace(
                    corner,
                    sameSurface(
                        faceIndexes,
                        faces,
                        voxelmap::FaceRef{
                            .cell = voxel::voxelCellAt(
                            offsetBy(
                                face.cell.position(),
                                cornerWay(face.side, corner)),
                            voxel::VoxelMaterial{}),
                            .side = face.side},
                        face)
                        && wrapsAroundCorner(faceIndexes, faces, face, corner));
            }

            return beyondCorners;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] std::map<voxelmap::FaceRef, std::size_t> facesByPlace(
            const std::vector<voxelmap::FaceRef> &faces)
        {
            std::map<voxelmap::FaceRef, std::size_t> faceIndexes;

            for (std::size_t which = 0; which < faces.size(); ++which)
            {
                faceIndexes.emplace(faces[which], which);
            }

            return faceIndexes;
        } // GCOVR_EXCL_LINE
    }

}
