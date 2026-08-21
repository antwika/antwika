#include "antwika/voxel/VoxelCube.hpp"

#include <set>
#include <tuple>

#include "antwika/voxel/VoxelStairs.hpp"

namespace antwika::voxel
{

    std::int32_t cubeTop(const std::int32_t cube)
    {
        return (cube * kCubeSide) + kCubeSide - 1;
    }

    std::int32_t cubeIndexOfLevel(const std::int32_t level)
    {
        const auto underLevel = level < 0 ? level - kCubeSide + 1 : level;

        return underLevel / kCubeSide;
    }

    namespace
    {
        [[nodiscard]] std::int32_t lowestOf(const std::int32_t place)
        {
            const auto cubeOffset = place % kCubeSide;

            return place - (
                cubeOffset < 0 ? cubeOffset + kCubeSide : cubeOffset);
        }

        constexpr std::array<VoxelCell, 4> kAboutACubeCells{
            VoxelCell{.x = 1},
            VoxelCell{.x = -1},
            VoxelCell{.z = 1},
            VoxelCell{.z = -1}};

        [[nodiscard]] VoxelCell negated(const VoxelCell stepCell)
        {
            return VoxelCell{.x = -stepCell.x, .z = -stepCell.z};
        }

        [[nodiscard]] bool groundBeside(
            const std::set<VoxelCell> &standingCells,
            const VoxelCell cornerCell,
            const VoxelCell stepCell,
            const std::int32_t reach)
        {
            for (
            std::int32_t alongIndex = 0; alongIndex < kCubeSide; ++alongIndex)
            {
                const auto besideCell = VoxelCell{
                    .x = cornerCell.x + (stepCell.x * kCubeSide)
                         + (stepCell.x == 0 ? alongIndex : 0),
                    .y = reach,
                    .z = cornerCell.z + (stepCell.z * kCubeSide)
                         + (stepCell.z == 0 ? alongIndex : 0)};

                const auto foundCell = standingCells.find(besideCell);

                if (foundCell != standingCells.end()
                    && foundCell->kind != Kind::Ramp)
                {
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]] bool standsBeside(
            const std::set<VoxelCell> &standingCells,
            const VoxelCell cornerCell,
            const VoxelCell stepCell)
        {
            for (std::int32_t upIndex = 0; upIndex < kCubeSide; ++upIndex)
            {
                for (std::int32_t alongIndex = 0; alongIndex < kCubeSide;
                     ++alongIndex)
                {
                    const auto besideCell = VoxelCell{
                        .x = cornerCell.x + (stepCell.x * kCubeSide)
                             + (stepCell.x == 0 ? alongIndex : 0),
                        .y = cornerCell.y + upIndex,
                        .z = cornerCell.z + (stepCell.z * kCubeSide)
                             + (stepCell.z == 0 ? alongIndex : 0)};

                    if (standingCells.contains(besideCell))
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        [[nodiscard]] bool isAutoFacedRamp(
            const std::vector<VoxelCell> &cells, const VoxelCell cell)
        {
            const auto corner = cubeCornerOf(cell);
            auto foundAny = false;

            for (const auto voxel : cells)
            {
                if (cubeCornerOf(voxel) != corner)
                {
                    continue;
                }

                if (voxel.kind != Kind::Ramp
                    || voxel.facing != Facing::Any)
                {
                    return false;
                }

                foundAny = true;
            }

            return foundAny;
        }

        [[nodiscard]] bool standsAs(
            const std::vector<VoxelCell> &cells,
            const VoxelCell cell,
            const std::vector<VoxelCell> &wantCell)
        {
            const auto corner = cubeCornerOf(cell);
            const std::set<VoxelCell> wantedCells(
                wantCell.begin(),
                wantCell.end());
            std::size_t standing = 0;

            for (const auto voxel : cells)
            {
                if (cubeCornerOf(voxel) != corner)
                {
                    continue;
                }

                if (!wantedCells.contains(voxel))
                {
                    return false;
                }

                ++standing;
            }

            return standing == wantedCells.size();
        }
    }

    Side facing(const Side side)
    {
        switch (side)
        {
        case Side::Top:
            return Side::Bottom;
        case Side::Bottom:
            return Side::Top;
        case Side::Left:
            return Side::Right;
        case Side::Right:
            break;
        }

        return Side::Left;
    }

    FaceEdge facing(const FaceEdge edge)
    {
        return FaceEdge{
            .side = facing(edge.side), .edge = edge.edge};
    }

    VoxelCell cubeCornerOf(const VoxelCell cell)
    {
        return VoxelCell{
            .x = lowestOf(cell.x),
            .y = lowestOf(cell.y),
            .z = lowestOf(cell.z)};
    }

    std::vector<VoxelCell> cubeCells(const VoxelCell cornerCell)
    {
        std::vector<VoxelCell> cells;

        cells.reserve(kCubeVoxels);

        for (std::int32_t z = 0; z < kCubeSide; ++z)
        {
            for (std::int32_t y = 0; y < kCubeSide; ++y)
            {
                for (std::int32_t x = 0; x < kCubeSide; ++x)
                {
                    cells.push_back(
                        VoxelCell{
                            .x = cornerCell.x + x,
                            .y = cornerCell.y + y,
                            .z = cornerCell.z + z});
                }
            }
        }

        return cells;
    } // GCOVR_EXCL_LINE

    std::vector<VoxelCell> expandCubesToVoxels(
        const std::vector<VoxelCell> &cells)
    {
        std::vector<VoxelCell> expandedCells;

        expandedCells.reserve(cells.size() * kCubeVoxels);

        for (const auto cell : cells)
        {
            for (const auto voxel : cubeCells(
                     VoxelCell{
                         .x = cell.x * kCubeSide,
                         .y = cell.y * kCubeSide,
                         .z = cell.z * kCubeSide}))
            {
                expandedCells.push_back(voxel);
            }
        }

        return expandedCells;
    } // GCOVR_EXCL_LINE

    VoxelCell rampDirectionFor(
        const std::vector<VoxelCell> &cells, const VoxelCell cell)
    {
        const std::set<VoxelCell> standingCells(
            cells.begin(), cells.end());
        const auto corner = cubeCornerOf(cell);
        const auto top = corner.y + kCubeSide - 1;

        for (const auto wantsAWayIn : {true, false})
        {
            for (const auto reach : {top, corner.y})
            {
                for (const auto step : kAboutACubeCells)
                {
                    if (!groundBeside(standingCells, corner, step, reach))
                    {
                        continue;
                    }

                    if (wantsAWayIn
                        && standsBeside(
                            standingCells, corner, negated(step)))
                    {
                        continue;
                    }

                    return step;
                }
            }
        }

        return kAboutACubeCells.front();
    }

    std::vector<VoxelCell> cubeVoxels(
        const VoxelCell cornerCell, const Kind kind, const VoxelCell climbCell)
    {
        if (kind != Kind::Ramp)
        {
            auto grownCells = cubeCells(cornerCell);

            for (auto &cell : grownCells)
            {
                cell.kind = kind;
            }

            return grownCells;
        }

        const auto alongX = climbCell.x != 0;
        const auto forward = alongX ? climbCell.x > 0 : climbCell.z > 0;
        const auto lowStep = forward ? 0 : kCubeSide - 1;
        const auto highStep = forward ? kCubeSide - 1 : 0;

        std::vector<VoxelCell> grownCells;

        for (std::int32_t acrossIndex = 0; acrossIndex < kCubeSide;
             ++acrossIndex)
        {
            for (const auto &[step, upStep] :
                 {std::pair{lowStep, 0},
                  std::pair{highStep, 0},
                  std::pair{highStep, kCubeSide - 1}})
            {
                grownCells.push_back(
                    VoxelCell{
                        .x = cornerCell.x + (alongX ? step : acrossIndex),
                        .y = cornerCell.y + upStep,
                        .z = cornerCell.z + (alongX ? acrossIndex : step),
                        .kind = Kind::Ramp});
            }
        }

        return grownCells;
    } // GCOVR_EXCL_LINE

    std::vector<VoxelCell> withBlockAt(
        const std::vector<VoxelCell> &cells,
        const VoxelCell cell,
        const Kind kind,
        const Facing facingOverride)
    {
        auto updatedCells = withoutBlockAt(cells, cell);
        const auto climb = facingOverride == Facing::Any
                         ? rampDirectionFor(cells, cell)
                         : stepVectorFor(facingOverride);

        for (auto voxel : cubeVoxels(cubeCornerOf(cell), kind, climb))
        {
            voxel.facing =
                kind == Kind::Ramp ? facingOverride : Facing::Any;

            updatedCells.push_back(voxel);
        }

        return updatedCells;
    } // GCOVR_EXCL_LINE

    std::vector<VoxelCell> withRampsRebuilt(
        const std::vector<VoxelCell> &cells, const VoxelCell cell)
    {
        const auto corner = cubeCornerOf(cell);
        auto updatedCells = cells;

        for (const auto step : kAboutACubeCells)
        {
            const auto besideCell = VoxelCell{
                .x = corner.x + (step.x * kCubeSide),
                .y = corner.y,
                .z = corner.z + (step.z * kCubeSide)};

            if (!isAutoFacedRamp(updatedCells, besideCell))
            {
                continue;
            }

            const auto want = cubeVoxels(
                cubeCornerOf(besideCell),
                Kind::Ramp,
                rampDirectionFor(updatedCells, besideCell));

            if (standsAs(updatedCells, besideCell, want))
            {
                continue;
            }

            updatedCells = withBlockAt(updatedCells, besideCell, Kind::Ramp,
                Facing::Any);
        }

        return updatedCells;
    } // GCOVR_EXCL_LINE

    std::vector<VoxelCell> withoutBlockAt(
        const std::vector<VoxelCell> &cells, const VoxelCell cell)
    {
        const auto block = cubeCells(cubeCornerOf(cell));
        const std::set<VoxelCell> goingCells(block.begin(), block.end());

        std::vector<VoxelCell> keptCells;

        for (const auto voxel : cells)
        {
            if (!goingCells.contains(voxel))
            {
                keptCells.push_back(voxel);
            }
        }

        return keptCells;
    } // GCOVR_EXCL_LINE

}
