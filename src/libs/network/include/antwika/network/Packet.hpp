#pragma once

#include <cstddef>
#include <vector>

#include "antwika/network/PeerId.hpp"

namespace antwika::network
{

    struct Packet final
    {
        PeerId from{};

        std::vector<std::byte> payload;

        [[nodiscard]] bool operator==(const Packet &other) const = default;
    };

}
