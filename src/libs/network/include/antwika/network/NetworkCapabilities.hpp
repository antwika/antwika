#pragma once

#include <cstddef>

namespace antwika::network
{

    struct NetworkCapabilities final
    {
        bool connects = false;

        bool listens = false;

        std::size_t maxPeers = 0;

        std::size_t maxPayloadBytes = 0;

        [[nodiscard]] bool operator==(const NetworkCapabilities &other) const
            = default;
    };

}
