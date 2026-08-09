#pragma once

#include <cstdint>

namespace antwika::geometry
{

    struct Size final
    {
        std::uint32_t width = 0;
        std::uint32_t height = 0;

        [[nodiscard]] bool operator==(const Size &other) const = default;
    };

}
