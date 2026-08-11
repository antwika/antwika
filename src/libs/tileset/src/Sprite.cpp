#include "antwika/tileset/Sprite.hpp"

#include <string_view>

namespace antwika::tileset
{

    std::string_view toString(Side side) noexcept
    {
        switch (side)
        {
            case Side::North:
                return "north";
            case Side::East:
                return "east";
            case Side::South:
                return "south";
            case Side::West:
                return "west";
        }

        return "unknown";
    }

}
