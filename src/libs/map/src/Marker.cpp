#include "antwika/map/Marker.hpp"

namespace antwika::map
{

    std::optional<Marker> markerFor(const Tool tool)
    {
        switch (tool)
        {
        case Tool::Key:
            return Marker::Key;
        case Tool::Door:
            return Marker::Door;
        case Tool::Checkpoint:
            return Marker::Checkpoint;
        case Tool::Food:
            return Marker::Food;
        case Tool::Water:
            return Marker::Water;
        default:
            break;
        }

        return std::nullopt;
    }

}
