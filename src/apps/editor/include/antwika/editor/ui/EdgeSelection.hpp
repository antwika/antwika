#pragma once

#include <optional>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/tilemap/TileEdges.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/widget/WidgetId.hpp>
#include <antwika/voxel/VoxelCube.hpp>

namespace antwika::editor
{

    struct EdgeSelection final
    {
        voxel::Side side = voxel::Side::Top;

        std::optional<voxel::EdgeKind> edge = voxel::EdgeKind::Boundary;

        [[nodiscard]] bool operator==(
            const EdgeSelection &other) const = default;
    };

}
