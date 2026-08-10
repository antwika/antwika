#include "antwika/tilemap/FlowDirection.hpp"

#include <string_view>

namespace antwika::tilemap
{

    std::string_view toString(FlowDirection direction) noexcept
    {
        switch (direction)
        {
            case FlowDirection::North:
                return "north";
            case FlowDirection::East:
                return "east";
            case FlowDirection::South:
                return "south";
            case FlowDirection::West:
                return "west";
        }

        return "unknown";
    }

}
