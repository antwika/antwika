#pragma once

#include <cstdint>
#include <string>

namespace antwika::editor
{

    struct StatusMessage final
    {
        std::string text;

        std::uint32_t expiresAtTick = 0;

        bool warns = false;
    };

}
