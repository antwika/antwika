#pragma once

#include <istream>
#include <ostream>
#include <string>
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
#include "antwika/map/mapfile/Placement.hpp"

namespace antwika::map
{

    struct Character final
    {
        std::string name{};

        Placement idlePlacement{};

        std::vector<voxel::VoxelCell> patrolPathCells{};

        std::vector<std::string> dialogue{};

        std::vector<std::string> components{};

        bool player = false;

        [[nodiscard]] bool operator==(const Character &other) const
            = default;
    };

}
