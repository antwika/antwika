#pragma once

#include <array>
#include <cstdint>
#include <string>

#include <antwika/enums/Enumeration.hpp>

#include "antwika/tilemap/Rgb.hpp"
#include "antwika/tilemap/TerrainClass.hpp"

namespace antwika::tilemap
{

    inline constexpr std::uint32_t kSchemaVersion = 4;

    struct MapHeader final
    {
        std::string id{};
        std::uint32_t schemaVersion = kSchemaVersion;
        Rgb ink{};
        Rgb paper = Rgb{.red = 255, .green = 255, .blue = 255};
        std::array<std::string, enums::kCount<TerrainClass>>
            tilesets{};

        [[nodiscard]] bool operator==(const MapHeader &other) const
            = default;
    };

}
