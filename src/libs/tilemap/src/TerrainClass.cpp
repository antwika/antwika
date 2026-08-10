#include "antwika/tilemap/TerrainClass.hpp"

#include <string_view>

namespace antwika::tilemap
{

    std::string_view toString(TerrainClass terrain) noexcept
    {
        switch (terrain)
        {
            case TerrainClass::Floor:
                return "floor";
            case TerrainClass::Wall:
                return "wall";
            case TerrainClass::Water:
                return "water";
            case TerrainClass::Cliff:
                return "cliff";
            case TerrainClass::Path:
                return "path";
            case TerrainClass::Stair:
                return "stair";
        }

        return "unknown";
    }

}
