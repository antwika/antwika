#pragma once

#include <antwika/map/MapFile.hpp>

namespace antwika::editor
{

    inline constexpr std::string_view kCarriedLightName =
        "component::CarriedLight";

    [[nodiscard]] bool carriesLight(const map::Character &character);

    void toggleCarriedLight(map::Character &character);

}
