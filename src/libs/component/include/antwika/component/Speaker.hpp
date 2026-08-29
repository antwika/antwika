#pragma once

#include <cstdint>

namespace antwika::component
{

    struct Speaker final
    {
        std::uint32_t nextLineIndex = 0;

        [[nodiscard]] bool operator==(
            const Speaker &other) const = default;
    };

}
