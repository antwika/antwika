#pragma once

#include <string>
#include <vector>

#include "antwika/map/mapfile/Map.hpp"

namespace antwika::map
{

    [[nodiscard]] std::vector<std::string> getMapWarnings(const Map &map);

}
