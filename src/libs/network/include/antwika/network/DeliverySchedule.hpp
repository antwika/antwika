#pragma once

#include <cstddef>
#include <vector>

namespace antwika::network
{

    struct DeliverySchedule final
    {
        std::size_t delayPumps = 0;

        std::vector<std::size_t> dropped;

        [[nodiscard]] bool operator==(const DeliverySchedule &other) const
            = default;
    };

}
