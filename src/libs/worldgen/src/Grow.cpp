#include "antwika/worldgen/Grow.hpp"

#include <algorithm>
#include <numeric>
#include <optional>
#include <utility>

#include <antwika/rng/SplitMix64Rng.hpp>
#include <antwika/wfc/AdjacencyConstraint.hpp>
#include <antwika/wfc/ConstraintRefs.hpp>
#include <antwika/wfc/Domain.hpp>
#include <antwika/wfc/SolveResult.hpp>
#include <antwika/wfc/Solver.hpp>
#include <antwika/wfc/SolverLimits.hpp>
#include <antwika/worldgen/ruleset/CompiledRuleset.hpp>

#include "Lattice.hpp"
#include "Stairs.hpp"
#include "antwika/worldgen/WorldgenError.hpp"

namespace antwika::worldgen
{

    namespace
    {
        using detail::Board;
        using detail::fits;

        struct Seam final
        {
            std::size_t lowIndex;
            std::size_t highIndex;
            Axis axis;
        };

        struct GrowState final
        {
            const CompiledRuleset &compiledRuleset;
            ChunkShape shape;
            voxel::VoxelCell originCell;

            std::vector<wfc::Domain> waveDomains{};
            std::vector<bool> settledFlags{};

            std::vector<std::size_t> pinnedIndexes{};
        };

        [[nodiscard]] voxel::VoxelCell inTheWorld(
            const voxel::VoxelCell originCell, const voxel::VoxelCell cubeCell)
        {
            return voxel::VoxelCell{
                .x = originCell.x + cubeCell.x,
                .y = originCell.y + cubeCell.y,
                .z = originCell.z + cubeCell.z};
        }

        [[nodiscard]] ChunkResult troubleAt(
            const ChunkOutcome outcome,
            const voxel::VoxelCell originCell,
            const std::vector<voxel::VoxelCell> &cubeCells)
        {
            ChunkResult result{.outcome = outcome};

            for (const voxel::VoxelCell cube : cubeCells)
            {
                result.culpritCells.push_back(inTheWorld(originCell, cube));
            }

            return result;
        } // GCOVR_EXCL_LINE

        void keepDemandsFromTheRim(GrowState &growing)
        {
            const ChunkShape shape = growing.shape;

            for (
                std::size_t cell = 0; cell < growing.waveDomains.size(); ++cell)
            {
                const auto cube = cubeAt(shape, cell);

                const std::array<std::pair<Face, bool>, 4> rims{
                    std::pair{Face::East, cube.x + 1 >= shape.width},
                    std::pair{Face::West, cube.x == 0},
                    std::pair{Face::South, cube.z + 1 >= shape.depth},
                    std::pair{Face::North, cube.z == 0}};

                for (const auto &[face, atRim] : rims)
                {
                    if (!atRim)
                    {
                        continue;
                    }

                    std::vector<std::size_t> domainValues(
                        growing.waveDomains[cell].begin(),
                        growing.waveDomains[cell].end());

                    for (const std::size_t which : domainValues)
                    {
                        const Socket shownSocket =
                            growing.compiledRuleset.at(which)
                                .sockets[static_cast<std::size_t>(face)];

                        if (isDemand(shownSocket))
                        {
                            growing.waveDomains[cell].remove(which);
                        }
                    }
                }
            }
        }

        void layDistricts(GrowState &growing)
        {
            const std::size_t count = growing.compiledRuleset.size();

            growing.waveDomains.assign(
                cubeCount(growing.shape), wfc::Domain(count));
            growing.settledFlags.assign(cubeCount(growing.shape), false);

            for (
                std::size_t cell = 0; cell < growing.waveDomains.size(); ++cell)
            {
                const auto cube = cubeAt(growing.shape, cell);
                const auto desire = growing.compiledRuleset.desireIn(
                    growing.compiledRuleset.districtOf(growing.shape, cube.y));

                for (std::size_t which = 0; which < count; ++which)
                {
                    if (desire[which] == 0)
                    {
                        growing.waveDomains[cell].remove(which);
                    }
                }
            }
        }

