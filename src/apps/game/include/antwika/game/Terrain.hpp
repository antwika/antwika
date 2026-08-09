#pragma once

#include <cstddef>
#include <cstdint>

namespace antwika::game
{

    enum class Terrain : std::uint8_t
    {
        Water = 0,
        Plains = 1,
        Forest = 2,
        Hills = 3,
        Mountain = 4,
    };

    inline constexpr std::size_t kTerrainCount = 5;

    [[nodiscard]] constexpr bool isLand(Terrain terrain) noexcept
    {
        return terrain != Terrain::Water;
    }

    [[nodiscard]] constexpr Terrain terrainOf(std::size_t symbol) noexcept
    {
        return symbol < kTerrainCount
                   ? static_cast<Terrain>(symbol)
                   : Terrain::Mountain;
    }

    [[nodiscard]] constexpr std::size_t symbolOf(Terrain terrain) noexcept
    {
        return static_cast<std::size_t>(terrain);
    }

}
