#include "antwika/voxel/VoxelOcclusion.hpp"

#include <glm/geometric.hpp>

#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <optional>
#include <vector>

namespace antwika::voxel
{

    namespace
    {

        [[nodiscard]] std::optional<VoxelCell> roofOver(
            const std::set<VoxelCell> &filledCells,
            const VoxelCell cell,
            const std::int32_t lowest)
        {
            for (std::int32_t level = lowest;
                 level < lowest + kRoofSearchLevels;
                 ++level)
            {
                const VoxelCell overheadCell{
                    .x = cell.x, .y = level, .z = cell.z};
                const auto foundCell = filledCells.find(overheadCell);

                if (foundCell != filledCells.end()
                    && foundCell->kind != Kind::Water)
                {
                    return overheadCell;
                }
            }

            return std::nullopt;
        }

        [[nodiscard]] std::set<VoxelCell> shellAbout(
            const std::set<VoxelCell> &filledCells,
            const VoxelCell columnCell,
            const std::int32_t lowest,
            const std::int32_t roofLevel)
        {
            const auto arm =
                static_cast<std::int32_t>(kOcclusionMaskWidth) / 2;

            const auto withinWindow = [columnCell, lowest, roofLevel](
                                          const VoxelCell cell)
            {
                return cell.y >= lowest && cell.y < roofLevel
                       && std::abs(cell.x - columnCell.x) <= arm
                       && std::abs(cell.z - columnCell.z) <= arm;
            };

            const auto standsIn = [&filledCells](const VoxelCell cell)
            {
                const auto foundCell = filledCells.find(cell);

                return foundCell != filledCells.end()
                       && foundCell->kind != Kind::Water;
            };

            std::set<VoxelCell> shellCells;
            const VoxelCell fromCell{
                .x = columnCell.x, .y = lowest, .z = columnCell.z};

            if (!withinWindow(fromCell) || standsIn(fromCell))
            {
                return shellCells;
            }

            std::set<VoxelCell> airCells{fromCell};
            std::vector<VoxelCell> askingCells{fromCell};

            while (!askingCells.empty())
            {
                const auto nextCell = askingCells.back();

                askingCells.pop_back();

                for (const auto way :
                     {VoxelCell{.x = 1}, VoxelCell{.x = -1},
                      VoxelCell{.y = 1}, VoxelCell{.y = -1},
                      VoxelCell{.z = 1}, VoxelCell{.z = -1}})
                {
                    const VoxelCell besideCell{
                        .x = nextCell.x + way.x,
                        .y = nextCell.y + way.y,
                        .z = nextCell.z + way.z};

                    if (!withinWindow(besideCell))
                    {
                        continue;
                    }

                    if (standsIn(besideCell))
                    {
                        shellCells.insert(besideCell);

                        continue;
                    }

                    if (airCells.insert(besideCell).second)
                    {
                        askingCells.push_back(besideCell);
                    }
                }
            }

            return shellCells;
        } // GCOVR_EXCL_LINE

        void liftWhatIsNotTheRoom(
            const std::set<VoxelCell> &filledCells,
            const VoxelCell columnCell,
            const std::int32_t lowest,
            const std::int32_t roofLevel,
            std::set<VoxelCell> &cells)
        {
            const auto shellCells =
                shellAbout(filledCells, columnCell, lowest, roofLevel);

            if (shellCells.empty())
            {
                return;
            }

            const auto arm =
                static_cast<std::int32_t>(kOcclusionMaskWidth) / 2;

            for (const auto &besideCell : filledCells)
            {
                if (cells.size() >= kMaxOccludedVoxels)
                {
                    break;
                }

                if (besideCell.y < lowest || besideCell.y >= roofLevel
                    || besideCell.kind == Kind::Water
                    || std::abs(besideCell.x - columnCell.x) > arm
                    || std::abs(besideCell.z - columnCell.z) > arm
                    || shellCells.contains(besideCell))
                {
                    continue;
                }

                cells.insert(besideCell);
            }
        }

        void liftTheRoof(
            const std::set<VoxelCell> &filledCells,
            const VoxelCell columnCell,
            const std::int32_t roofLevel,
            std::set<VoxelCell> &cells)
        {
            const auto arm =
                static_cast<std::int32_t>(kOcclusionMaskWidth) / 2;

            for (const auto &overheadCell : filledCells)
            {
                if (cells.size() >= kMaxOccludedVoxels)
                {
                    break;
                }

                if (overheadCell.y < roofLevel
                    || overheadCell.kind == Kind::Water
                    || std::abs(overheadCell.x - columnCell.x) > arm
                    || std::abs(overheadCell.z - columnCell.z) > arm)
                {
                    continue;
                }

                cells.insert(overheadCell);
            }
        }

    }

    glm::vec3 lineOfSight(const glm::vec3 standing)
    {
        return standing + glm::vec3{0.0F, kLineOfSightRise, 0.0F};
    }

    glm::vec3 upperLineOfSight(const glm::vec3 standing)
    {
        return standing + glm::vec3{0.0F, kUpperSightRise, 0.0F};
    }

    std::set<VoxelCell> occludingVoxels(
        const std::set<VoxelCell> &filledCells, const glm::vec3 standing)
    {
        std::set<VoxelCell> cells;
        const auto sightPoint = lineOfSight(standing);
        const VoxelCell columnCell{
            .x = static_cast<std::int32_t>(
                std::floor(sightPoint.x / kVoxelSide)),
            .y = static_cast<std::int32_t>(
                std::floor(sightPoint.y / kVoxelSide)),
            .z = static_cast<std::int32_t>(
                std::floor(sightPoint.z / kVoxelSide))};
        const auto overheadCell =
            roofOver(filledCells, columnCell, columnCell.y);

        if (!overheadCell.has_value())
        {
            return cells;
        }

        liftTheRoof(filledCells, columnCell, overheadCell->y, cells);
        liftWhatIsNotTheRoom(
            filledCells, columnCell, columnCell.y, overheadCell->y, cells);

        return cells;
    } // GCOVR_EXCL_LINE

}