        [[nodiscard]] std::optional<ChunkResult> layHints(
            GrowState &growing,
            Board &board,
            const std::vector<voxel::VoxelCell> &hintCells)
        {
            for (const voxel::VoxelCell hint : hintCells)
            {
                const voxel::VoxelCell cubeCell{
                    .x = hint.x - growing.originCell.x,
                    .y = hint.y - growing.originCell.y,
                    .z = hint.z - growing.originCell.z,
                    .kind = hint.kind,
                    .facing = hint.facing};

                if (!within(growing.shape, cubeCell))
                {
                    return troubleAt(
                        ChunkOutcome::HintOutside,
                        growing.originCell,
                        {cubeCell});
                }

                const auto wantedPieces =
                    growing.compiledRuleset.matching(hint.kind, hint.facing);

                if (wantedPieces.empty())
                {
                    return troubleAt(
                        ChunkOutcome::HintUnknown,
                        growing.originCell,
                        {cubeCell});
                }

                const std::size_t cell = cellOf(growing.shape, cubeCell);

                if (!fits(board, cell, wantedPieces))
                {
                    return troubleAt(
                        ChunkOutcome::HintsConflict,
                        growing.originCell,
                        {cubeCell});
                }

                board.hold(cell, wantedPieces);
                growing.settledFlags[cell] = true;
                growing.pinnedIndexes.push_back(cell);
            }

            return std::nullopt;
        }

