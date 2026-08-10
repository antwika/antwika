#pragma once

#include <cstdint>
#include <string_view>

namespace antwika::tilemap
{

    enum class TerrainClass : std::uint8_t
    {
        Floor = 0,
        Wall,
        Water,
        Cliff,
        Path,
        Stair,
    };

    [[nodiscard]] constexpr TerrainClass enumBound(TerrainClass) noexcept
    {
        return TerrainClass::Stair;
    }

    [[nodiscard]] std::string_view toString(TerrainClass terrain) noexcept;

}
