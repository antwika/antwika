#pragma once

#include <cstddef>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/MeshData.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/Voxels.hpp>

namespace antwika::voxelmap
{

    struct QuadPaint final
    {
        gfx::RectF tileRect{};

        bool mirrored = false;

        gfx::Vec3 liftPoint{0.0F, 0.0F, 0.0F};

        gfx::Color color{
            .red = 255, .green = 255, .blue = 255, .alpha = 255};

        // Whether the face's border band may sink into a bevel
        // where its edges stand open to the air. Stays off for
        // water, stairs and ramps, whose edges keep their corners.
        bool beveled = false;
    };

    void addFaceQuads(
        gfx::MeshData &mesh,
        const voxel::Voxels &voxels,
        std::size_t side,
        voxel::VoxelPosition climbPosition,
        bool climbing,
        gfx::Vec3 middlePoint,
        const QuadPaint &paint);

}
