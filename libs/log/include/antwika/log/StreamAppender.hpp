#pragma once

#include <ostream>
#include <string_view>

#include "antwika/log/Appender.hpp"

namespace antwika::log
{

    class StreamAppender : public Appender
    {
    public:
        explicit StreamAppender(std::ostream &stream);
        void append(std::string_view message) override;

    private:
        std::ostream &stream;
    };

} // namespace antwika::log
