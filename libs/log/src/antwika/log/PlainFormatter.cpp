#include "antwika/log/PlainFormatter.hpp"

namespace antwika::log
{

    std::string PlainFormatter::format(std::chrono::system_clock::time_point time, std::string_view level, std::string_view message)
    {
        return std::format(
            "[{:%Y-%m-%d %H:%M:%S}] [{}] {}",
            std::chrono::floor<std::chrono::seconds>(time),
            level,
            message);
    }

} // antwika::log
