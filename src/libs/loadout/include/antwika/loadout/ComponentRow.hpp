#pragma once

#include <span>
#include <string_view>

#include <antwika/loadout/ComponentValue.hpp>
#include <antwika/loadout/FieldRow.hpp>
#include <antwika/loadout/Role.hpp>

namespace antwika::loadout
{

    struct ComponentRow final
    {
        std::string_view name;

        Role role;

        std::span<const FieldRow> fields;

        ComponentValue (*fresh)();
    };

}
