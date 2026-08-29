#pragma once

#include <cstdint>

namespace antwika::component
{

    struct CharacterIndex final
    {
        std::uint32_t index = 0;

        [[nodiscard]] bool operator==(
            const CharacterIndex &other) const = default;
    };

}
