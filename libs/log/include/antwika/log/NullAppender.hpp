#pragma once

#include <string_view>

#include "antwika/log/Appender.hpp"

namespace antwika::log
{

    class NullAppender : public Appender
    {
    public:
        void append(std::string_view message) override;
    };

} // namespace antwika::log
