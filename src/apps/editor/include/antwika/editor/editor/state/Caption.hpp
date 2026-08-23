#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace antwika::editor
{

    struct Caption final
    {
        std::string name;

        std::string line;

        std::optional<std::size_t> speaker;

        std::uint32_t start = 0;

        std::uint32_t untilTick = 0;
    };

}
