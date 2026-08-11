#pragma once

#include <cstdint>

#include "antwika/tilemap/Overlay.hpp"
#include "antwika/tilemap/TerrainClass.hpp"
#include "antwika/tilemap/WaterAttributes.hpp"

namespace antwika::tilemap
{

    struct Slab final
    {
        std::int32_t level = 0;
        TerrainClass terrain = TerrainClass::Floor;
        Overlay overlay = Overlay::None;
        WaterAttributes water{};
        std::uint8_t light = 255;

        [[nodiscard]] bool operator==(const Slab &other) const = default;
    };

}
