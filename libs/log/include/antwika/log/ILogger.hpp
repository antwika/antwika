#pragma once

#include <string_view>

#ifdef _WIN32
#define LOG_EXPORT __declspec(dllexport)
#else
#define LOG_EXPORT
#endif

namespace antwika::log
{

    class LOG_EXPORT ILogger
    {
    public:
        virtual ~ILogger() = default;
        virtual void trace(std::string_view) = 0;
        virtual void debug(std::string_view) = 0;
        virtual void info(std::string_view) = 0;
        virtual void warning(std::string_view) = 0;
        virtual void error(std::string_view) = 0;
        virtual void fatal(std::string_view) = 0;
    };

} // namespace antwika::log
