#pragma once

#include <cstdint>

#include <antwika/loadout/ComponentValue.hpp>
#include <antwika/map/mapfile/Placement.hpp>

namespace antwika::gameplay
{

    struct SpawnContext final
    {
        map::Placement placement{};

        std::uint32_t index = 0;

        const loadout::ComponentValues *componentValues = nullptr;
    };

}
