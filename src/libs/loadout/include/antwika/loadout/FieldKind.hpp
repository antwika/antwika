#pragma once

#include <cstdint>

namespace antwika::loadout
{

    enum class FieldKind : std::uint8_t
    {
        Flag,
        Whole,
        Fixed,
        Tint,
        Slots,
    };

}
