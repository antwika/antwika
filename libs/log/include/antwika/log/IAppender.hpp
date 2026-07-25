#pragma once

#include <string_view>

namespace antwika::log
{

    class IAppender
    {
    public:
        virtual ~IAppender() = default;
        virtual void append(std::string_view message) = 0;
    };

} // namespace antwika::log
