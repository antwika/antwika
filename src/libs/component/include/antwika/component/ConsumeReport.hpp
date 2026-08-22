#pragma once

#include <cstdint>

namespace antwika::component
{

    struct ConsumeReport final
    {
        std::uint8_t kind = 0;

        bool anyLeft = false;
    };

}
