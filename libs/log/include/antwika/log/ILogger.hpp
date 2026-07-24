#pragma once

#include <string_view>

namespace antwika::log
{

    class ILogger
    {
    public:
        virtual ~ILogger() = default;
        virtual void trace(std::string_view) noexcept = 0;
        virtual void debug(std::string_view) noexcept = 0;
        virtual void info(std::string_view) noexcept = 0;
        virtual void warning(std::string_view) noexcept = 0;
        virtual void error(std::string_view) noexcept = 0;
        virtual void fatal(std::string_view) noexcept = 0;
    };

} // namespace antwika::log
