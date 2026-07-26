#include "antwika/log/PlainFormatter.hpp"

#include <format>

namespace antwika::log
{

    std::string PlainFormatter::format(std::chrono::system_clock::time_point time, Level level, std::string_view message) const
    {
        return std::format(
            "[{:%Y-%m-%d %H:%M:%S}] [{}] {}",
            std::chrono::floor<std::chrono::seconds>(time),
            toString(level),
            message);
    }

} // antwika::log
