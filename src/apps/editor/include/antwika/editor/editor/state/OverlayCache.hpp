#pragma once

#include <vector>

#include <antwika/solver/FaceSeam.hpp>
#include <antwika/voxelmap/VoxelPick.hpp>

namespace antwika::editor
{

    struct OverlayCache final
    {
        bool stale = true;

        std::vector<voxelmap::LineSegment> gridLines;

        std::vector<voxelmap::LineSegment> topLines;

        std::vector<solver::FaceSeam> seamsAboveLevel;

        std::vector<solver::FaceSeam> seamsAtLevel;
    };

}
