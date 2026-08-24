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

        constexpr std::uint64_t kStairWeight = 5;
        constexpr std::uint64_t kAcrossWeight = 2;

        constexpr std::int32_t kLeastBranchRise = 2;
        constexpr std::int32_t kMostBranchRise = 8;

        constexpr std::int32_t kStreetTries = 8;

        constexpr std::size_t kMostPins = 4;

        struct StairStep final
        {
            std::size_t position = 0;
            std::span<const std::size_t> wantedCells{};
        };

        struct Move final
        {
            voxel::VoxelPosition toPosition{};
            std::uint64_t weight = 0;
            std::array<StairStep, kMostPins> stairSteps{};
            std::size_t pinCount = 0;
        };

        [[nodiscard]] voxel::VoxelPosition getSteppedPosition(
            const voxel::VoxelPosition fromPosition,
            const voxel::Facing facing,
            const std::int32_t times)
        {
            const auto step = voxel::stepVectorFor(facing);

            return voxel::VoxelPosition{
                .x = fromPosition.x + (step.x * times),
                .y = fromPosition.y,
                .z = fromPosition.z + (step.z * times)};
        }

        [[nodiscard]] voxel::VoxelPosition getRaisedPosition(
            const voxel::VoxelPosition fromPosition, const std::int32_t levels)
        {
            return voxel::VoxelPosition{
                .x =
                    fromPosition.x, .y =
                        fromPosition.y + levels, .z = fromPosition.z};
        }
    }

    std::size_t getWalkSteps(const ChunkShape shape)
    {
        return static_cast<std::size_t>(
            4 * (shape.width + shape.depth + shape.height));
    }

    bool fits(
        const Board &board,
        const std::size_t position,
        const std::span<const std::size_t> wantedCells)
    {
        return std::ranges::any_of(
            wantedCells,
            [&](const std::size_t value)
            { return board.holds(position, value); });
    }

    std::int32_t getHighestTerrace(
        const CompiledRuleset &compiledRuleset, const ChunkShape shape)
    {
        for (std::int32_t level = shape.height - 1; level >= 0; --level)
        {
            const auto desire =
                compiledRuleset.desireIn(
                    compiledRuleset.districtOf(shape, level));

            for (const std::size_t which : compiledRuleset.getWearing(Role::Bear))
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

            [[nodiscard]] std::optional<voxel::VoxelPosition> street()
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
                        const voxel::VoxelPosition cubePosition{
                            .x = randomX, .y = level, .z = randomZ};

                        if (opens(cubePosition))
                        {
                            return cubePosition;
                        }
                    }
                }

                return std::nullopt;
            }

            [[nodiscard]] bool begin(const voxel::VoxelPosition cubePosition)
            {
                const std::size_t mark = board.getMarkStep();

                std::vector<std::size_t> positions;
                hold(positions, cellOf(shape, cubePosition),
                    rules.getWearing(Role::Room));

                const auto underCell = getRaisedPosition(cubePosition, -1);
                if (stands(underCell))
                {
                    hold(
                        positions,
                        cellOf(shape, underCell),
                        rules.getWearing(Role::Bear));
                }

                if (!settle(rules, shape, board, positions))
                {
                    board.rewind(mark);
                    return false;
                }

                landings.push_back(cellOf(shape, cubePosition));
                walkedPositions.insert(cubePosition);

                return true;
            }

            [[nodiscard]] bool climb(
                const voxel::VoxelPosition fromPosition,
                const std::int32_t upTo)
            {
                voxel::VoxelPosition position = fromPosition;

                for (std::size_t step = 0; step < getWalkSteps(shape); ++step)
                {
                    if (position.y >= upTo)
                    {
                        return true;
                    }

                    auto moves = movesFrom(position);
                    bool went = false;

                    while (!moves.empty())
                    {
                        const std::size_t which = drawn(moves);

                        if (lay(moves[which]))
                        {
                            position = moves[which].toPosition;
                            went = true;
                            break;
                        }

                        moves.erase(
                            moves.begin()
                            + static_cast<std::ptrdiff_t>(which));
                    }

                    if (!went)
                    {
                        stuckPosition = position;
                        return false;
                    }
                }

                stuckPosition = position;
                return false;
            }

            [[nodiscard]] const std::vector<std::size_t> &getLandings() const
            {
                return landings;
            }

            [[nodiscard]] voxel::VoxelPosition getStoppedPosition() const
            {
                return stuckPosition;
            }

        private:
            const CompiledRuleset &rules;
            ChunkShape shape;
            Board &board;
            rng::IRng &rng;

            std::set<voxel::VoxelPosition> walkedPositions{};
            std::vector<std::size_t> landings{};
            voxel::VoxelPosition stuckPosition{};

            [[nodiscard]] bool stands(
                const voxel::VoxelPosition cubePosition) const
            {
                return isWithin(shape, cubePosition);
            }

            [[nodiscard]] bool isUntrodden(
                const voxel::VoxelPosition cubePosition) const
            {
                return !walkedPositions.contains(cubePosition);
            }

            [[nodiscard]] bool wears(
                const voxel::VoxelPosition cubePosition, const Role role) const
            {
                return stands(cubePosition)
                       && fits(
                           board, cellOf(shape,
                               cubePosition), rules.getWearing(role));
            }

            [[nodiscard]] bool opens(
                const voxel::VoxelPosition cubePosition) const
            {
                const auto underCell = getRaisedPosition(cubePosition, -1);

                return wears(cubePosition, Role::Room)
                       && (underCell.y < 0 || wears(underCell, Role::Bear));
            }

            void hold(
                std::vector<std::size_t> &positions,
                const std::size_t position,
                const std::span<const std::size_t> wantedCells)
            {
                board.hold(position, wantedCells);
                positions.push_back(position);
            }

            [[nodiscard]] bool lay(const Move &move)
            {
                const std::size_t mark = board.getMarkStep();

                std::vector<std::size_t> positions;
                for (std::size_t index = 0; index < move.pinCount; ++index)
                {
                    hold(positions, move.stairSteps[index].position,
                    move.stairSteps[index].wantedCells);
                }

                for (const std::size_t position : positions)
                {
                    // GCOVR_EXCL_START
                    if (board.getWave()[position].isEmpty())
                    {
                        board.rewind(mark);
                        return false;
                    }
                    // GCOVR_EXCL_STOP
                }

                if (!settle(rules, shape, board, positions))
                {
                    board.rewind(mark);
                    return false;
                }

                landings.push_back(cellOf(shape, move.toPosition));
                walkedPositions.insert(move.toPosition);

                return true;
            }

            void addStair(
                std::vector<Move> &moves,
                const voxel::VoxelPosition position,
                const voxel::Facing facing) const
            {
                const auto mid = getSteppedPosition(position, facing, 1);
                const auto land = getSteppedPosition(position, facing, 2);
                const auto raisedCell = getRaisedPosition(land, 1);

                if (!stands(mid) || !stands(land) || !stands(raisedCell))
                {
                    return;
                }

                if (!isUntrodden(mid) || !isUntrodden(raisedCell))
                {
                    return;
                }

                const auto steps = rules.getMatching(voxel::Kind::Ramp, facing);

                if (!wears(position, Role::Room)
                    || !fits(board, cellOf(shape, mid), steps)
                    || !wears(land, Role::Land) || !wears(raisedCell,
                        Role::Room))
                {
                    return;
                }

                Move madeMove{.toPosition = raisedCell, .weight = kStairWeight};
                madeMove.stairSteps[madeMove.pinCount++] = StairStep{
                    .position = cellOf(shape, position),
                    .wantedCells = rules.getWearing(Role::Room)};
                madeMove.stairSteps[madeMove.pinCount++] =
                    StairStep{.position = cellOf(shape, mid),
                        .wantedCells = steps};
                madeMove.stairSteps[madeMove.pinCount++] = StairStep{
                    .position = cellOf(shape, land),
                    .wantedCells = rules.getWearing(Role::Land)};
                madeMove.stairSteps[madeMove.pinCount++] = StairStep{
                    .position = cellOf(shape, raisedCell),
                    .wantedCells = rules.getWearing(Role::Room)};

                moves.push_back(madeMove);
            }

            void addAcross(
                std::vector<Move> &moves,
                const voxel::VoxelPosition position,
                const voxel::Facing facing) const
            {
                const auto toPosition = getSteppedPosition(position, facing, 1);
                const auto underCell = getRaisedPosition(toPosition, -1);

                if (!stands(toPosition) || !isUntrodden(toPosition)
                    || !wears(toPosition, Role::Perch))
                {
                    return;
                }

                if (stands(underCell) && !wears(underCell, Role::Bear))
                {
                    return;
                }

                Move madeMove{.toPosition = toPosition,
                    .weight = kAcrossWeight};
                madeMove.stairSteps[madeMove.pinCount++] = StairStep{
                    .position = cellOf(shape, toPosition),
                    .wantedCells = rules.getWearing(Role::Perch)};

                if (stands(underCell))
                {
                    madeMove.stairSteps[madeMove.pinCount++] = StairStep{
                        .position = cellOf(shape, underCell),
                        .wantedCells = rules.getWearing(Role::Bear)};
                }

                moves.push_back(madeMove);
            }

            [[nodiscard]] std::vector<Move> movesFrom(
                const voxel::VoxelPosition position) const
            {
                std::vector<Move> moves;

                for (const voxel::Facing facing : kWaysAbout)
                {
                    addStair(moves, position, facing);
                    addAcross(moves, position, facing);
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

        const std::int32_t roof = getHighestTerrace(compiledRuleset, shape);
        voxel::VoxelPosition stuckPosition{};

        for (std::uint32_t attempt = 0; attempt < kRouteAttempts; ++attempt)
        {
            const std::size_t mark = board.getMarkStep();
            Walker walker(compiledRuleset, shape, board, rng);

            const auto start = walker.street();

            if (!start.has_value() || !walker.begin(*start)
                || !walker.climb(*start, roof))
            {
                stuckPosition = walker.getStoppedPosition();
                board.rewind(mark);
                continue;
            }

            for (std::uint32_t branch = 1; branch < wantedCount; ++branch)
            {
                const auto laidCubes = walker.getLandings();
                const auto fromCube = laidCubes[rng.next() % laidCubes.size()];
                const auto cubePosition = cubeAt(shape, fromCube);
                const auto rise = static_cast<std::int32_t>(
                    rng.next()
                    % static_cast<std::uint64_t>(
                        kMostBranchRise - kLeastBranchRise + 1))
                    + kLeastBranchRise;

                (void)walker.climb(cubePosition, std::min(cubePosition.y + rise,
                    roof));
            }

            return StairResult{.climbed = true, .landings = walker.getLandings()};
        }

        return StairResult{.climbed = false, .stuckPosition = stuckPosition};
    } // GCOVR_EXCL_LINE

}
