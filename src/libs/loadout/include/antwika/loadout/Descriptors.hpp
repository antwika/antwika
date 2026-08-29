#pragma once

#include <span>
#include <string_view>

#include <antwika/loadout/ComponentRow.hpp>

namespace antwika::loadout
{

    [[nodiscard]] std::span<const ComponentRow> getComponentRows();

    [[nodiscard]] const ComponentRow *getComponentRow(
        std::string_view name);

}
