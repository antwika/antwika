#include "Stairs.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <set>

#include <antwika/voxel/VoxelStairs.hpp>
#include <antwika/worldgen/ruleset/CompiledRuleset.hpp>

namespace antwika::worldgen::detail
{

    namespace
    {
        constexpr std::array kWaysAbout{
            voxel::Facing::East,
            voxel::Facing::West,
            voxel::Facing::North,
            voxel::Facing::South};

        constexpr std::uint64_t kLadderWeight = 3;
        constexpr std::uint64_t kStairWeight = 5;
        constexpr std::uint64_t kAcrossWeight = 2;

        constexpr std::int32_t kLeastBranchRise = 2;
        constexpr std::int32_t kMostBranchRise = 8;

        constexpr std::int32_t kStreetTries = 8;

        constexpr std::size_t kMostPins = 4;

        struct StairStep final
        {
            std::size_t cell = 0;
            std::span<const std::size_t> wantedCells{};
        };

        struct Move final
        {
            voxel::VoxelCell toCell{};
            std::uint64_t weight = 0;
            std::array<StairStep, kMostPins> stairSteps{};
            std::size_t pinCount = 0;
        };

        [[nodiscard]] voxel::VoxelCell stepped(
            const voxel::VoxelCell fromCell,
            const voxel::Facing facing,
            const std::int32_t times)
        {
            const auto step = voxel::stepVectorFor(facing);

            return voxel::VoxelCell{
                .x = fromCell.x + (step.x * times),
                .y = fromCell.y,
                .z = fromCell.z + (step.z * times)};
        }

        [[nodiscard]] voxel::VoxelCell raised(
            const voxel::VoxelCell fromCell, const std::int32_t levels)
        {
            return voxel::VoxelCell{
                .x = fromCell.x, .y = fromCell.y + levels, .z = fromCell.z};
        }
    }

    std::size_t walkSteps(const ChunkShape shape)
    {
        return static_cast<std::size_t>(
            4 * (shape.width + shape.depth + shape.height));
    }

    bool fits(
        const Board &board,
        const std::size_t cell,
        const std::span<const std::size_t> wantedCells)
    {
        return std::ranges::any_of(
            wantedCells,
            [&](const std::size_t value)
            { return board.holds(cell, value); });
    }

    std::int32_t highestTerrace(
        const CompiledRuleset &compiledRuleset, const ChunkShape shape)
    {
        for (std::int32_t level = shape.height - 1; level >= 0; --level)
        {
            const auto desire =
                compiledRuleset.desireIn(
                    compiledRuleset.districtOf(shape, level));

            for (const std::size_t which : compiledRuleset.wearing(Role::Bear))
            {
                if (desire[which] != 0)
                {
                    return level;
                }
            }
        }

        return 0;
    }

    namespace
    {
        class Walker final
        {
        public:
            Walker(
                const CompiledRuleset &compiledRuleset,
                const ChunkShape shape,
                Board &board,
                rng::IRng &rng)
                : rules(compiledRuleset), shape(shape), board(board), rng(rng)
            {
            }

            [[nodiscard]] std::optional<voxel::VoxelCell> street()
            {
                for (std::int32_t tries = 0; tries < kStreetTries; ++tries)
                {
                    const auto randomX = static_cast<std::int32_t>(
                        rng.next()
                        % static_cast<std::uint64_t>(shape.width));
                    const auto randomZ = static_cast<std::int32_t>(
                        rng.next()
                        % static_cast<std::uint64_t>(shape.depth));

                    for (std::int32_t level = 0; level < shape.height;
                         ++level)
                    {
                        const voxel::VoxelCell cubeCell{
                            .x = randomX, .y = level, .z = randomZ};

                        if (opens(cubeCell))
                        {
                            return cubeCell;
                        }
                    }
                }

                return std::nullopt;
            }

            [[nodiscard]] bool begin(const voxel::VoxelCell cubeCell)
            {
                const std::size_t mark = board.mark();

                std::vector<std::size_t> cells;
                hold(cells, cellOf(shape, cubeCell), rules.wearing(Role::Room));

                const auto underCell = raised(cubeCell, -1);
                if (stands(underCell))
                {
                    hold(
                        cells,
                        cellOf(shape, underCell),
                        rules.wearing(Role::Bear));
                }

                if (!settle(rules, shape, board, cells))
                {
                    board.rewind(mark);
                    return false;
                }

                landings.push_back(cellOf(shape, cubeCell));
                walkedCells.insert(cubeCell);

                return true;
            }

