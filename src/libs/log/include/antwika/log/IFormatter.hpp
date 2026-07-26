#pragma once

#include <string>
#include <chrono>

#include "Level.hpp"

namespace antwika::log
{

    class IFormatter
    {
    public:
        virtual ~IFormatter() = default;
        [[nodiscard]] virtual std::string format(std::chrono::system_clock::time_point time, Level level, std::string_view message) = 0;
    };

} // antwika::log
