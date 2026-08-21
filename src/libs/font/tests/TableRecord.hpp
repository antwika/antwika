#pragma once

#include <cstdint>
#include <string_view>

namespace antwika::font::tests
{

    struct TableRecord final
    {
        std::string_view tag;
        std::uint32_t offset = 0;
        std::uint32_t length = 0;
    };

}
