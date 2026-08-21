#pragma once

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/voxel/VoxelOcclusion.hpp>
#include <antwika/voxelmap/Voxel.hpp>

namespace antwika::voxelmap
{

    struct LineSegment final
    {
        gfx::Vec3 fromPosition{0.0F, 0.0F, 0.0F};

        gfx::Vec3 toPosition{0.0F, 0.0F, 0.0F};

        [[nodiscard]] bool operator==(const LineSegment &other) const
            = default;
    };

}
