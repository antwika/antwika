#pragma once

#include <string>
#include <chrono>

namespace antwika::log
{

    class IFormatter
    {
    public:
        virtual ~IFormatter() = default;

        virtual std::string format(std::chrono::system_clock::time_point time, std::string_view level, std::string_view message) = 0;
    };

} // antwika::log
