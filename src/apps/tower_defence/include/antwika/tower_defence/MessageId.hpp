#pragma once

#include <cstdint>

namespace antwika::tower_defence
{

    enum class MessageId : std::uint16_t
    {
        Level,

        Wave,

        Lives,

        Score,

        Best,

        Cleared,

        Overrun,

        Count,
    };

}
