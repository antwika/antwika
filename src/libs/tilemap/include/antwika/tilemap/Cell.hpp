#pragma once

#include <cstdint>

#include "antwika/tilemap/Overlay.hpp"
#include "antwika/tilemap/TerrainClass.hpp"
#include "antwika/tilemap/WaterAttributes.hpp"

namespace antwika::tilemap
{

    struct Cell final
    {
        std::int32_t height = 0;
        TerrainClass terrain = TerrainClass::Floor;
        Overlay overlay = Overlay::None;
        WaterAttributes water{};
        std::uint8_t light = 255;

        [[nodiscard]] bool operator==(const Cell &other) const = default;
    };

}
