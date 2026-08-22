#include <cstddef>
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
            const voxel::VoxelPosition climbPosition, const gfx::Vec3 normal)
        {
            return ((normal.x * static_cast<float>(climbPosition.x))
                    + (normal.z * static_cast<float>(climbPosition.z)))
                   < 0.0F;
        }

    }

    std::vector<FaceRef> visibleFacesOf(const voxel::Voxels &voxels)
    {
        std::vector<FaceRef> faces;

        for (const auto &[position, material] : voxels)
        {
            const auto climb = material.kind == voxel::Kind::Ramp
                             ? voxel::inferredRampDirection(voxels, position)
                             : voxel::VoxelPosition{};
            const auto level = voxel::stairHalfOf(voxels, position);

            for (std::size_t side = 0; side < kFaces; ++side)
            {
                const auto neighbourPosition = offsetBy(
                    position, kVoxelFaces[side].neighbourOffsetPosition);
                const auto beyondKind =
                    effectiveKindAt(voxels, neighbourPosition);
                const auto footprint =
                    kVoxelFaces[side].normal.y > 0.0F
                    && material.kind == voxel::Kind::Normal
                    && beyondKind == voxel::Kind::Normal;

                if (beyondKind.has_value()
                    && voxel::occludes(*beyondKind, material.kind)
                    && !footprint)
                {
                    continue;
                }

                if (kVoxelFaces[side].normal.y > 0.0F
                    && kindAt(voxels, neighbourPosition)
                           == voxel::Kind::Ramp)
                {
                    continue;
                }

                if (kindAt(voxels, neighbourPosition) == voxel::Kind::Ramp
                    && facedByAClimb(
                        voxel::inferredRampDirection(
                            voxels, neighbourPosition),
                        kVoxelFaces[side].normal))
                {
                    continue;
                }

                faces.push_back(
                    FaceRef{
                        .cell = voxel::voxelCellAt(position, material),
                        .side = side,
                        .climbPosition = climb,
                        .levelHalf = level});
            }
        }

        return faces;
    } // GCOVR_EXCL_LINE

}
