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
            voxel::VoxelPosition originPosition;

            std::vector<wfc::Domain> waveDomains{};
            std::vector<bool> settledFlags{};

            std::vector<std::size_t> pinnedIndexes{};
        };

        [[nodiscard]] voxel::VoxelPosition inTheWorld(
            const voxel::VoxelPosition originPosition,
            const voxel::VoxelPosition cubePosition)
        {
            return voxel::VoxelPosition{
                .x = originPosition.x + cubePosition.x,
                .y = originPosition.y + cubePosition.y,
                .z = originPosition.z + cubePosition.z};
        }

        [[nodiscard]] ChunkResult troubleAt(
            const ChunkOutcome outcome,
            const voxel::VoxelPosition originPosition,
            const std::vector<voxel::VoxelPosition> &cubePositions)
        {
            ChunkResult result{.outcome = outcome};

            for (const voxel::VoxelPosition cube : cubePositions)
            {
                result.culpritPositions.push_back(
                    inTheWorld(originPosition, cube));
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
            const voxel::Voxels &hintVoxels)
        {
            for (const auto &[hintPosition, material] : hintVoxels)
            {
                const voxel::VoxelPosition cubePosition{
                    .x = hintPosition.x - growing.originPosition.x,
                    .y = hintPosition.y - growing.originPosition.y,
                    .z = hintPosition.z - growing.originPosition.z};

                if (!within(growing.shape, cubePosition))
                {
                    return troubleAt(
                        ChunkOutcome::HintOutside,
                        growing.originPosition,
                        std::vector<voxel::VoxelPosition>{cubePosition});
                }

                const auto wantedPieces = growing.compiledRuleset.matching(
                    material.kind, material.facing);

                if (wantedPieces.empty())
                {
                    return troubleAt(
                        ChunkOutcome::HintUnknown,
                        growing.originPosition,
                        std::vector<voxel::VoxelPosition>{cubePosition});
                }

                const std::size_t cell = cellOf(growing.shape, cubePosition);

                if (!fits(board, cell, wantedPieces))
                {
                    return troubleAt(
                        ChunkOutcome::HintsConflict,
                        growing.originPosition,
                        std::vector<voxel::VoxelPosition>{cubePosition});
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
                const auto cubePosition = cubeAt(shape, cell);

                if (cubePosition.x + 1 < shape.width)
                {
                    seams.push_back(
                        Seam{
                            .lowIndex = cell,
                            .highIndex = cell + 1,
                            .axis = Axis::Across});
                }

                if (cubePosition.z + 1 < shape.depth)
                {
                    seams.push_back(
                        Seam{
                            .lowIndex = cell,
                            .highIndex = cell + width,
                            .axis = Axis::Along});
                }

                if (cubePosition.y + 1 < shape.height)
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
                        growing.originPosition,
                        std::vector<voxel::VoxelPosition>{
                            cubeAt(growing.shape, seam.lowIndex),
                            cubeAt(growing.shape, seam.highIndex)});
                }
            }

            return std::nullopt;
        }

        [[nodiscard]] std::vector<voxel::VoxelPosition> thinnest(
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

            std::vector<voxel::VoxelPosition> namedPositions;
            for (const auto &[count, cell] : rankedPairs)
            {
                if (namedPositions.size() >= kMaxReportedCulprits)
                {
                    break;
                }

                namedPositions.push_back(
                    inTheWorld(
                        growing.originPosition, cubeAt(growing.shape, cell)));
            }

            return namedPositions;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] std::vector<std::optional<std::size_t>> rollWishes(
            const GrowState &growing, rng::IRng &fillRng)
        {
            std::vector<std::optional<std::size_t>> wishes(
                growing.waveDomains.size());

            for (
                std::size_t cell = 0; cell < growing.waveDomains.size(); ++cell)
            {
                const auto cubePosition = cubeAt(growing.shape, cell);
                const auto desire = growing.compiledRuleset.desireIn(
                    growing.compiledRuleset.districtOf(growing.shape,
                        cubePosition.y));

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
            .originPosition = request.originPosition};

        layDistricts(growing);
        keepDemandsFromTheRim(growing);

        Board board(growing.waveDomains);

        std::vector<std::size_t> everyCell(growing.waveDomains.size());
        std::iota(everyCell.begin(), everyCell.end(), std::size_t{0});

        if (!detail::settle(compiledRuleset, request.shape, board, everyCell))
        {
            return ChunkResult{
                .outcome = ChunkOutcome::Unsatisfiable,
                .culpritPositions = thinnest(growing)};
        }

        if (const auto troubleFailure =
                layHints(growing, board, request.hintVoxels))
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
                .culpritPositions = thinnest(growing)};
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
                ChunkOutcome::NoWayUp,
                request.originPosition,
                std::vector<voxel::VoxelPosition>{laidWays.stuckPosition});
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
                .culpritPositions = thinnest(growing)};
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

            const auto cubePosition = inTheWorld(
                request.originPosition, cubeAt(request.shape, cell));

            result.cubeVoxels[cubePosition] = voxel::VoxelMaterial{
                .kind = prototype.kind, .facing = prototype.facing};
        }

        return result;
    } // GCOVR_EXCL_LINE

}
