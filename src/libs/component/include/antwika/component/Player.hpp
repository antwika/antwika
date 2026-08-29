#pragma once

#include <cstdint>

namespace antwika::component
{

    struct Player final
    {
        std::uint8_t padding = 0;

        [[nodiscard]] bool operator==(
            const Player &other) const = default;
    };

}
