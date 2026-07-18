#pragma once

#include <string_view>

#ifdef _WIN32
#define LOG_EXPORT __declspec(dllexport)
#else
#define LOG_EXPORT
#endif

namespace antwika::log
{

class LOG_EXPORT Appender {
public:
    virtual ~Appender() = default;
    virtual void append(std::string_view message) = 0;
};

} // namespace antwika::log