        [[nodiscard]] std::vector<Seam> seamsOf(const ChunkShape shape)
        {
            const auto width = static_cast<std::size_t>(shape.width);
            const auto depth = static_cast<std::size_t>(shape.depth);

            std::vector<Seam> seams;
            seams.reserve(cubeCount(shape) * 3);

            for (std::size_t cell = 0; cell < cubeCount(shape); ++cell)
            {
                const auto cubeCell = cubeAt(shape, cell);

                if (cubeCell.x + 1 < shape.width)
                {
                    seams.push_back(
                        Seam{
                            .lowIndex = cell,
                            .highIndex = cell + 1,
                            .axis = Axis::Across});
                }

                if (cubeCell.z + 1 < shape.depth)
                {
                    seams.push_back(
                        Seam{
                            .lowIndex = cell,
                            .highIndex = cell + width,
                            .axis = Axis::Along});
                }

                if (cubeCell.y + 1 < shape.height)
                {
                    seams.push_back(
                        Seam{
                            .lowIndex = cell,
                            .highIndex = cell + (width * depth),
                            .axis = Axis::Upright});
                }
            }

            return seams;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] bool anyPairFits(
            const CompiledRuleset &compiledRuleset,
            const std::vector<wfc::Domain> &waveDomains,
            const Seam seam)
        {
            const auto &table = compiledRuleset.tableAlong(seam.axis);

            for (const std::size_t lowValue : waveDomains[seam.lowIndex])
            {
                for (const std::size_t highValue : waveDomains[seam.highIndex])
                {
                    if (table.compatible(lowValue, highValue))
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        [[nodiscard]] std::optional<ChunkResult> weighSettledSeams(
            const GrowState &growing, const std::vector<Seam> &seams)
        {
            for (const Seam seam : seams)
            {
                if (!growing.settledFlags[seam.lowIndex]
                    || !growing.settledFlags[seam.highIndex])
                {
                    continue;
                }

                if (!anyPairFits(
                        growing.compiledRuleset, growing.waveDomains, seam))
                {
                    return troubleAt(
                        ChunkOutcome::HintsConflict,
                        growing.originCell,
                        {cubeAt(growing.shape, seam.lowIndex),
                         cubeAt(growing.shape, seam.highIndex)});
                }
            }

            return std::nullopt;
        }

        [[nodiscard]] std::vector<voxel::VoxelCell> thinnest(
            const GrowState &growing)
        {
            std::vector<std::pair<std::size_t, std::size_t>> rankedPairs;

            for (
                std::size_t cell = 0; cell < growing.waveDomains.size(); ++cell)
            {
                if (growing.settledFlags[cell])
                {
                    continue;
                }

                rankedPairs.emplace_back(growing.waveDomains[cell].count(),
                    cell);
            }

            std::ranges::sort(rankedPairs);

            std::vector<voxel::VoxelCell> namedCells;
            for (const auto &[count, cell] : rankedPairs)
            {
                if (namedCells.size() >= kMaxReportedCulprits)
                {
                    break;
                }

                namedCells.push_back(
                    inTheWorld(
                        growing.originCell, cubeAt(growing.shape, cell)));
            }

            return namedCells;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] std::vector<std::optional<std::size_t>> rollWishes(
            const GrowState &growing, rng::IRng &fillRng)
        {
            std::vector<std::optional<std::size_t>> wishes(
                growing.waveDomains.size());

            for (
                std::size_t cell = 0; cell < growing.waveDomains.size(); ++cell)
            {
                const auto cubeCell = cubeAt(growing.shape, cell);
                const auto desire = growing.compiledRuleset.desireIn(
                    growing.compiledRuleset.districtOf(growing.shape,
                        cubeCell.y));

                std::uint64_t total = 0;
                for (const std::size_t which : growing.waveDomains[cell])
                {
                    total += desire[which];
                }

                const std::uint64_t drawnValue =
                    total == 0 ? 0 : fillRng.next() % total;

                std::uint64_t walkedWeight = 0;
                for (const std::size_t which : growing.waveDomains[cell])
                {
                    walkedWeight += desire[which];

                    if (total != 0 && drawnValue < walkedWeight)
                    {
                        wishes[cell] = which;
                        break;
                    }
                }
            }

            return wishes;
        } // GCOVR_EXCL_LINE
    }

    ChunkResult growChunk(
        const CompiledRuleset &compiledRuleset, const ChunkRequest &request)
    {
        rng::SplitMix64Rng waysRng(request.seed);
        rng::SplitMix64Rng fillRng(request.seed ^ kFillSalt);

        return growChunk(compiledRuleset, request, waysRng, fillRng);
    }

    ChunkResult growChunk(
        const CompiledRuleset &compiledRuleset,
        const ChunkRequest &request,
        rng::IRng &waysRng,
        rng::IRng &fillRng)
    {
        GrowState growing{
            .compiledRuleset = compiledRuleset,
            .shape = request.shape,
            .originCell = request.originCell};

        layDistricts(growing);
        keepDemandsFromTheRim(growing);

        Board board(growing.waveDomains);

        std::vector<std::size_t> everyCell(growing.waveDomains.size());
        std::iota(everyCell.begin(), everyCell.end(), std::size_t{0});

        if (!detail::settle(compiledRuleset, request.shape, board, everyCell))
        {
            return ChunkResult{
                .outcome = ChunkOutcome::Unsatisfiable,
                .culpritCells = thinnest(growing)};
        }

        if (const auto troubleFailure =
                layHints(growing, board, request.hintCells))
        {
            return *troubleFailure;
        }

        const auto seams = seamsOf(request.shape);

        if (const auto troubleFailure = weighSettledSeams(growing, seams))
        {
            return *troubleFailure;
        }

        if (!detail::settle(
                compiledRuleset, request.shape, board, growing.pinnedIndexes))
        {
            return ChunkResult{
                .outcome = ChunkOutcome::HintsConflict,
                .culpritCells = thinnest(growing)};
        }

        const auto laidWays =
            detail::layWays(
                compiledRuleset,
                request.shape,
                board,
                request.ways,
                waysRng);

        if (!laidWays.climbed)
        {
            return troubleAt(
                ChunkOutcome::NoWayUp, request.originCell,
                {laidWays.stuckCell});
        }

        for (const std::size_t cell : laidWays.landings)
        {
            growing.settledFlags[cell] = true;
        }

        std::vector<wfc::AdjacencyConstraint> adjacencyConstraints;
        adjacencyConstraints.reserve(seams.size());

        for (const Seam seam : seams)
        {
            adjacencyConstraints.emplace_back(
                seam.lowIndex, seam.highIndex, compiledRuleset.sharedTableAlong(
                    seam.axis));
        }

        auto wishes = rollWishes(growing, fillRng);

        const wfc::Solver solver(
            growing.waveDomains,
            wfc::referencesTo(adjacencyConstraints),
            {},
            wfc::SolverLimits{.maxSteps = request.maxSteps},
            std::move(wishes));

        const auto solution = solver.solve();

        if (solution.outcome != wfc::SolveOutcome::Solved)
        {
            return ChunkResult{
                .outcome = solution.outcome == wfc::SolveOutcome::Unsatisfiable
                         ? ChunkOutcome::Unsatisfiable
                         : ChunkOutcome::LimitExceeded,
                .culpritCells = thinnest(growing)};
        }

        ChunkResult result;

        for (std::size_t cell = 0; cell < solution.assignment.size(); ++cell)
        {
            const Prototype &prototype = compiledRuleset.at(
                solution.assignment[cell]);

            if (prototype.air)
            {
                continue;
            }

            auto cubeCell =
                inTheWorld(request.originCell, cubeAt(request.shape, cell));
            cubeCell.kind = prototype.kind;
            cubeCell.facing = prototype.facing;

            result.cubeCells.push_back(cubeCell);
        }

        return result;
    } // GCOVR_EXCL_LINE

}
