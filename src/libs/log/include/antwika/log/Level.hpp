#pragma once

#include <cstdint>
#include <string>

namespace antwika::log
{

    enum class Level : std::uint8_t
    {
        Trace = 0,
        Debug = 10,
        Info = 20,
        Warning = 30,
        Error = 40,
        Fatal = 50,
    };

    [[nodiscard]] std::string_view toString(Level level) noexcept;

}
