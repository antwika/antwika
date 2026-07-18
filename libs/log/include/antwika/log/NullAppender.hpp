#pragma once

#include <string_view>

#include "antwika/log/Appender.hpp"

#ifdef _WIN32
#define LOG_EXPORT __declspec(dllexport)
#else
#define LOG_EXPORT
#endif

namespace antwika::log
{

class LOG_EXPORT NullAppender: public Appender {
public:
    void append(std::string_view message) override;
};

} // namespace antwika::log
