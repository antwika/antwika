#pragma once

#include <cstdint>
#include <string>

#include "antwika/tilemap/Rgb.hpp"

namespace antwika::tilemap
{

    inline constexpr std::uint32_t kSchemaVersion = 2;

    struct MapHeader final
    {
        std::string id{};
        std::uint32_t schemaVersion = kSchemaVersion;
        Rgb ink{};
        Rgb paper = Rgb{.red = 255, .green = 255, .blue = 255};

        [[nodiscard]] bool operator==(const MapHeader &other) const
            = default;
    };

}
