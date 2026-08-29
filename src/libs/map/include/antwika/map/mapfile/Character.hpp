#pragma once

#include <string>
#include <vector>

#include <antwika/loadout/ComponentValue.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

#include "antwika/map/mapfile/Placement.hpp"

namespace antwika::map
{

    struct Character final
    {
        std::string name{};

        Placement idlePlacement{};

        std::vector<voxel::VoxelPosition> patrolPathPositions{};

        std::vector<std::string> dialogue{};

        std::vector<std::string> components{};

        loadout::ComponentValues componentValues{};

        bool player = false;

        [[nodiscard]] bool operator==(const Character &other) const
            = default;
    };

}