            [[nodiscard]] bool climb(
                const voxel::VoxelCell fromCell, const std::int32_t upTo)
            {
                voxel::VoxelCell cell = fromCell;

                for (std::size_t step = 0; step < walkSteps(shape); ++step)
                {
                    if (cell.y >= upTo)
                    {
                        return true;
                    }

                    auto moves = movesFrom(cell);
                    bool went = false;

                    while (!moves.empty())
                    {
                        const std::size_t which = drawn(moves);

                        if (lay(moves[which]))
                        {
                            cell = moves[which].toCell;
                            went = true;
                            break;
                        }

                        moves.erase(
                            moves.begin()
                            + static_cast<std::ptrdiff_t>(which));
                    }

                    if (!went)
                    {
                        stuckCell = cell;
                        return false;
                    }
                }

                stuckCell = cell;
                return false;
            }

            [[nodiscard]] const std::vector<std::size_t> &laid() const
            {
                return landings;
            }

            [[nodiscard]] voxel::VoxelCell stopped() const
            {
                return stuckCell;
            }

        private:
            const CompiledRuleset &rules;
            ChunkShape shape;
            Board &board;
            rng::IRng &rng;

            std::set<voxel::VoxelCell> walkedCells{};
            std::vector<std::size_t> landings{};
            voxel::VoxelCell stuckCell{};

            [[nodiscard]] bool stands(const voxel::VoxelCell cubeCell) const
            {
                return within(shape, cubeCell);
            }

            [[nodiscard]] bool untrodden(const voxel::VoxelCell cubeCell) const
            {
                return !walkedCells.contains(cubeCell);
            }

            [[nodiscard]] bool wears(
                const voxel::VoxelCell cubeCell, const Role role) const
            {
                return stands(cubeCell)
                       && fits(
                           board, cellOf(shape, cubeCell), rules.wearing(role));
            }

            [[nodiscard]] bool opens(const voxel::VoxelCell cubeCell) const
            {
                const auto underCell = raised(cubeCell, -1);

                return wears(cubeCell, Role::Room)
                       && (underCell.y < 0 || wears(underCell, Role::Bear));
            }

            void hold(
                std::vector<std::size_t> &cells,
                const std::size_t cell,
                const std::span<const std::size_t> wantedCells)
            {
                board.hold(cell, wantedCells);
                cells.push_back(cell);
            }

            [[nodiscard]] bool lay(const Move &move)
            {
                const std::size_t mark = board.mark();

                std::vector<std::size_t> cells;
                for (std::size_t index = 0; index < move.pinCount; ++index)
                {
                    hold(cells, move.stairSteps[index].cell,
                    move.stairSteps[index].wantedCells);
                }

                for (const std::size_t cell : cells)
                {
                    // GCOVR_EXCL_START
                    if (board.wave()[cell].isEmpty())
                    {
                        board.rewind(mark);
                        return false;
                    }
                    // GCOVR_EXCL_STOP
                }

                if (!settle(rules, shape, board, cells))
                {
                    board.rewind(mark);
                    return false;
                }

                landings.push_back(cellOf(shape, move.toCell));
                walkedCells.insert(move.toCell);

                return true;
            }

            void addLadder(
                std::vector<Move> &moves, const voxel::VoxelCell cells) const
            {
                const auto raisedCell = raised(cells, 1);

                if (!stands(raisedCell) || !untrodden(raisedCell))
                {
                    return;
                }

                if (!wears(cells, Role::Climb)
                    || !wears(raisedCell, Role::Perch))
                {
                    return;
                }

                Move madeMove{.toCell = raisedCell, .weight = kLadderWeight};
                madeMove.stairSteps[madeMove.pinCount++] = StairStep{
                    .cell = cellOf(shape, cells),
                    .wantedCells = rules.wearing(Role::Climb)};
                madeMove.stairSteps[madeMove.pinCount++] = StairStep{
                    .cell = cellOf(shape, raisedCell),
                    .wantedCells = rules.wearing(Role::Perch)};

                moves.push_back(madeMove);
            }

