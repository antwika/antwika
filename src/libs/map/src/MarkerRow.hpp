#pragma once

#include <string_view>

#include "MapFileShared.hpp"

namespace antwika::map::mapfile
{

    struct MarkerRow final
    {
        std::string_view key;

        Marker marker;
    };

}
