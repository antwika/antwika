#include <cstddef>
#include <set>
#include <vector>

#include <antwika/gfx/Math3D.hpp>

#include <antwika/voxelmap/Voxel.hpp>

#include "VoxelDetail.hpp"

namespace antwika::voxelmap
{
    using namespace voxeldetail;

    namespace
    {

        [[nodiscard]] bool facedByAClimb(
            const voxel::VoxelCell climbCell, const gfx::Vec3 normal)
        {
            return ((normal.x * static_cast<float>(climbCell.x))
                    + (normal.z * static_cast<float>(climbCell.z)))
                   < 0.0F;
        }

    }

    std::vector<FaceRef> visibleFacesOf(
        const std::vector<voxel::VoxelCell> &cells)
    {
        const std::set<voxel::VoxelCell> filledCells(
            cells.begin(),
            cells.end());

        std::vector<FaceRef> faces;

        for (const auto cell : cells)
        {
            const auto climb = cell.kind == voxel::Kind::Ramp
                             ? voxel::inferredRampDirection(
                                         filledCells, cell)
                                   : voxel::VoxelCell{};
            const auto level = voxel::stairHalfOf(filledCells, cell);

            for (std::size_t side = 0; side < kFaces; ++side)
            {
                const auto neighbourCell = offsetBy(
                    cell, kVoxelFaces[side].neighbourOffsetCell);
                const auto beyondKind =
                    effectiveKindAt(filledCells, neighbourCell);
                const auto footprint =
                    kVoxelFaces[side].normal.y > 0.0F
                    && cell.kind == voxel::Kind::Normal
                    && beyondKind == voxel::Kind::Normal;

                if (beyondKind.has_value()
                    && voxel::occludes(*beyondKind, cell.kind)
                    && !footprint)
                {
                    continue;
                }

                if (kVoxelFaces[side].normal.y > 0.0F
                    && kindAt(filledCells, neighbourCell) == voxel::Kind::Ramp)
                {
                    continue;
                }

                if (kindAt(filledCells, neighbourCell) == voxel::Kind::Ramp
                    && facedByAClimb(
                        voxel::inferredRampDirection(filledCells,
                        neighbourCell),
                        kVoxelFaces[side].normal))
                {
                    continue;
                }

                faces.push_back(
                    FaceRef{
                        .cell = cell,
                        .side = side,
                        .climbCell = climb,
                        .levelHalf = level});
            }
        }

        return faces;
    } // GCOVR_EXCL_LINE

}
