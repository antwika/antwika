#pragma once

#include <cstddef>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/MeshData.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

namespace antwika::voxelmap
{

    struct QuadPaint final
    {
        gfx::RectF tileRect{};

        bool mirrored = false;

        gfx::Vec3 liftPoint{0.0F, 0.0F, 0.0F};

        gfx::Color color{
            .red = 255, .green = 255, .blue = 255, .alpha = 255};
    };

    void addFaceQuads(
        gfx::MeshData &mesh,
        std::size_t side,
        voxel::VoxelPosition climbPosition,
        bool climbing,
        gfx::Vec3 middlePoint,
        const QuadPaint &paint);

}
