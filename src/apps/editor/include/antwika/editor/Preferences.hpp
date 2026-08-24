#pragma once

#include <antwika/map/Settings.hpp>
#include <antwika/voxel/VoxelMaterial.hpp>

namespace antwika::editor
{

    struct Preferences final
    {
        map::Tool tool = map::Tool::Brush;

        map::Paint paint = map::Paint::Brush;

        map::View view = map::View::World;

        voxel::Kind kind = voxel::Kind::Normal;

        bool showRuleLines = true;

        bool grid = true;

        bool showPlacementGhost = true;

        bool lampSight = true;

        bool cameraFollows = true;

        bool hideAboveLevel = false;

        [[nodiscard]] bool operator==(const Preferences &other) const
            = default;
    };

}
