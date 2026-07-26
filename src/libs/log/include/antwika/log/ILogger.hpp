#pragma once

#include <string_view>

#include "ILogPolicy.hpp"
#include "Level.hpp"

namespace antwika::log
{

    class ILogger
    {
    public:
        virtual ~ILogger() = default;
        virtual void log(Level level, std::string_view message) noexcept = 0;
    };

} // namespace antwika::log
