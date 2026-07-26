#pragma once

#include <string>

namespace antwika::log
{

    enum class Level
    {
        Trace = 0,
        Debug = 10,
        Info = 20,
        Warning = 30,
        Error = 40,
        Fatal = 50,
    };

    std::string_view toString(Level level) noexcept;

} // namespace antwika::log
