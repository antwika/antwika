#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace antwika::component
{

    inline constexpr std::size_t kInventorySlots = 4;

    inline constexpr std::uint8_t kEmptySlot = 0;

    struct Inventory final
    {
        std::array<std::uint8_t, kInventorySlots> slots{};
    };

}
