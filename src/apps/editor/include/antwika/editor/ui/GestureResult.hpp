#pragma once

#include <antwika/enums/Enumeration.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/map/Settings.hpp>
#include <antwika/tilemap/TileEdges.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/ui/WidgetId.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include "antwika/editor/ui/EdgeSelection.hpp"
#include "antwika/editor/ui/PointerAction.hpp"

namespace antwika::editor
{

    struct GestureResult final
    {
        PointerAction action = PointerAction::Nothing;

        geometry::GridCell fromCell{};

        geometry::GridCell toCell{};

        EdgeSelection selection{};

        voxel::Corner corner = voxel::Corner::TopLeft;

        [[nodiscard]] bool operator==(const GestureResult &other) const
            = default;
    };

}
