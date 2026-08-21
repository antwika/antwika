#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <vector>

#include <antwika/pathfinding/GridPos.hpp>
#include <antwika/pathfinding/IWalkGraph.hpp>
#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/voxel/VoxelStairs.hpp>
#include <antwika/worldgen/ChunkShape.hpp>

namespace antwika::worldgen::fakes
{

    using antwika::pathfinding::GridPos;
    using antwika::pathfinding::IWalkGraph;
    using antwika::voxel::Facing;
    using antwika::voxel::Kind;
    using antwika::voxel::VoxelCell;

    inline constexpr std::array<Facing, 4> kEveryWayAboutFacings{
        Facing::East, Facing::West, Facing::North, Facing::South};

    class FakeChunkWalkGraph final : public IWalkGraph
    {
    public:
        FakeChunkWalkGraph(
            const ChunkShape shape, const std::vector<VoxelCell> &cubeCells)
            : shape(shape)
        {
            for (const VoxelCell cube : cubeCells)
            {
                stoodCells[GridPos{
                    .x = cube.x, .y = cube.y, .z = cube.z}] = cube;
            }
        }

        [[nodiscard]] bool within(const GridPos gridPosition) const
        {
            return gridPosition.x >= 0
                   && gridPosition.x < shape.width
                   && gridPosition.y >= 0
                   && gridPosition.y < shape.height && gridPosition.z >= 0
                   && gridPosition.z < shape.depth;
        }

        [[nodiscard]] bool roomy(const GridPos gridPosition) const
        {
            if (!within(gridPosition))
            {
                return false;
            }

            const auto foundCell = stoodCells.find(gridPosition);

            return foundCell == stoodCells.end()
                   || foundCell->second.kind == Kind::Ladder;
        }

        [[nodiscard]] bool bears(const GridPos gridPosition) const
        {
            const auto foundCell = stoodCells.find(gridPosition);

            return foundCell != stoodCells.end()
                   && foundCell->second.kind == Kind::Normal;
        }

        [[nodiscard]] std::vector<GridPos> neighbors(
            const GridPos fromPos) const override
        {
            std::vector<GridPos> foundPoses;

            if (laddered(fromPos) && roomy(above(fromPos)))
            {
                foundPoses.push_back(above(fromPos));
            }

            for (const Facing facing : kEveryWayAboutFacings)
            {
                const auto besidePos = stepped(fromPos, facing, 1);

                if (roomy(besidePos)
                    && (bears(below(besidePos)) || laddered(besidePos)))
                {
                    foundPoses.push_back(besidePos);
                }

                const auto land = stepped(fromPos, facing, 2);
                const auto ontoPos = above(land);

                if (climbs(besidePos, facing) && bears(land) && roomy(ontoPos))
                {
                    foundPoses.push_back(ontoPos);
                }
            }

            return foundPoses;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] std::vector<GridPos> streets() const
        {
            std::vector<GridPos> wayPositions;

            for (std::int32_t x = 0; x < shape.width; ++x)
            {
                for (std::int32_t z = 0; z < shape.depth; ++z)
                {
                    for (std::int32_t y = 0; y < shape.height; ++y)
                    {
                        const GridPos gridPosition{.x = x, .y = y, .z = z};

                        if (roomy(gridPosition) && bears(below(gridPosition)))
                        {
                            wayPositions.push_back(gridPosition);
                            break;
                        }
                    }
                }
            }

            return wayPositions;
        } // GCOVR_EXCL_LINE

    private:
        ChunkShape shape;
        std::map<GridPos, VoxelCell> stoodCells{};

        [[nodiscard]] bool laddered(const GridPos gridPosition) const
        {
            const auto foundPoses = stoodCells.find(gridPosition);

            return foundPoses != stoodCells.end()
                   && foundPoses->second.kind == Kind::Ladder;
        }

        [[nodiscard]] bool climbs(
            const GridPos gridPosition, const Facing facing) const
        {
            const auto foundPoses = stoodCells.find(gridPosition);

            return foundPoses != stoodCells.end()
                   && foundPoses->second.kind == Kind::Ramp
                   && foundPoses->second.facing == facing;
        }

        [[nodiscard]] static GridPos above(const GridPos gridPosition)
        {
            return GridPos{
                .x = gridPosition.x,
                .y = gridPosition.y + 1,
                .z = gridPosition.z};
        }

        [[nodiscard]] static GridPos below(const GridPos gridPosition)
        {
            return GridPos{
                .x = gridPosition.x,
                .y = gridPosition.y - 1,
                .z = gridPosition.z};
        }

        [[nodiscard]] static GridPos stepped(
            const GridPos gridPosition,
            const Facing facing,
            const std::int32_t times)
        {
            const auto step = voxel::stepVectorFor(facing);

            return GridPos{
                .x = gridPosition.x + (step.x * times),
                .y = gridPosition.y,
                .z = gridPosition.z + (step.z * times)};
        }
    };

}
