#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <vector>

#include <antwika/pathfinding/GridPos.hpp>
#include <antwika/pathfinding/IWalkGraph.hpp>
#include <antwika/voxel/VoxelMaterial.hpp>
#include <antwika/voxel/Voxels.hpp>
#include <antwika/voxel/VoxelStairs.hpp>
#include <antwika/worldgen/ChunkShape.hpp>

namespace antwika::worldgen::fakes
{

    using antwika::pathfinding::GridPos;
    using antwika::pathfinding::IWalkGraph;
    using antwika::voxel::Facing;
    using antwika::voxel::Kind;
    using antwika::voxel::VoxelMaterial;
    using antwika::voxel::Voxels;

    inline constexpr std::array<Facing, 4> kEveryWayAboutFacings{
        Facing::East, Facing::West, Facing::North, Facing::South};

    class FakeChunkWalkGraph final : public IWalkGraph
    {
    public:
        FakeChunkWalkGraph(
            const ChunkShape shape, const Voxels &cubeVoxels)
            : shape(shape)
        {
            for (const auto &[position, material] : cubeVoxels)
            {
                stoodMaterials[GridPos{
                    .x = position.x,
                    .y = position.y,
                    .z = position.z}] = material;
            }
        }

        [[nodiscard]] bool isWithin(const GridPos gridPosition) const
        {
            return gridPosition.x >= 0
                   && gridPosition.x < shape.width
                   && gridPosition.y >= 0
                   && gridPosition.y < shape.height && gridPosition.z >= 0
                   && gridPosition.z < shape.depth;
        }

        [[nodiscard]] bool isRoomy(const GridPos gridPosition) const
        {
            if (!isWithin(gridPosition))
            {
                return false;
            }

            const auto foundCell = stoodMaterials.find(gridPosition);

            return foundCell == stoodMaterials.end();
        }

        [[nodiscard]] bool bears(const GridPos gridPosition) const
        {
            const auto foundCell = stoodMaterials.find(gridPosition);

            return foundCell != stoodMaterials.end()
                   && foundCell->second.kind == Kind::Normal;
        }

        [[nodiscard]] std::vector<GridPos> getNeighbors(
            const GridPos fromPos) const override
        {
            std::vector<GridPos> foundPoses;

            for (const Facing facing : kEveryWayAboutFacings)
            {
                const auto besidePos = getSteppedCell(fromPos, facing, 1);

                if (isRoomy(besidePos) && bears(getCellBelow(besidePos)))
                {
                    foundPoses.push_back(besidePos);
                }

                const auto land = getSteppedCell(fromPos, facing, 2);
                const auto ontoPos = getCellAbove(land);

                if (climbs(besidePos, facing) && bears(land) && isRoomy(ontoPos))
                {
                    foundPoses.push_back(ontoPos);
                }
            }

            return foundPoses;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] std::vector<GridPos> getStreets() const
        {
            std::vector<GridPos> wayPositions;

            for (std::int32_t x = 0; x < shape.width; ++x)
            {
                for (std::int32_t z = 0; z < shape.depth; ++z)
                {
                    for (std::int32_t y = 0; y < shape.height; ++y)
                    {
                        const GridPos gridPosition{.x = x, .y = y, .z = z};

                        if (isRoomy(gridPosition) && bears(getCellBelow(gridPosition)))
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
        std::map<GridPos, VoxelMaterial> stoodMaterials{};

        [[nodiscard]] bool climbs(
            const GridPos gridPosition, const Facing facing) const
        {
            const auto foundPoses = stoodMaterials.find(gridPosition);

            return foundPoses != stoodMaterials.end()
                   && foundPoses->second.kind == Kind::Ramp
                   && foundPoses->second.facing == facing;
        }

        [[nodiscard]] static GridPos getCellAbove(const GridPos gridPosition)
        {
            return GridPos{
                .x = gridPosition.x,
                .y = gridPosition.y + 1,
                .z = gridPosition.z};
        }

        [[nodiscard]] static GridPos getCellBelow(const GridPos gridPosition)
        {
            return GridPos{
                .x = gridPosition.x,
                .y = gridPosition.y - 1,
                .z = gridPosition.z};
        }

        [[nodiscard]] static GridPos getSteppedCell(
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
