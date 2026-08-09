#pragma once

#include <cstdint>
#include <string_view>

namespace antwika::config
{

    struct Format final
    {
        std::string_view magic;

        std::uint32_t version;
    };

}
