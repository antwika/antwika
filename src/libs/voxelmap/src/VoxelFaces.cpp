#include <cstddef>
#include <vector>

#include <antwika/gfx/Math3D.hpp>

#include <antwika/voxelmap/Voxel.hpp>
#include <antwika/voxel/KindTraits.hpp>

#include "VoxelDetail.hpp"

namespace antwika::voxelmap
{
    using namespace voxeldetail;

    namespace
    {

        [[nodiscard]] bool isFacedByAClimb(
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

        for (const auto &[position, material] : voxel::getSortedCells(voxels))
        {
            const auto climb = voxel::isRamped(material.kind)
                             ? voxel::getInferredRampDirection(voxels, position)
                             : voxel::VoxelPosition{};
            const auto level = voxel::stairHalfOf(voxels, position);

            for (std::size_t side = 0; side < kFaces; ++side)
            {
                const auto neighbourPosition = getOffsetBy(
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
                    && voxel::isRamped(
                        kindAt(voxels, neighbourPosition)))
                {
                    continue;
                }

                if (voxel::isRamped(kindAt(voxels, neighbourPosition))
                    && isFacedByAClimb(
                        voxel::getInferredRampDirection(
                            voxels, neighbourPosition),
                        kVoxelFaces[side].normal))
                {
                    continue;
                }

                faces.push_back(
                    FaceRef{
                        .cell = voxel::VoxelCell{
                        .position = position,
                        .material = material},
                        .side = side,
                        .climbPosition = climb,
                        .levelHalf = level});
            }
        }

        return faces;
    } // GCOVR_EXCL_LINE

}
