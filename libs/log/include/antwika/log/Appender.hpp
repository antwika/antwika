#pragma once

#include <string_view>

namespace antwika::log
{

    class Appender
    {
    public:
        virtual ~Appender() = default;
        virtual void append(std::string_view message) = 0;
    };

} // namespace antwika::log
