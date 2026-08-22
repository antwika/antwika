#pragma once

#include <istream>
#include <ostream>
#include <vector>
#include <antwika/camera/FlyCamera.hpp>
#include <antwika/decor/Decor.hpp>
#include <antwika/decor/TileAnimation.hpp>
#include <antwika/decor/Variants.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/io/SafeWrite.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/map/Layers.hpp>
#include <antwika/map/Settings.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/tile/TilePaint.hpp>
#include <antwika/tile/TileRules.hpp>
#include <antwika/tile/Transitions.hpp>
#include <antwika/voxelmap/Voxel.hpp>

namespace antwika::map
{

    struct PressurePlate final
    {
        voxel::VoxelPosition position{};

        std::vector<voxel::VoxelPosition> togglePositions{};

        [[nodiscard]] bool operator==(const PressurePlate &other) const
            = default;
    };

}
