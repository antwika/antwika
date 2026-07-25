#pragma once

#include <string>

namespace antwika::log
{

    enum class Level
    {
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        Fatal,
    };

    std::string toString(Level level);

} // namespace antwika::log
