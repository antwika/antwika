#pragma once

#include <string_view>

#include <antwika/input/ActionMap.hpp>

namespace antwika::game
{

    inline constexpr std::string_view kWalkNorth = "walk.north";

    inline constexpr std::string_view kWalkSouth = "walk.south";

    inline constexpr std::string_view kWalkWest = "walk.west";

    inline constexpr std::string_view kWalkEast = "walk.east";

    inline constexpr std::string_view kRun = "run";

    inline constexpr std::string_view kLeave = "leave";

    [[nodiscard]] input::ActionMap defaultActions();

}
