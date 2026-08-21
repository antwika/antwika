#pragma once

#include <cstdint>
#include <antwika/rng/IRng.hpp>
#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/wfc/CompatibilityTable.hpp>
#include "antwika/worldgen/ChunkShape.hpp"

namespace antwika::worldgen
{

    enum class RulesetFault : std::uint8_t
    {
        NoPrototypes,
        RampWithoutFacing,
        FaceMeetsNothing,
        NoDistricts,
        DistrictsDoNotRise,
        DistrictMissizes,
        DistrictAllowsNothing,
        RoleWornByNoPrototype,
    };

}
