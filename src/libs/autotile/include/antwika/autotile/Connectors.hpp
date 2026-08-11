#pragma once

#include <array>
#include <cstdint>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/tilemap/TerrainClass.hpp>

namespace antwika::autotile
{

    inline constexpr std::uint8_t kEdgeNorth = 1;
    inline constexpr std::uint8_t kEdgeEast = 2;
    inline constexpr std::uint8_t kEdgeSouth = 4;
    inline constexpr std::uint8_t kEdgeWest = 8;
    inline constexpr std::uint8_t kEdgeAll = 15;

    inline constexpr std::size_t kQuadrantSlots = 16;

    struct SheetConnectors final
    {
        std::array<std::uint8_t, 8> edges{
            kEdgeAll,
            kEdgeAll,
            kEdgeAll,
            kEdgeAll,
            kEdgeAll,
            kEdgeAll,
            kEdgeAll,
            kEdgeAll};
        std::array<std::uint8_t, kQuadrantSlots> quadrants{};
        std::uint16_t quadrantMask = 0;

        [[nodiscard]] bool operator==(
            const SheetConnectors &other) const = default;
    };

    using TerrainConnectors = std::array<
        SheetConnectors,
        enums::kCount<tilemap::TerrainClass>>;

}