            void addStair(
                std::vector<Move> &moves,
                const voxel::VoxelCell cell,
                const voxel::Facing facing) const
            {
                const auto mid = stepped(cell, facing, 1);
                const auto land = stepped(cell, facing, 2);
                const auto raisedCell = raised(land, 1);

                if (!stands(mid) || !stands(land) || !stands(raisedCell))
                {
                    return;
                }

                if (!untrodden(mid) || !untrodden(raisedCell))
                {
                    return;
                }

                const auto steps = rules.matching(voxel::Kind::Ramp, facing);

                if (!wears(cell, Role::Room)
                    || !fits(board, cellOf(shape, mid), steps)
                    || !wears(land, Role::Land) || !wears(raisedCell,
                        Role::Room))
                {
                    return;
                }

                Move madeMove{.toCell = raisedCell, .weight = kStairWeight};
                madeMove.stairSteps[madeMove.pinCount++] = StairStep{
                    .cell = cellOf(shape, cell),
                    .wantedCells = rules.wearing(Role::Room)};
                madeMove.stairSteps[madeMove.pinCount++] =
                    StairStep{.cell = cellOf(shape, mid), .wantedCells = steps};
                madeMove.stairSteps[madeMove.pinCount++] = StairStep{
                    .cell = cellOf(shape, land),
                    .wantedCells = rules.wearing(Role::Land)};
                madeMove.stairSteps[madeMove.pinCount++] = StairStep{
                    .cell = cellOf(shape, raisedCell),
                    .wantedCells = rules.wearing(Role::Room)};

                moves.push_back(madeMove);
            }

            void addAcross(
                std::vector<Move> &moves,
                const voxel::VoxelCell cell,
                const voxel::Facing facing) const
            {
                const auto toCell = stepped(cell, facing, 1);
                const auto underCell = raised(toCell, -1);

                if (!stands(toCell) || !untrodden(toCell)
                    || !wears(toCell, Role::Perch))
                {
                    return;
                }

                if (stands(underCell) && !wears(underCell, Role::Bear))
                {
                    return;
                }

                Move madeMove{.toCell = toCell, .weight = kAcrossWeight};
                madeMove.stairSteps[madeMove.pinCount++] = StairStep{
                    .cell = cellOf(shape, toCell),
                    .wantedCells = rules.wearing(Role::Perch)};

                if (stands(underCell))
                {
                    madeMove.stairSteps[madeMove.pinCount++] = StairStep{
                        .cell = cellOf(shape, underCell),
                        .wantedCells = rules.wearing(Role::Bear)};
                }

                moves.push_back(madeMove);
            }

            [[nodiscard]] std::vector<Move> movesFrom(
                const voxel::VoxelCell cell) const
            {
                std::vector<Move> moves;

                addLadder(moves, cell);

                for (const voxel::Facing facing : kWaysAbout)
                {
                    addStair(moves, cell, facing);
                    addAcross(moves, cell, facing);
                }

                return moves;
            } // GCOVR_EXCL_LINE

            [[nodiscard]] std::size_t drawn(const std::vector<Move> &moves)
            {
                std::uint64_t total = 0;
                for (const Move &move : moves)
                {
                    total += move.weight;
                }

                std::uint64_t left = rng.next() % total;

                for (std::size_t index = 0; index < moves.size(); ++index)
                {
                    if (left < moves[index].weight)
                    {
                        return index;
                    }

                    left -= moves[index].weight;
                }

                return moves.size() - 1; // GCOVR_EXCL_LINE
            }
        };
    }

    StairResult layWays(
        const CompiledRuleset &compiledRuleset,
        const ChunkShape shape,
        Board &board,
        const std::uint32_t wantedCount,
        rng::IRng &rng)
    {
        if (wantedCount == 0)
        {
            return StairResult{.climbed = true};
        }

        const std::int32_t roof = highestTerrace(compiledRuleset, shape);
        voxel::VoxelCell stuckCell{};

        for (std::uint32_t attempt = 0; attempt < kRouteAttempts; ++attempt)
        {
            const std::size_t mark = board.mark();
            Walker walker(compiledRuleset, shape, board, rng);

            const auto start = walker.street();

            if (!start.has_value() || !walker.begin(*start)
                || !walker.climb(*start, roof))
            {
                stuckCell = walker.stopped();
                board.rewind(mark);
                continue;
            }

            for (std::uint32_t branch = 1; branch < wantedCount; ++branch)
            {
                const auto laidCubes = walker.laid();
                const auto fromCube = laidCubes[rng.next() % laidCubes.size()];
                const auto cubeCell = cubeAt(shape, fromCube);
                const auto rise = static_cast<std::int32_t>(
                    rng.next()
                    % static_cast<std::uint64_t>(
                        kMostBranchRise - kLeastBranchRise + 1))
                    + kLeastBranchRise;

                (void)walker.climb(cubeCell, std::min(cubeCell.y + rise, roof));
            }

            return StairResult{.climbed = true, .landings = walker.laid()};
        }

        return StairResult{.climbed = false, .stuckCell = stuckCell};
    } // GCOVR_EXCL_LINE

}
