#pragma once

#include <antwika/map/Settings.hpp>

namespace antwika::editor
{

    struct ViewChoice final
    {
        map::View activeView = map::View::World;
    };

}
