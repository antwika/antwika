#pragma once

#include <ostream>
#include <string_view>

#include "antwika/log/Appender.hpp"

#ifdef _WIN32
#define LOG_EXPORT __declspec(dllexport)
#else
#define LOG_EXPORT
#endif

namespace antwika::log
{

class LOG_EXPORT StreamAppender: public Appender {
public:
    explicit StreamAppender(std::ostream& stream);
    void append(std::string_view message) override;
private:
    std::ostream& stream;
};

} // namespace antwika::log
