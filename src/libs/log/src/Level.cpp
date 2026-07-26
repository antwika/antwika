#include "antwika/log/Level.hpp"

#include <string>

namespace antwika::log
{

    std::string_view toString(Level level) noexcept
    {
        switch (level)
        {
        case Level::Trace:
            return "TRACE";
        case Level::Debug:
            return "DEBUG";
        case Level::Info:
            return "INFO";
        case Level::Warning:
            return "WARNING";
        case Level::Error:
            return "ERROR";
        case Level::Fatal:
            return "FATAL";
        default:
            return "UNKNOWN";
        }
    }

} // namespace antwika::log
