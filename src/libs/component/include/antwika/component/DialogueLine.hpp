#pragma once

#include <cstdint>

namespace antwika::component
{

    struct DialogueLine final
    {
        std::uint32_t characterIndex = 0;

        std::uint32_t lineIndex = 0;
    };

}
