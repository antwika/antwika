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

#include <antwika/enums/Enumeration.hpp>
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

    namespace
    {

        [[nodiscard]] std::string_view getFacingNamed(
            const std::size_t side)
        {
            const auto normal = voxelmap::getFaceNormal(side);

            if (normal.y > 0.0F)
            {
                return "top";
            }

            if (normal.y < 0.0F)
            {
                return "underside";
            }

            if (normal.z > 0.0F)
            {
                return "south side";
            }

            if (normal.z < 0.0F)
            {
                return "north side";
            }

            return normal.x > 0.0F ? "east side" : "west side";
        }

        [[nodiscard]] std::string getWhereNamed(
            const std::vector<voxelmap::FaceRef> &conflictFaces)
        {
            if (conflictFaces.empty())
            {
                return {};
            }

            std::string message = "; look at the ";

            for (std::size_t index = 0; index < conflictFaces.size(); ++index)
            {
                message += index == 0 ? "" : ", the ";
                message += std::string(getFacingNamed(conflictFaces[index].side))
                        + " of ("
                        + std::to_string(
                            conflictFaces[index].cell.position.x) + ","
                        + std::to_string(
                            conflictFaces[index].cell.position.y) + ","
                        + std::to_string(
                            conflictFaces[index].cell.position.z) + ")";
            }

            return message;
        } // GCOVR_EXCL_LINE

        struct SideNameRow final
        {
            voxel::Side side;
            std::string_view name;
        };

        constexpr std::array<SideNameRow, voxel::kFaceSides> kSideNames{{
            {voxel::Side::Top, "top"},
            {voxel::Side::Bottom, "bottom"},
            {voxel::Side::Left, "left"},
            {voxel::Side::Right, "right"}}};

        static_assert(enums::tagsInOrder(kSideNames, &SideNameRow::side));

        [[nodiscard]] std::string_view getSideNamed(const voxel::Side side)
        {
            return enums::lookup(kSideNames, side).name;
        }

        [[nodiscard]] std::string_view getEdgeNamed(const voxel::EdgeKind edge)
        {
            return edge == voxel::EdgeKind::Interior ? "inward" : "outward";
        }
    }

    std::vector<WeaveGap> getMissingRules(
        const std::vector<voxelmap::FaceRef> &faces,
        const tile::TileRules &rules,
        const CornerSeams corners)
    {
        const auto tilesByDomain = getRuledTilesByDomain(rules);

        std::vector<WeaveGap> wantedGaps;

        if (tilesByDomain.empty())
        {
            wantedGaps.push_back(
                WeaveGap{
                    .troubleFailure = SolveFailure::EmptyDomain,
                    .unsatisfiedAtlas = tilemap::Atlas::Wall});

            return wantedGaps;
        }

        std::set<tilemap::TileEdge> namedEdges;

        for (const auto &seam : getFaceAdjacency(faces, corners))
        {
            const auto asks = DomainKey{
                atlasOf(faces[seam.faceA].side),
                faces[seam.faceA].cell.material.kind};
            const auto meets = DomainKey{
                atlasOf(faces[seam.faceB].side),
                faces[seam.faceB].cell.material.kind};

            if (!tilesByDomain.contains(asks) || !tilesByDomain.contains(meets)
                || namedEdges.contains(seam.edgeA))
            {
                continue;
            }

            const auto theirs =
                tilesFor(rules, tilesByDomain.at(meets), faces[seam.faceB]);
            const auto anySatisfied = std::ranges::any_of(
                tilesFor(rules, tilesByDomain.at(asks), faces[seam.faceA]),
                [&](const tilemap::Tile hereTile)
                {
                    return std::ranges::any_of(
                        theirs,
                        [&](const tilemap::Tile thereTile)
                        {
                            return edgesCompatible(
                                rules,
                                hereTile,
                                seam.edgeA,
                                thereTile,
                                seam.edgeB);
                        });
                });

            if (!anySatisfied)
            {
                namedEdges.insert(seam.edgeA);
                wantedGaps.push_back(
                    WeaveGap{
                        .troubleFailure = SolveFailure::IncompatibleEdge,
                        .unsatisfiedEdge = seam.edgeA});
            }
        }

        return wantedGaps;
    } // GCOVR_EXCL_LINE

    std::string getWeaveErrorMessage(
        const std::vector<voxelmap::FaceRef> &faces,
        const tile::TileRules &rules,
        const TileSolve &solve,
        const CornerSeams corners)
    {
        const auto alone =
            solve.skippedFaceCount == 0
                                    ? std::string()
                : ("; " + std::to_string(solve.skippedFaceCount)
                   + " faces left as they were, no tile of their "
                     "atlas given to what they are made of having "
                     "rules");

        if (solve.tiles.has_value())
        {
            return "wove a texturing that keeps the rules" + alone;
        }

        std::string message = "no texturing";

        for (const auto &want : getMissingRules(faces, rules, corners))
        {
            message += want.troubleFailure == SolveFailure::EmptyDomain
                     ? "; no tile of either atlas has rules yet"
                     : ("; nothing may meet along a "
                           + std::string(
                               getSideNamed(want.unsatisfiedEdge.side))
                           + " "
                           + std::string(
                               getEdgeNamed(want.unsatisfiedEdge.edge))
                           + " edge");
        }

        if (solve.troubleFailure == SolveFailure::Unsatisfiable)
        {
            message += "; no way of laying them all keeps every rule "
                    "at once"
                    + getWhereNamed(solve.conflictFaces);
        }

        if (solve.troubleFailure == SolveFailure::IncompatibleEdge
            && message.size() == std::string("no texturing").size())
        {
            message += "; nothing either face is left with may meet "
                    "along a "
                    + std::string(
                        getSideNamed(solve.unsatisfiedEdge.side))
                    + " "
                    + std::string(
                        getEdgeNamed(solve.unsatisfiedEdge.edge))
                    + " edge" + getWhereNamed(solve.conflictFaces);
        }

        if (solve.troubleFailure == SolveFailure::EmptyDomain
            && message.size() == std::string("no texturing").size())
        {
            message += "; some face lies at a rim no "
                    + std::string(
                        solve.unsatisfiedAtlas == tilemap::Atlas::Floor
                                                ? "flat"
                                                : "upright")
                    + " tile is allowed to lie at"
                    + getWhereNamed(solve.conflictFaces);
        }

        return message + alone;
    } // GCOVR_EXCL_LINE

}
